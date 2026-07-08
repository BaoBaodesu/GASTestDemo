// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_GameplayAbility.h"
#include "T_LockOnAbility.generated.h"

/**
 * 
 */
UCLASS()
class GASTESTDEMO1_API UT_LockOnAbility : public UT_GameplayAbility
{
	GENERATED_BODY()
	
public:
	UT_LockOnAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
};
