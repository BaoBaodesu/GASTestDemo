// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_PrimaryComboAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/T_BaseCharacter.h"
#include "Engine/HitResult.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
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

	static ConstructorHelpers::FObjectFinder<USoundBase> ImpactSoundFinder(
		TEXT("/Game/GASTestDemo/Audio/SC_Punch.SC_Punch"));
	if (ImpactSoundFinder.Succeeded())
	{
		ImpactSound = ImpactSoundFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactSystemFinder(
		TEXT("/Game/VisualSandbox/FX/FXS_Default_Impact_Flesh.FXS_Default_Impact_Flesh"));
	if (ImpactSystemFinder.Succeeded())
	{
		ImpactSystem = ImpactSystemFinder.Object;
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
	ActorsHitThisSwing.Reset();

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
	ActorsHitThisSwing.Reset();

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

bool UT_PrimaryComboAbility::ApplyComboHitTarget(AActor* HitActor, const FHitResult* HitResult)
{
	if (!IsValid(HitActor)) return false;

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(AvatarActor) || !IsValid(SourceASC) || !IsValid(DamageEffectClass)) return false;

	if (!AvatarActor->HasAuthority()) return false;

	const AT_BaseCharacter* TargetCharacter = Cast<AT_BaseCharacter>(HitActor);
	if (!IsValid(TargetCharacter) || !TargetCharacter->IsAlive()) return false;
	if (HitActor == AvatarActor) return false;

	const bool bAlreadyHit = ActorsHitThisSwing.ContainsByPredicate(
		[HitActor](const TWeakObjectPtr<AActor>& Entry)
		{
			return Entry.Get() == HitActor;
		});
	if (bAlreadyHit) return false;

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
	if (!IsValid(TargetASC)) return false;

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(AvatarActor, AvatarActor);
	if (HitResult)
	{
		ContextHandle.AddHitResult(*HitResult);
	}

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, GetAbilityLevel(), ContextHandle);
	if (!SpecHandle.IsValid()) return false;

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	ActorsHitThisSwing.Add(HitActor);

	const FVector ImpactLocation = HitResult ? FVector(HitResult->ImpactPoint) : HitActor->GetActorLocation();
	const FRotator ImpactRotation = HitResult ? HitResult->ImpactNormal.Rotation() : FRotator::ZeroRotator;
	if (IsValid(ImpactSound))
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, ImpactLocation);
	}
	if (IsValid(ImpactSystem))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactSystem, ImpactLocation, ImpactRotation);
	}

	FGameplayEventData HitReactPayload;
	HitReactPayload.Instigator = AvatarActor;
	HitReactPayload.Target = HitActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor, TTags::Events::Enemy::HitReact, HitReactPayload);

	return true;
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
	ActorsHitThisSwing.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
