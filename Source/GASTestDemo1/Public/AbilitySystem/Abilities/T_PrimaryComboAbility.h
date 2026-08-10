// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_GameplayAbility.h"
#include "T_PrimaryComboAbility.generated.h"

struct FHitResult;

/**
 * 玩家主攻连击：蒙太奇分段播放 + ComboWindow 输入；命中由 AnimNotifyState_ComboHit 驱动。
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

	// 由 ComboHit NotifyState 调用：对目标结算一次伤害，成功返回 true
	bool ApplyComboHitTarget(AActor* HitActor, const FHitResult* HitResult = nullptr);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combo")
	TArray<TObjectPtr<UAnimMontage>> AttackMontages;

	UPROPERTY(EditDefaultsOnly, Category = "Combo|Damage")
	TSubclassOf<class UGameplayEffect> DamageEffectClass;

	// 已废弃：命中改由 AnimNotifyState_ComboHit 骨骼检测，保留以免破坏蓝图序列化
	UPROPERTY(EditDefaultsOnly, Category = "Combo|Damage|Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Use AnimNotifyState_ComboHit sockets instead."))
	float HitBoxRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo|Damage|Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Use AnimNotifyState_ComboHit sockets instead."))
	float HitBoxForwardOffset = 200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Combo|Damage|Deprecated", meta = (DeprecatedProperty, DeprecationMessage = "Use AnimNotifyState_ComboHit sockets instead."))
	float HitBoxElevationOffset = 20.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	int32 ComboIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Combo")
	bool bQueuedNextCombo = false;
	
	bool bComboMontageSwitching = false;

	// 当前段攻击已命中目标（换段时清空）
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> ActorsHitThisSwing;

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
