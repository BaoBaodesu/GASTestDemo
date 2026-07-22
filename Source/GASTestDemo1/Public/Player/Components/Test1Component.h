// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Test1Component.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UCapsuleComponent;
class USkeletalMeshComponent;
class UMotionWarpingComponent;
class UAnimMontage;

/**
 * 测试用翻越类型
 */
UENUM(BlueprintType)
enum class ETest1TraversalType : uint8
{
	None,
	Vault,
	Climb
};

/**
 * 本次检测得到的翻越数据
 */
struct FTest1TraversalData
{
	ETest1TraversalType Type = ETest1TraversalType::None;

	FVector WallLocation = FVector::ZeroVector;
	FVector WallNormal = FVector::ZeroVector;

	FVector StartLocation = FVector::ZeroVector;
	FVector MiddleLocation = FVector::ZeroVector;
	FVector LandLocation = FVector::ZeroVector;

	FRotator TargetRotation = FRotator::ZeroRotator;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UTest1Component : public UActorComponent
{
	GENERATED_BODY()

public:
	UTest1Component();

	/**
	 * 尝试执行翻越或爬墙
	 * V键只需要调用这个函数
	 */
	UFUNCTION(BlueprintCallable, Category="Test1|Traversal")
	bool TryTraversal();

	UFUNCTION(BlueprintPure, Category="Test1|Traversal")
	bool IsTraversing() const { return bIsTraversing; }

protected:
	virtual void BeginPlay() override;

private:
	/** 缓存角色需要使用的组件 */
	bool CacheOwnerComponents();

	/** 检测角色前方墙体 */
	bool DetectWall(FHitResult& OutWallHit) const;

	/** 根据墙体检测顶部、厚度和落点 */
	bool BuildTraversalData(
		const FHitResult& WallHit,
		FTest1TraversalData& OutTraversalData) const;

	/** 从指定位置向下进行球形检测 */
	bool TraceDownAtLocation(
		const FVector& Location,
		float StartZ,
		float EndZ,
		FHitResult& OutHit) const;

	/** 检测目标位置是否能容纳角色胶囊体 */
	bool HasRoomAtLocation(const FVector& Location) const;

	/** 设置三个 Motion Warping Target */
	void SetupWarpTargets(const FTest1TraversalData& TraversalData);

	/** 播放对应翻越动画 */
	bool StartTraversal(const FTest1TraversalData& TraversalData);

	/** Montage 结束时恢复角色状态 */
	void OnTraversalMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** 恢复碰撞和移动模式 */
	void FinishTraversal();

	/** 绘制三个目标点 */
	void DrawTraversalData(const FTest1TraversalData& TraversalData) const;

private:
	/*-------------------- 动画 --------------------*/

	/** 矮墙翻越动画 */
	UPROPERTY(EditAnywhere, Category="Test1|Animation")
	TObjectPtr<UAnimMontage> VaultMontage;

	/** 高墙爬升动画 */
	UPROPERTY(EditAnywhere, Category="Test1|Animation")
	TObjectPtr<UAnimMontage> ClimbMontage;

	UPROPERTY(EditAnywhere, Category="Test1|Animation", meta=(ClampMin="0.1"))
	float MontagePlayRate = 1.0f;

	/*---------------- Motion Warping ----------------*/

	/**
	 * 名称必须和 Montage 中的 Motion Warping Notify 一致
	 */
	UPROPERTY(EditAnywhere, Category="Test1|Motion Warping")
	FName StartWarpTargetName = TEXT("VaultStart");

	UPROPERTY(EditAnywhere, Category="Test1|Motion Warping")
	FName MiddleWarpTargetName = TEXT("VaultMiddle");

	UPROPERTY(EditAnywhere, Category="Test1|Motion Warping")
	FName LandWarpTargetName = TEXT("VaultLand");

	/**
	 * Start目标距离墙面的额外距离
	 */
	UPROPERTY(EditAnywhere, Category="Test1|Motion Warping")
	float WarpStartOffset = 2.0f;

	/*-------------------- 墙体检测 --------------------*/

	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	/** 前方检测的高度，对应蓝图中的 Z + 55 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float WallTraceHeight = 55.0f;

	/** 前方检测距离，对应蓝图中的 Forward * 70 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float WallTraceDistance = 70.0f;

	/** 球形检测半径 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float TraceRadius = 5.0f;

	/**
	 * 墙面法线Z值不能太大，
	 * 避免把地面识别成墙面
	 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float MaxWallNormalZ = 0.3f;

	/*-------------------- 顶部检测 --------------------*/

	/** 从墙面向墙内偏移 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float TopForwardOffset = 10.0f;

	/** 顶部检测的额外起始高度 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float TopTraceExtraHeight = 50.0f;

	/** 认为是可站立平面的最小法线Z */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float MinTopNormalZ = 0.7f;

	/** 连续顶部之间允许的高度误差 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float SurfaceHeightTolerance = 20.0f;

	/*-------------------- 高度判断 --------------------*/

	/** 低于这个高度不执行翻越 */
	UPROPERTY(EditAnywhere, Category="Test1|Traversal")
	float MinTraversalHeight = 50.0f;

	/** 低于这个高度时优先判断为Vault */
	UPROPERTY(EditAnywhere, Category="Test1|Traversal")
	float MaxVaultHeight = 80.0f;

	/** 允许爬升的最大高度 */
	UPROPERTY(EditAnywhere, Category="Test1|Traversal")
	float MaxClimbHeight = 250.0f;

	/*-------------------- 厚度与落点 --------------------*/

	/** 每次向墙后方增加的检测距离 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float ForwardProbeStep = 30.0f;

	/** 最远检测距离 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float MaxForwardProbeDistance = 250.0f;

	/** 障碍物厚度低于这个值时才允许Vault */
	UPROPERTY(EditAnywhere, Category="Test1|Traversal")
	float MaxVaultDepth = 120.0f;

	/** 离开障碍边缘后继续向前寻找落点的距离 */
	UPROPERTY(EditAnywhere, Category="Test1|Traversal")
	float VaultLandingForwardOffset = 40.0f;

	/** Vault落点需要比障碍物顶部至少低多少 */
	UPROPERTY(EditAnywhere, Category="Test1|Traversal")
	float MinLandingDrop = 30.0f;

	/** 向下寻找落点的距离 */
	UPROPERTY(EditAnywhere, Category="Test1|Trace")
	float LandingTraceDepth = 300.0f;

	/** Climb完成后角色站在墙顶内部的位置 */
	UPROPERTY(EditAnywhere, Category="Test1|Traversal")
	float ClimbLandForwardDistance = 60.0f;

	/*-------------------- 调试 --------------------*/

	UPROPERTY(EditAnywhere, Category="Test1|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, Category="Test1|Debug")
	float DebugDrawTime = 5.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category="Test1|Traversal",
		meta=(AllowPrivateAccess="true"))
	bool bIsTraversing = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly,
		Category="Test1|Traversal",
		meta=(AllowPrivateAccess="true"))
	ETest1TraversalType CurrentTraversalType =
		ETest1TraversalType::None;

	/*-------------------- 缓存组件 --------------------*/

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY()
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY()
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY()
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	/** 保存执行前的胶囊体碰撞状态 */
	ECollisionEnabled::Type PreviousCapsuleCollision =
		ECollisionEnabled::QueryAndPhysics;
};