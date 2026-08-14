// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "Delegates/Delegate.h"
#include "GameplayTagContainer.h"
#include "T_GuardShoot.generated.h"

class AT_GuardCharacter;
class AT_PlayerProjectile;
class UAbilityTask_PlayMontageAndWait;
class UAbilitySystemComponent;
class UAnimMontage;
class UGameplayEffect;
class UT_ProjectileShooterComponent;

/**
 * Guard 射击能力：服务器权威，从枪口朝目标瞄准点发射，死亡/受击/换弹时中断。
 */
UCLASS()
class GASTESTDEMO1_API UT_GuardShoot : public UT_GameplayAbility
{
	GENERATED_BODY()

public:

	UT_GuardShoot();

	// 激活要求/持有/阻挡标签（供测试与蓝图检查）
	const FGameplayTagContainer& GetActivationRequiredTags() const { return ActivationRequiredTags; }
	const FGameplayTagContainer& GetActivationOwnedTags() const { return ActivationOwnedTags; }
	const FGameplayTagContainer& GetActivationBlockedTags() const { return ActivationBlockedTags; }

	// 判断标签是否属于射击中断标签（死亡/受击/换弹），供测试复用
	static bool IsInterruptingTag(const FGameplayTag& Tag);
	static float GetSpreadHalfAngle(float Distance);

	// 预热首次开火的蒙太奇、Niagara 与投射物，避免进入 Combat 第一枪卡顿
	static void PrimeShootPresentation(AT_GuardCharacter* Guard);

protected:

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:

	bool ExecuteShot(AActor* Target);

	void OnInterruptTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Shoot|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Shoot|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Shoot|Damage")
	float Damage{10.f};

	// 瞄准点相对目标脚底的高度偏移（约胸部；勿再叠加到胶囊中心上）
	UPROPERTY(EditDefaultsOnly, Category = "Guard|Shoot|Damage")
	float AimHeightOffset{100.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Shoot|Projectile")
	TSubclassOf<AT_PlayerProjectile> ProjectileClass;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	FDelegateHandle ReloadTagHandle;
	FDelegateHandle HitReactTagHandle;
	FDelegateHandle DeadTagHandle;
	FGameplayTag DeadTag;
	bool bShotFired{false};
};
