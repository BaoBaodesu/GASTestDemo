// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_PrimaryComboAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "Utils/T_BlueprintLibrary.h"
#include "UObject/ConstructorHelpers.h"

UT_PrimaryComboAbility::UT_PrimaryComboAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(
		TEXT("/Game/GASTestDemo/AbilitySystem/GameplayEffects/Player/GE_PlayerDanage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void UT_PrimaryComboAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ComboIndex = 0;
	bQueuedNextCombo = false;

	UAbilityTask_WaitGameplayEvent* WaitPrimaryAttackEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, TTags::Events::Player::Primary, nullptr, false, false);
	if (IsValid(WaitPrimaryAttackEvent))
	{
		WaitPrimaryAttackEvent->EventReceived.AddDynamic(this, &UT_PrimaryComboAbility::OnPrimaryAttackEvent);
		WaitPrimaryAttackEvent->ReadyForActivation();
	}

	PlayComboMontage();
}

void UT_PrimaryComboAbility::InputPressed(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	Super::InputPressed(Handle, ActorInfo, ActivationInfo);

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid()) return;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (ASC->HasMatchingGameplayTag(TTags::State::Action::ComboWindow))
	{
		if (bQueuedNextCombo) return;
		
		bQueuedNextCombo = true;
		bComboMontageSwitching = true;
		TryPlayNextCombo();
		bComboMontageSwitching = false;
	}
}

void UT_PrimaryComboAbility::PlayComboMontage()
{
	if (!AttackMontages.IsValidIndex(ComboIndex) || !IsValid(AttackMontages[ComboIndex]))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	bQueuedNextCombo = false;

	UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AttackMontages[ComboIndex],
		1.0f,
		NAME_None,
		true,
		1.0f,
		0.0f,
		false
	);

	if (!IsValid(PlayMontageTask))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	PlayMontageTask->OnCompleted.AddDynamic(this, &UT_PrimaryComboAbility::TryPlayNextCombo);
	PlayMontageTask->OnBlendOut.AddDynamic(this, &UT_PrimaryComboAbility::OnComboMontageInterrupted);
	PlayMontageTask->OnInterrupted.AddDynamic(this, &UT_PrimaryComboAbility::OnComboMontageInterrupted);
	PlayMontageTask->OnCancelled.AddDynamic(this, &UT_PrimaryComboAbility::OnComboMontageInterrupted);

	PlayMontageTask->ReadyForActivation();
}

void UT_PrimaryComboAbility::OnPrimaryAttackEvent(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(AvatarActor) || !IsValid(SourceASC) || !IsValid(DamageEffectClass)) return;

	const TArray<AActor*> ActorsHit = UT_BlueprintLibrary::HitBoxOverlapTest(
		AvatarActor, HitBoxRadius, HitBoxForwardOffset, HitBoxElevationOffset);

	for (AActor* HitActor : ActorsHit)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!IsValid(TargetASC)) continue;

		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
		if (SpecHandle.IsValid())
		{
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}

		FGameplayEventData HitReactPayload;
		HitReactPayload.Instigator = AvatarActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, TTags::Events::Enemy::HitReact, HitReactPayload);
	}
}

void UT_PrimaryComboAbility::TryPlayNextCombo()
{
	if (!bQueuedNextCombo)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	ComboIndex++;

	if (!AttackMontages.IsValidIndex(ComboIndex))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	PlayComboMontage();
}

void UT_PrimaryComboAbility::OnComboMontageInterrupted()
{
	if (bComboMontageSwitching) return;
	
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UT_PrimaryComboAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ComboIndex = 0;
	bQueuedNextCombo = false;
	bComboMontageSwitching = false;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
