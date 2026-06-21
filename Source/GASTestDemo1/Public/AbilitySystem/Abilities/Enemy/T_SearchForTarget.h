// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "T_SearchForTarget.generated.h"

namespace EPathFollowingResult
{
	enum Type : int;
}

/**
 * 
 */
class AT_EnemyCharacter;
class AAIController;
class AT_BaseCharacter;
class UAITask_MoveTo;
class UAbilityTask_WaitDelay;
class UT_WaitGameplayEvent;
UCLASS()
class GASTESTDEMO1_API UT_SearchForTarget : public UT_GameplayAbility
{
	GENERATED_BODY()
	
public:
	UT_SearchForTarget();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	TWeakObjectPtr<AT_EnemyCharacter> OwningEnemy;
	TWeakObjectPtr<AAIController> OwningAIController;
	TWeakObjectPtr<AT_BaseCharacter> TargetBaseCharacter;
	
private:

	UPROPERTY()
	TObjectPtr<UT_WaitGameplayEvent> WaitGameplayEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> SearchDelayTask;

	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveToLocationOrActorTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> AttackDelayTask;

	void StartSearch();

	UFUNCTION()
	void EndAttackEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void Search();

	void MoveToTargetAndAttack();

	UFUNCTION()
	void AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result, AAIController* AIController);

	UFUNCTION()
	void Attack();
};
