// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_SwitchLockOnTargetAbility.h"

#include "Player/Components/T_LockOnComponent.h"

UT_SwitchLockOnTargetAbility::UT_SwitchLockOnTargetAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UT_SwitchLockOnTargetAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (UT_LockOnComponent* LockOnComponent = AvatarActor ? AvatarActor->FindComponentByClass<UT_LockOnComponent>() : nullptr)
	{
		LockOnComponent->SwitchTarget(Direction);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}