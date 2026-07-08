// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_LockOnAbility.h"
#include "Player/Components/T_LockOnComponent.h"
#include "GameplayTags/TTags.h"

UT_LockOnAbility::UT_LockOnAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	AbilityTags.AddTag(TTags::TAbilities::LockOn);
	ActivationOwnedTags.AddTag(TTags::State::LockOn);
}

void UT_LockOnAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                       const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	if (!IsValid(AvatarActor))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UT_LockOnComponent* LockOnComponent = AvatarActor->FindComponentByClass<UT_LockOnComponent>();

	if (!IsValid(LockOnComponent))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	LockOnComponent->ToggleLockOn();

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
