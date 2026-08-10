#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/T_GameplayAbility.h"
#include "T_Shoot.generated.h"

class AT_PlayerProjectile;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;
class UAnimMontage;
class UGameplayEffect;
class USkeletalMeshComponent;
class USoundBase;
class UT_AimingComponent;
class UT_ProjectileShooterComponent;

UCLASS()
class GASTESTDEMO1_API UT_Shoot : public UT_GameplayAbility
{
	GENERATED_BODY()

public:
	UT_Shoot();

	// 激活阻挡标签（供测试与蓝图检查）
	const FGameplayTagContainer& GetActivationBlockedTags() const { return ActivationBlockedTags; }

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	void ExecuteShot();
	void FinishAbility(bool bWasCancelled);

	UFUNCTION()
	void OnFireEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageCancelled();

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Animation")
	TObjectPtr<UAnimMontage> FireMontage;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Damage")
	float Damage{10.f};

	// 命中敌人头部（head 骨骼附近）时的伤害倍率
	UPROPERTY(EditDefaultsOnly, Category="Shoot|Damage")
	float HeadshotDamageMultiplier{2.f};

	// 用于判定头部命中的敌人骨骼名
	UPROPERTY(EditDefaultsOnly, Category="Shoot|Damage")
	FName HeadBoneName{TEXT("head")};

	// 命中点距头部骨骼中心超过该距离则不视为头部命中（单位 cm）
	UPROPERTY(EditDefaultsOnly, Category="Shoot|Damage")
	float HeadshotToleranceRadius{40.f};

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Projectile")
	TSubclassOf<AT_PlayerProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category="Shoot|Sound")
	TObjectPtr<USoundBase> DryFireSound;

	UPROPERTY(Transient)
	TObjectPtr<UT_AimingComponent> AimingComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY(Transient)
	TObjectPtr<UT_ProjectileShooterComponent> ProjectileShooterComponent;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> FireEventTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

	bool bShotExecuted{false};
};
