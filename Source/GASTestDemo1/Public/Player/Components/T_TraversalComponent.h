// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T_TraversalComponent.generated.h"

class UCapsuleComponent;
class T_TraversalVault;
class T_TraversalMantle;
class T_TraversalClimb;

/**
 * 遍历动作类型
 */
UENUM(BlueprintType)
enum class ETraversalActionType : uint8
{
	None	UMETA(DisplayName = "None"),

	// 翻越高度与厚度都在 Vault 范围内的障碍物
	Vault	UMETA(DisplayName = "Vault"),

	// 从地面攀上较高平台
	Climb	UMETA(DisplayName = "Climb"),

	// 双手撑住中等高度的平台并上去
	Mantle	UMETA(DisplayName = "Mantle")
};

UENUM(BlueprintType)
enum class ETraversalEntryState : uint8
{
	Stand	UMETA(DisplayName = "Stand"),
	Walk	UMETA(DisplayName = "Walk"),
	Run		UMETA(DisplayName = "Run")
};

/**
 * 一次遍历检测得到的全部数据
 */
USTRUCT(BlueprintType)
struct FTraversalCheckResult
{
	GENERATED_BODY()

	// 是否找到可执行的遍历动作
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bTraversalFound = false;

	// 最终选择的动作类型
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	ETraversalActionType ActionType = ETraversalActionType::None;

	// 是否检测到墙面
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bHasWall = false;

	// 是否检测到墙顶
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bHasTopSurface = false;

	// 是否检测到障碍物后沿
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bHasFarEdge = false;

	// 墙顶是否有足够空间容纳角色
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bHasTopStandingSpace = false;

	// 障碍物后方是否存在安全落点
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	bool bHasVaultLandingSpace = false;

	// 从角色脚底到墙顶的高度
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	float ObstacleHeight = 0.f;

	// 障碍物沿前进方向的厚度
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	float ObstacleDepth = 0.f;

	// 墙面命中位置
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector WallLocation = FVector::ZeroVector;

	// 墙面法线，通常指向角色
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector WallNormal = FVector::ZeroVector;

	// 墙顶位置
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector TopLocation = FVector::ZeroVector;

	// 障碍物后沿位置
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector FarEdgeLocation = FVector::ZeroVector;

	// 动作结束后的站立位置，表示胶囊体中心
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FVector LandingLocation = FVector::ZeroVector;

	// 角色应当面向墙壁的旋转
	UPROPERTY(BlueprintReadOnly, Category = "Traversal")
	FRotator TargetRotation = FRotator::ZeroRotator;

	// 接近墙面阶段的 Motion Warping Target
	UPROPERTY(BlueprintReadOnly, Category = "Traversal|Motion Warping")
	FTransform FrontLedgeWarpTarget = FTransform::Identity;

	// 接触墙顶或越过墙顶阶段的 Motion Warping Target
	UPROPERTY(BlueprintReadOnly, Category = "Traversal|Motion Warping")
	FTransform BackLedgeWarpTarget = FTransform::Identity;

};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_TraversalComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UT_TraversalComponent();

	// 跳跃与翻越共用入口，翻越未触发时由调用方执行普通跳跃
	bool Jump();

	// 检测角色前方是否存在可执行的遍历动作
	UFUNCTION(BlueprintCallable, Category = "Traversal")
	bool DetectTraversal(FTraversalCheckResult& OutResult);

	// 前方墙面检测
	bool DetectWall(FHitResult& OutWallHit) const;

	// 墙顶检测
	bool DetectTop(const FHitResult& WallHit, FHitResult& OutTopHit) const;

	// Vault 专用顶部检测，只在可翻越高度范围内寻找窗口下沿或矮墙顶部
	bool DetectVaultTop(const FHitResult& WallHit, FHitResult& OutTopHit) const;
	
	// 测量障碍物厚度，并尝试找到后沿
	bool MeasureObstacleDepth(
		const FHitResult& WallHit,
		const FHitResult& TopHit,
		float& OutObstacleDepth,
		FVector& OutFarEdgeLocation
	) const;

	// 检查障碍物后方落点
	bool FindLandingLocation(
		const FHitResult& WallHit,
		const FVector& FarEdgeLocation,
		FVector& OutLandingLocation
	) const;

	// 检查指定位置是否能容纳当前角色胶囊体
	bool IsCapsuleLocationClear(
		const FVector& CapsuleLocation,
		float ClearanceShrinkOverride = -1.f
	) const;

	// 在参考位置正下方寻找可供完整胶囊站立的位置，保留参考位置的水平坐标
	bool FindSafeStandingLocationBelow(
		const FVector& ReferenceLocation,
		float FloorClearance,
		FVector& OutStandingLocation
	) const;

	// 根据检测位置生成 Warp Target Transform
	void BuildWarpTargets(FTraversalCheckResult& Result) const;

protected:

	virtual void BeginPlay() override;

	void CacheOwnerComponents();

	// 检查障碍物顶部是否能站立
	bool FindTopStandingLocation(
		const FHitResult& WallHit,
		const FHitResult& TopHit,
		FVector& OutStandingLocation
	) const;

	// 获取角色脚底位置
	FVector GetCharacterFeetLocation() const;

	// 获取进入障碍物内部的水平方向
	FVector GetIntoObstacleDirection(const FVector& WallNormal) const;

	// 获取面向墙面的旋转
	FRotator GetFacingWallRotation(const FVector& WallNormal) const;

	// 调试绘制持续时间
	float GetDebugDuration() const;

	void DrawTraversalDebug(const FTraversalCheckResult& Result) const;

	friend class T_TraversalVault;
	friend class T_TraversalMantle;
	friend class T_TraversalClimb;
	
