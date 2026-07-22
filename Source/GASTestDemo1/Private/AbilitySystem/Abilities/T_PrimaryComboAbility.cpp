// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_PrimaryComboAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameplayTags/TTags.h"

UT_PrimaryComboAbility::UT_PrimaryComboAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
	PlayMontageTask->OnInterrupted.AddDynamic(this, &UT_PrimaryComboAbility::OnComboMontageInterrupted);
	PlayMontageTask->OnCancelled.AddDynamic(this, &UT_PrimaryComboAbility::OnComboMontageInterrupted);

	PlayMontageTask->ReadyForActivation();
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
