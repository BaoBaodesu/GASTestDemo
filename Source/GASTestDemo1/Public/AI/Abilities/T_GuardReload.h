// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "T_GuardReload.generated.h"

class UAbilityTask_PlayMontageAndWait;
class UAnimMontage;
class UT_AttributeSet;

/**
 * Guard 换弹能力：服务器权威，播放换弹蒙太奇，动画成功结束时才转移弹药。
 */
UCLASS()
class GASTESTDEMO1_API UT_GuardReload : public UT_GameplayAbility
{
	GENERATED_BODY()

public:

	UT_GuardReload();

	// 激活期间持有/阻挡标签（供测试与蓝图检查）
	const FGameplayTagContainer& GetActivationOwnedTags() const { return ActivationOwnedTags; }
	const FGameplayTagContainer& GetActivationBlockedTags() const { return ActivationBlockedTags; }

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Reload|Animation")
	TObjectPtr<UAnimMontage> ReloadMontage;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	TWeakObjectPtr<UT_AttributeSet> CachedAttributeSet;
	bool bReloadAmmoApplied{false};
};