protected:
	/*
	 * 缓存组件
	 */
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCapsuleComponent> OwnerCapsuleComponent;

	/*
	 * 基础检测参数
	 */

	// 场景物体必须阻挡此检测通道
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	TEnumAsByte<ECollisionChannel> TraversalTraceChannel = ECC_Visibility;

	// 胶囊空间检测使用的通道
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection")
	TEnumAsByte<ECollisionChannel> CapsuleTestChannel = ECC_Pawn;

	// 从脚底向上多少高度进行墙面检测
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection",
		meta = (ClampMin = "0.0", Units = "cm"))
	float WallTraceHeight = 55.f;

	// 墙面检测距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection", meta = (ClampMin = "0.0", Units = "cm"))
	float WallTraceDistance = 120.f;

	// 墙面球形检测半径
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection", meta = (ClampMin = "1.0", Units = "cm"))
	float WallTraceRadius = 5.f;

	// 墙面法线 Z 超过此值时，认为它更接近斜坡而不是墙
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaximumWallNormalZ = 0.35f;

	// 墙面至少要在角色正前方
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Detection", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
	float MinimumWallFacingDot = 0.5f;
	
	/*
	 * 墙顶检测参数
	 */

	// 墙顶射线向墙内偏移的距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Top", meta = (ClampMin = "0.0", Units = "cm"))
	float TopTraceInset = 15.f;

	// 墙顶检测在最大抓取高度上方额外增加的高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Top", meta = (ClampMin = "0.0", Units = "cm"))
	float TopTraceExtraHeight = 20.f;

	// Vault 顶面检测在最大 Vault 高度上方额外增加的高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Top", meta = (ClampMin = "0.0", Units = "cm"))
	float VaultTopTraceExtraHeight = 20.f;

	// 墙顶表面的法线 Z 至少达到此值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Top", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumTopNormalZ = 0.7f;

	// 在墙顶内部多远的位置检测站立空间
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Top", meta = (ClampMin = "0.0", Units = "cm"))
	float TopStandingInset = 45.f;
	
	/*
	 * 厚度检测
	 */

	// 每隔多少距离进行一次顶部向下检测
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Depth",
		meta = (ClampMin = "1.0", Units = "cm"))
	float DepthTraceStep = 10.f;

	// 最大检测厚度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Depth",
		meta = (ClampMin = "1.0", Units = "cm"))
	float MaximumDepthToMeasure = 180.f;

	// 厚度检测射线在墙顶上方的高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Depth",
		meta = (ClampMin = "0.0", Units = "cm"))
	float DepthTraceHeight = 50.f;

	// 厚度检测射线向下的距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Depth",
		meta = (ClampMin = "1.0", Units = "cm"))
	float DepthTraceDepth = 130.f;

	// 连续顶部允许的最大高度差
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Depth",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumTopHeightDifference = 20.f;
	
	/*
	 * 动作高度范围
	 */

	// 低于该高度时不使用遍历动作
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Action",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MinimumTraversalHeight = 30.f;

	// Vault 最大高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Action",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumVaultHeight = 130.f;

	// 可以执行 Vault 的最大障碍物厚度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Action",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumVaultDepth = 100.f;

	// Mantle 最大高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Action",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumMantleHeight = 150.f;

	// 从地面直接执行 Climb 的最大高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Action",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MaximumClimbHeight = 200.f;

	/*
	 * 落点检测
	 */

	// 越过障碍物后，再向前检测落点的距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Landing",
		meta = (ClampMin = "0.0", Units = "cm"))
	float VaultLandingForwardOffset = 45.f;

	// 落点射线起点高于参考位置的距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Landing",
		meta = (ClampMin = "0.0", Units = "cm"))
	float LandingTraceHeight = 100.f;

	// 落点射线向下检测距离
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Landing",
		meta = (ClampMin = "1.0", Units = "cm"))
	float LandingTraceDepth = 400.f;

	// 胶囊重叠检测时缩小一点，避免边缘误判
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Landing",
		meta = (ClampMin = "0.0", Units = "cm"))
	float CapsuleClearanceShrink = 2.f;
	
	/*
	 * Warp Target 局部偏移：
	 * X = 面向墙壁方向
	 * Y = 角色右方向
	 * Z = 世界向上方向
	 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping")
	FVector FrontLedgeWarpOffset = FVector::ZeroVector;

	// Climb 动画中，抓住墙沿时根骨低于墙沿的高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping",
		meta = (ClampMin = "0.0", Units = "cm"))
	float ClimbRootToLedgeHeight = 132.f;

	// Mantle 动画中，接触墙沿时根骨低于墙沿的高度；默认使用中性值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping",
		meta = (ClampMin = "0.0", Units = "cm"))
	float MantleRootToLedgeHeight = 0.f;

	// Climb 根骨相对墙沿目标的水平微调，X 负方向远离墙体；Z 已弃用且不会参与计算
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping")
	FVector ClimbFrontLedgeWarpOffset = FVector(0.f, 0.f, 0.f);

	// Mantle 根骨相对墙沿目标的局部偏移，X 负方向远离墙体
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping")
	FVector MantleFrontLedgeWarpOffset = FVector(0.f, 0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Motion Warping")
	FVector BackLedgeWarpOffset = FVector::ZeroVector;


	/*
	 * 调试
	 */

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Debug")
	bool bDrawDebug = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal|Debug",
		meta = (ClampMin = "0.0"))
	float DebugDrawDuration = 3.f;
};
