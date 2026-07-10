// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_GameplayAbility.h"
#include "T_PrimaryComboAbility.generated.h"

/**
 * 
 */
UCLASS()
class GASTESTDEMO1_API UT_PrimaryComboAbility : public UT_GameplayAbility
{
	GENERATED_BODY()
public:
	UT_PrimaryComboAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData
	) override;

	virtual void InputPressed(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo
	) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	int32 ComboIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	bool bQueuedNextCombo = false;
	
	bool bComboMontageSwitching = false;

	void PlayComboMontage();

	UFUNCTION()
	void TryPlayNextCombo();

	UFUNCTION()
	void OnComboMontageInterrupted();

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled
	) override;
};
