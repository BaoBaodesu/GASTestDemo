// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/T_ShooterAIController.h"
#include "Characters/T_EnemyCharacter.h"
#include "GameplayTagContainer.h"
#include "T_GuardCharacter.generated.h"

class UAnimMontage;
class UT_ProjectileShooterComponent;
class USkeletalMeshComponent;
class UPrimitiveComponent;
class USoundBase;
class UT_AIAwarenessWidget;
class UWidgetComponent;

UENUM(BlueprintType)
enum class EGuardPatrolMode : uint8
{
	Stationary,
	PingPong,
	Loop
};

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
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

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

	// 将警觉状态同步到 AnimInstance 的 bAlwaysAiming / IsAiming（巡逻放下枪，警觉后举枪）
	void SyncAnimIsAiming();

	UAnimMontage* GetPrimedAimPoseMontage() const { return PrimedAimPoseMontage; }

	// 非致命受击表现：播放受击蒙太奇并短暂挂上 HitReact 标签以打断射击
	void PlayHitReactPresentation(AActor* InstigatorActor = nullptr);

	void SetGuardAIPresentation(float InAwareness, ETGuardAIState InAIState, bool bHasVisualContact);
	void SetCombatStrafeEnabled(bool bEnabled);
	void SetReturnMovementEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	float GetGuardAwareness() const { return GuardAwareness; }

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	ETGuardAIState GetGuardAIState() const { return GuardAIState; }

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	bool HasGuardVisualContact() const { return bGuardHasVisualContact; }

	// 本地点转世界缓存并重置巡逻索引
	void InitializePatrolRoute();

	// 若尚未缓存则初始化一次（Possess 可能早于 BeginPlay）
	void EnsurePatrolRouteInitialized();

	// Stationary，或 PingPong/Loop 有效路点不足 2 个时视为站岗
	bool IsStationaryPatrol() const;

	// 当前应写入黑板的巡逻目标（站岗为当前位置）
	FVector GetCurrentPatrolDestination() const;

	// 已到达当前点时推进 PingPong/Loop 索引（带距离校验）
	void AdvancePatrolPointIfNeeded(const FVector& PawnLocation);

	// 巡逻停留结束后强制切到下一路点（MoveTo 已成功，不再做距离校验）
	void AdvancePatrolPointAfterWait();

	// 恢复 Patrol 时吸附到最近路点
	void SnapPatrolIndexToNearest(const FVector& PawnLocation);

	UFUNCTION(BlueprintPure, Category = "Guard|Patrol")
	EGuardPatrolMode GetPatrolMode() const { return PatrolMode; }

	UFUNCTION(BlueprintPure, Category = "Guard|Patrol")
	int32 GetPatrolPointIndex() const { return PatrolPointIndex; }

	UFUNCTION(BlueprintPure, Category = "Guard|Patrol")
	int32 GetPatrolDirection() const { return PatrolDirection; }

	// 按配置掷一次停留时长（含随机偏差，不小于 0）
	UFUNCTION(BlueprintPure, Category = "Guard|Patrol")
	float GetRolledPatrolPointWaitDuration() const;

	// 纯逻辑：推进后的索引与方向，供单测与运行时共用
	static void ComputeNextPatrolIndex(
		EGuardPatrolMode Mode,
		int32 PointCount,
		int32 CurrentIndex,
		int32 CurrentDirection,
		int32& OutIndex,
		int32& OutDirection);

	static int32 FindNearestPatrolIndex(const TArray<FVector>& WorldPoints, const FVector& Location);

protected:

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void HandleDeath() override;
	virtual void HandleRespawn() override;
	virtual void ResetAttributes() override;
	virtual void ScheduleDeathDestroy(float DelaySeconds) override;
	virtual void NotifyHit(UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp,
		bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

	// 站岗 / 多点往返 / 绕圈；默认站岗
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard|Patrol")
	EGuardPatrolMode PatrolMode{EGuardPatrolMode::Stationary};

	// 相对角色根的本地点；视口 MakeEditWidget 可拖
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard|Patrol", meta = (MakeEditWidget))
	TArray<FVector> PatrolPoints;

	// 到每个巡逻点后的停留时间（秒）；站岗时也用于 Patrol 分支的原地等待
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard|Patrol", meta = (ClampMin = "0.0"))
	float PatrolPointWaitSeconds{1.f};

	// 停留时间随机波动（最终等待 = WaitSeconds ± Deviation）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Guard|Patrol", meta = (ClampMin = "0.0"))
	float PatrolPointWaitRandomDeviation{0.5f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Patrol", meta = (ClampMin = "1.0"))
	float PatrolPointAcceptanceRadius{120.f};

private:

	// 服务器端生成并挂载默认武器，失败时输出明确错误并停止 AI
	void SpawnDefaultWeapon();

	// 关闭默认武器碰撞与 Shape 线框显示（不影响射击逻辑）
	void DisableWeaponCollisionDisplay();

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
	void InitializeAwarenessWidget();
	void RefreshAwarenessWidget();
	void HandleAIStateChanged(ETGuardAIState PreviousState);
	void PrimeCombatPresentation();
	void ApplyWeaponVisibility(bool bShowWeapon);

	UFUNCTION()
	void OnRep_GuardAwareness();

	UFUNCTION()
	void OnRep_GuardAIState(ETGuardAIState PreviousState);

	UFUNCTION()
	void OnRep_GuardHasVisualContact();

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

	UPROPERTY(ReplicatedUsing = OnRep_GuardAwareness)
	float GuardAwareness{0.f};

	UPROPERTY(ReplicatedUsing = OnRep_GuardAIState)
	ETGuardAIState GuardAIState{ETGuardAIState::Patrol};

	UPROPERTY(ReplicatedUsing = OnRep_GuardHasVisualContact)
	bool bGuardHasVisualContact{false};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Awareness")
	TObjectPtr<USoundBase> AlertSound;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> AwarenessWidgetComponent;

	UPROPERTY(Transient)
	TObjectPtr<UT_AIAwarenessWidget> AwarenessWidget;

	UPROPERTY(Transient)
	TArray<FVector> CachedPatrolWorldPoints;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PrimedAimPoseMontage;

	int32 PatrolPointIndex{0};
	int32 PatrolDirection{1};
	bool bPatrolRouteInitialized{false};

	FDelegateHandle HitReactTagWatchHandle;
	FDelegateHandle AimingTagWatchHandle;
	FTimerHandle HitReactStaleTimerHandle;
	FTimerHandle DeathDestroyTimerHandle;
	FOnMontageEnded FallbackDeathMontageEndedDelegate;
	FOnMontageEnded HitReactMontageEndedDelegate;

	bool bDeathStarted{false};
	bool bDeathDestroyed{false};
	bool bLastShouldAim{false};
	bool bWeaponVisibilityApplied{false};
	bool bWeaponVisible{false};
	bool bAimPosePrimeAttempted{false};
	int32 CombatPresentationPrimeFrames{0};
	bool bCombatStrafeEnabled{false};
	bool bReturnMovementEnabled{false};
	bool bSavedOrientRotationToMovement{true};
	bool bSavedUseControllerDesiredRotation{false};
	bool bSavedUseControllerRotationYaw{false};
	FRotator SavedRotationRate{0.f, 360.f, 0.f};
	bool bReturnSavedOrientRotationToMovement{true};
	bool bReturnSavedUseControllerDesiredRotation{false};
	bool bReturnSavedUseControllerRotationYaw{false};
	FRotator ReturnSavedRotationRate{0.f, 360.f, 0.f};
};
