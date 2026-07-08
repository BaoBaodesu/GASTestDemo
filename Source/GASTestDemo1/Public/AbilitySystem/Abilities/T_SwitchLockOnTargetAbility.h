// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_GameplayAbility.h"
#include "T_SwitchLockOnTargetAbility.generated.h"

/**
 * 
 */
UCLASS()
class GASTESTDEMO1_API UT_SwitchLockOnTargetAbility : public UT_GameplayAbility
{
	GENERATED_BODY()
	
public:
	UT_SwitchLockOnTargetAbility();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LockOn")
	float Direction = 1.0f;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;
};
