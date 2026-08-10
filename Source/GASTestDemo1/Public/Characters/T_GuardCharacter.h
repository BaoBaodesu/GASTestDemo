// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/T_EnemyCharacter.h"
#include "GameplayTagContainer.h"
#include "T_GuardCharacter.generated.h"

class UAnimMontage;
class UT_ProjectileShooterComponent;
class USkeletalMeshComponent;

/**
 * 人形持枪敌人：默认武器由服务器生成并挂到指定 socket，只读暴露武器与射击组件。
 * 死亡后播放死亡蒙太奇，结束后销毁，不再重生。
 */
UCLASS()
class GASTESTDEMO1_API AT_GuardCharacter : public AT_EnemyCharacter
{
	GENERATED_BODY()

public:

	AT_GuardCharacter();

	// 只读访问当前武器 Actor
	UFUNCTION(BlueprintPure, Category = "Guard|Weapon")
	AActor* GetWeaponActor() const { return WeaponActor; }

	// 只读访问当前武器网格
	UFUNCTION(BlueprintPure, Category = "Guard|Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	// 只读访问当前武器投射物射击组件
	UFUNCTION(BlueprintPure, Category = "Guard|Weapon")
	UT_ProjectileShooterComponent* GetProjectileShooterComponent() const { return ProjectileShooterComponent; }

	// 武器是否生成并配置完成
	UFUNCTION(BlueprintPure, Category = "Guard|Weapon")
	bool IsWeaponReady() const;

	// 只读访问默认武器类配置（用于验证必填配置）
	UFUNCTION(BlueprintPure, Category = "Guard|Weapon")
	TSubclassOf<AActor> GetDefaultWeaponClass() const { return DefaultWeaponClass; }

	// 只读访问武器挂点配置
	UFUNCTION(BlueprintPure, Category = "Guard|Weapon")
	FName GetWeaponAttachSocketName() const { return WeaponAttachSocketName; }

	// 清理卡住的受击/死亡阻挡标签，避免永久挡住瞄准与射击
	void ClearStaleBlockingTags();

	// 将 ASC Aiming 标签同步到 ABP_Shooter 的 IsAiming（该 ABP Cast BP_ShooterCharacter 失败时不会自行更新）
	void SyncAnimIsAiming();

	// 非致命受击表现：播放受击蒙太奇并短暂挂上 HitReact 标签以打断射击
	void PlayHitReactPresentation(AActor* InstigatorActor = nullptr);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void HandleDeath() override;
	virtual void HandleRespawn() override;
	virtual void ResetAttributes() override;

private:

	// 服务器端生成并挂载默认武器，失败时输出明确错误并停止 AI
	void SpawnDefaultWeapon();

	void StopGuardAI();

	void RegisterHitReactTagWatch();
	void UnregisterHitReactTagWatch();
	void RegisterAimingTagWatch();
	void UnregisterAimingTagWatch();

	UFUNCTION()
	void OnHitReactTagChanged(const FGameplayTag Tag, int32 NewCount);

	UFUNCTION()
	void OnAimingTagChanged(const FGameplayTag Tag, int32 NewCount);

	void ClearStaleHitReact();

	void PrepareForDeathPresentation();
	void PlayFallbackDeathMontage();
	void ScheduleDestroyAfterDeath(float DelaySeconds);
	void DestroyGuardAfterDeath();
	void ClearHitReactPresentation();

	UFUNCTION()
	void OnFallbackDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnHitReactMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Weapon")
	TSubclassOf<AActor> DefaultWeaponClass;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Weapon")
	FName WeaponAttachSocketName{TEXT("hand_rPistol")};

	// 死亡表现蒙太奇（默认 AM_Death1，与 Mannequin/ABP_Shooter 兼容）
	UPROPERTY(EditDefaultsOnly, Category = "Guard|Death")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 受击表现蒙太奇（敌人专用）
	UPROPERTY(EditDefaultsOnly, Category = "Guard|Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	// 死亡动画播放完毕后再延迟销毁的时间
	UPROPERTY(EditDefaultsOnly, Category = "Guard|Death", meta = (ClampMin = "0.0"))
	float DeathDestroyDelayAfterAnim{0.2f};

	// HitReact 标签未正常移除时的兜底清理时间
	UPROPERTY(EditDefaultsOnly, Category = "Guard|Combat")
	float HitReactStaleTimeout{2.f};

	UPROPERTY()
	TObjectPtr<AActor> WeaponActor;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> WeaponMesh;

	UPROPERTY()
	TObjectPtr<UT_ProjectileShooterComponent> ProjectileShooterComponent;

	FDelegateHandle HitReactTagWatchHandle;
	FDelegateHandle AimingTagWatchHandle;
	FTimerHandle HitReactStaleTimerHandle;
	FTimerHandle DeathDestroyTimerHandle;
	FOnMontageEnded FallbackDeathMontageEndedDelegate;
	FOnMontageEnded HitReactMontageEndedDelegate;

	bool bDeathStarted{false};
	bool bDeathDestroyed{false};
};
