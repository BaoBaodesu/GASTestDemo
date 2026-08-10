// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/Abilities/T_GuardReload.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AI/Abilities/T_GuardAmmoLibrary.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Characters/T_GuardCharacter.h"
#include "GameplayTags/TTags.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FGameplayTag& GetGuardDeadTag()
	{
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("TTags.Status.Dead"));
		return DeadTag;
	}
}

UT_GuardReload::UT_GuardReload()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Enemy::Reload.GetTag()));
	ActivationOwnedTags.AddTag(TTags::State::Action::Reloading);
	ActivationBlockedTags.AddTag(TTags::State::Action::Shooting);
	ActivationBlockedTags.AddTag(TTags::State::Action::HitReact);
	const FGameplayTag& DeadTag = GetGuardDeadTag();
	if (DeadTag.IsValid()) ActivationBlockedTags.AddTag(DeadTag);

	static ConstructorHelpers::FObjectFinder<UAnimMontage> ReloadMontageAsset(TEXT("/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload_Montage.MM_Pistol_Reload_Montage"));
	ReloadMontage = ReloadMontageAsset.Object;
}

void UT_GuardReload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(GetAvatarActorFromActorInfo());
	if (!IsValid(Guard))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(Guard->GetAttributeSet());
	if (!IsValid(AttributeSet)
		|| AttributeSet->GetMagazineAmmo() >= AttributeSet->GetMaxMagazineAmmo()
		|| AttributeSet->GetReserveAmmo() <= 0.f)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!IsValid(ReloadMontage))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: UT_GuardReload 未配置换弹蒙太奇。"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	CachedAttributeSet = AttributeSet;
	bReloadAmmoApplied = false;

	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ReloadMontage, 1.f, NAME_None, false);
	if (!IsValid(MontageTask))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	// BlendOut 也视为正常结束（否则弹药不补，BT 会空换弹死循环）
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageCancelled);
	MontageTask->ReadyForActivation();
}

void UT_GuardReload::OnMontageCompleted()
{
	if (bReloadAmmoApplied)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 动画正常完成才转移弹药
	if (UT_AttributeSet* AttributeSet = CachedAttributeSet.Get())
	{
		UT_GuardAmmoLibrary::ApplyReload(AttributeSet);
	}
	bReloadAmmoApplied = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_GuardReload::OnMontageCancelled()
{
	// 被打断不补充弹药
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UT_GuardReload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (IsValid(MontageTask)) MontageTask->EndTask();
	MontageTask = nullptr;
	CachedAttributeSet = nullptr;
	bReloadAmmoApplied = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
