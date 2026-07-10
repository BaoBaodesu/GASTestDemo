// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_GameplayAbility.h"
#include "Utils/T_BlueprintLibrary.h"
#include "T_RollAbility.generated.h"

/**
 * 
 */
class UMotionWarpingComponent;
class UAbilityTask_PlayMontageAndWait;
UCLASS()
class GASTESTDEMO1_API UT_RollAbility : public UT_GameplayAbility
{
	GENERATED_BODY()
	
public:
	UT_RollAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll")
	TObjectPtr<UAnimMontage> RollMontage;

	// 额外增加的翻滚距离，所有方向共用同一个值
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll")
	float RollExtraDistance = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roll")
	FName WarpTargetName = FName("Roll");

	UPROPERTY(BlueprintReadOnly, Category = "Roll")
	ERollDirection RollDirection = ERollDirection::Forward;

	UPROPERTY(BlueprintReadOnly, Category = "Roll")
	FName RollSectionName = FName("Forward");

	FVector2D GetMovementInput() const;

	FVector GetRollLocalDirection(const ERollDirection& InRollDirection) const;

	UFUNCTION()
	void OnRollFinished();

	UFUNCTION()
	void OnRollCancelled();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;
};
