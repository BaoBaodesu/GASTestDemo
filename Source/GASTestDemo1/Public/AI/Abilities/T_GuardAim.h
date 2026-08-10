// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "T_GuardAim.generated.h"

class AT_ShooterAIController;
class UAnimMontage;
class UAnimSequence;

/**
 * Guard 瞄准能力：服务器权威，使用 Controller 当前目标与 Focus/ControlRotation，不依赖玩家相机。
 */
UCLASS()
class GASTESTDEMO1_API UT_GuardAim : public UT_GameplayAbility
{
	GENERATED_BODY()

public:

	UT_GuardAim();

	// 激活期间持有标签（供测试与蓝图检查）
	const FGameplayTagContainer& GetActivationOwnedTags() const { return ActivationOwnedTags; }
	const FGameplayTagContainer& GetActivationBlockedTags() const { return ActivationBlockedTags; }

	// 射击蒙太奇打断后，若仍在瞄准则重新播放 ADS 姿势
	void RestartAimPose();

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// ABP_Shooter 依赖 BP_ShooterCharacter 赋值失败时，用 UpperBody Slot 强制 ADS 举枪姿势
	UPROPERTY(EditDefaultsOnly, Category = "Guard|Aim|Animation")
	TObjectPtr<UAnimSequence> AimIdleSequence;

	// 为空时自动使用 FireMontage 的 Slot（ABP 仅有部分 Slot 接入最终输出）
	UPROPERTY(EditDefaultsOnly, Category = "Guard|Aim|Animation")
	FName AimSlotName;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Aim|Animation")
	TObjectPtr<UAnimMontage> SlotReferenceMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Aim|Animation", meta = (ClampMin = "0.0"))
	float AimBlendInTime{0.2f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Aim|Animation", meta = (ClampMin = "0.0"))
	float AimBlendOutTime{0.2f};

private:

	void StartAimPoseMontage(AActor* AvatarActor);
	void StopAimPoseMontage(AActor* AvatarActor);

	TWeakObjectPtr<AT_ShooterAIController> CachedController;
	TWeakObjectPtr<UAnimMontage> ActiveAimPoseMontage;
};
