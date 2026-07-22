// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/Components/Test1Component.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MotionWarpingComponent.h"

UTest1Component::UTest1Component()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTest1Component::BeginPlay()
{
	Super::BeginPlay();

	CacheOwnerComponents();
}

bool UTest1Component::CacheOwnerComponents()
{
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!IsValid(OwnerCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("Test1Component只能添加到Character上"));

		return false;
	}

	CharacterMovement = OwnerCharacter->GetCharacterMovement();
	CapsuleComponent = OwnerCharacter->GetCapsuleComponent();
	MeshComponent = OwnerCharacter->GetMesh();

	if (!IsValid(CharacterMovement) ||
		!IsValid(CapsuleComponent) ||
		!IsValid(MeshComponent))
	{
		return false;
	}

	// 优先寻找角色身上已有的Motion Warping组件
	MotionWarpingComponent =
		OwnerCharacter->FindComponentByClass<UMotionWarpingComponent>();

	// 测试组件没有找到时，运行时自动创建一个
	if (!IsValid(MotionWarpingComponent))
	{
		MotionWarpingComponent =
			NewObject<UMotionWarpingComponent>(
				OwnerCharacter,
				TEXT("Test1MotionWarpingComponent"));

		if (IsValid(MotionWarpingComponent))
		{
			OwnerCharacter->AddInstanceComponent(
				MotionWarpingComponent);

			MotionWarpingComponent->RegisterComponent();
		}
	}

	return IsValid(MotionWarpingComponent);
}

bool UTest1Component::TryTraversal()
{
	if (bIsTraversing)
	{
		return false;
	}

	if (!IsValid(OwnerCharacter) && !CacheOwnerComponents())
	{
		return false;
	}

	// 当前版本只允许在地面执行测试
	if (CharacterMovement->IsFalling())
	{
		return false;
	}

	FHitResult WallHit;

	if (!DetectWall(WallHit))
	{
		return false;
	}

	FTest1TraversalData TraversalData;

	if (!BuildTraversalData(WallHit, TraversalData))
	{
		return false;
	}

	DrawTraversalData(TraversalData);

	return StartTraversal(TraversalData);
}

bool UTest1Component::DetectWall(FHitResult& OutWallHit) const
{
	if (!IsValid(OwnerCharacter) || !GetWorld())
	{
		return false;
	}

	const FVector Start =
		OwnerCharacter->GetActorLocation() +
		FVector::UpVector * WallTraceHeight;

	const FVector End =
		Start +
		OwnerCharacter->GetActorForwardVector() *
		WallTraceDistance;

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(Test1WallTrace),
		false);

	QueryParams.AddIgnoredActor(OwnerCharacter);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutWallHit,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			Start,
			End,
			bHit ? FColor::Green : FColor::Red,
			false,
			DebugDrawTime,
			0,
			2.0f);

		DrawDebugSphere(
			GetWorld(),
			bHit ? OutWallHit.ImpactPoint : End,
			TraceRadius,
			12,
			bHit ? FColor::Green : FColor::Red,
			false,
			DebugDrawTime);
	}

	if (!bHit)
	{
		return false;
	}

	// 法线Z过大时，命中的可能是地面或斜坡
	return FMath::Abs(OutWallHit.ImpactNormal.Z) <=
		MaxWallNormalZ;
}

bool UTest1Component::BuildTraversalData(
	const FHitResult& WallHit,
	FTest1TraversalData& OutTraversalData) const
{
	if (!IsValid(OwnerCharacter) ||
		!IsValid(CapsuleComponent))
	{
		return false;
	}

	const float CapsuleHalfHeight =
		CapsuleComponent->GetScaledCapsuleHalfHeight();

	const float CapsuleRadius =
		CapsuleComponent->GetScaledCapsuleRadius();

	const float FeetZ =
		OwnerCharacter->GetActorLocation().Z -
		CapsuleHalfHeight;

	// 墙面法线朝向角色，取反后就是角色进入墙体的方向
	const FVector WallNormal =
		WallHit.ImpactNormal.GetSafeNormal2D();

	const FVector FacingDirection = -WallNormal;

	/*
	 * 从墙面稍微向内偏移，然后从上向下检测墙顶
	 */
	const FVector TopProbeLocation =
		WallHit.ImpactPoint +
		FacingDirection * TopForwardOffset;

	FHitResult TopHit;

	if (!TraceDownAtLocation(
		TopProbeLocation,
		FeetZ + MaxClimbHeight + TopTraceExtraHeight,
		FeetZ - 20.0f,
		TopHit))
	{
		return false;
	}

	// 墙顶必须接近水平
	if (TopHit.ImpactNormal.Z < MinTopNormalZ)
	{
		return false;
	}

	const float WallHeight =
		TopHit.ImpactPoint.Z - FeetZ;

	if (WallHeight < MinTraversalHeight ||
		WallHeight > MaxClimbHeight)
	{
		return false;
	}

	FHitResult LandingHit;

	float ObstacleDepth = TopForwardOffset;
	float EdgeDistance = 0.0f;

	bool bFoundEdge = false;
	bool bFoundLanding = false;

	/*
	 * 沿墙后方逐步向下检测：
	 * 连续命中同样高度，表示障碍物还没有结束。
	 * 高度明显下降或者不再命中，表示到达障碍边缘。
	 */
	for (float Distance =
			 TopForwardOffset + ForwardProbeStep;
		 Distance <= MaxForwardProbeDistance;
		 Distance += ForwardProbeStep)
	{
		const FVector ProbeLocation =
			WallHit.ImpactPoint +
			FacingDirection * Distance;

		FHitResult ProbeHit;

		const bool bProbeHit = TraceDownAtLocation(
			ProbeLocation,
			TopHit.ImpactPoint.Z + TopTraceExtraHeight,
			FeetZ - LandingTraceDepth,
			ProbeHit);

		const bool bSameTopSurface =
			bProbeHit &&
			ProbeHit.ImpactNormal.Z >= MinTopNormalZ &&
			FMath::Abs(
				ProbeHit.ImpactPoint.Z -
				TopHit.ImpactPoint.Z) <=
			SurfaceHeightTolerance;

		if (!bFoundEdge && bSameTopSurface)
		{
			ObstacleDepth = Distance;
			continue;
		}

		if (!bFoundEdge)
		{
			bFoundEdge = true;
			EdgeDistance = Distance;
		}

		/*
		 * 离开障碍边缘一定距离后，
		 * 找到低于墙顶的水平面作为Vault落点。
		 */
		if (bProbeHit &&
			ProbeHit.ImpactNormal.Z >= MinTopNormalZ &&
			Distance >=
				EdgeDistance + VaultLandingForwardOffset &&
			ProbeHit.ImpactPoint.Z <=
				TopHit.ImpactPoint.Z - MinLandingDrop)
		{
			LandingHit = ProbeHit;
			bFoundLanding = true;
			break;
		}
	}

	const bool bCanVault =
		WallHeight <= MaxVaultHeight &&
		bFoundEdge &&
		bFoundLanding &&
		ObstacleDepth <= MaxVaultDepth;

	OutTraversalData.Type =
		bCanVault
			? ETest1TraversalType::Vault
			: ETest1TraversalType::Climb;

	OutTraversalData.WallLocation =
		WallHit.ImpactPoint;

	OutTraversalData.WallNormal =
		WallNormal;

	// 角色最终朝向墙体
	OutTraversalData.TargetRotation =
		FacingDirection.Rotation();

	OutTraversalData.TargetRotation.Pitch = 0.0f;
	OutTraversalData.TargetRotation.Roll = 0.0f;

	/*
	 * Start目标位于墙面外侧，
	 * 距离约等于胶囊体半径。
	 */
	OutTraversalData.StartLocation =
		WallHit.ImpactPoint +
		WallNormal *
		(CapsuleRadius + WarpStartOffset);

	OutTraversalData.StartLocation.Z =
		OwnerCharacter->GetActorLocation().Z;

	/*
	 * 蓝图中对墙顶位置增加了约100的Z值。
	 * 这里直接使用胶囊体半高，使目标表示角色中心位置。
	 */
	OutTraversalData.MiddleLocation =
		TopHit.ImpactPoint +
		FVector::UpVector * CapsuleHalfHeight;

	if (OutTraversalData.Type ==
		ETest1TraversalType::Vault)
	{
		OutTraversalData.LandLocation =
			LandingHit.ImpactPoint +
			FVector::UpVector * CapsuleHalfHeight;
	}
	else
	{
		/*
		 * Climb需要检测墙顶内部是否仍然有平台。
		 */
		const FVector ClimbLandProbe =
			WallHit.ImpactPoint +
			FacingDirection *
			ClimbLandForwardDistance;

		FHitResult ClimbLandHit;

		if (!TraceDownAtLocation(
			ClimbLandProbe,
			TopHit.ImpactPoint.Z +
				TopTraceExtraHeight,
			FeetZ - 20.0f,
			ClimbLandHit))
		{
			return false;
		}

		if (ClimbLandHit.ImpactNormal.Z <
				MinTopNormalZ ||
			FMath::Abs(
				ClimbLandHit.ImpactPoint.Z -
				TopHit.ImpactPoint.Z) >
				SurfaceHeightTolerance)
		{
			return false;
		}

		OutTraversalData.LandLocation =
			ClimbLandHit.ImpactPoint +
			FVector::UpVector *
			CapsuleHalfHeight;
	}

	// 落点位置必须可以容纳角色
	return HasRoomAtLocation(
		OutTraversalData.LandLocation);
}

bool UTest1Component::TraceDownAtLocation(
	const FVector& Location,
	float StartZ,
	float EndZ,
	FHitResult& OutHit) const
{
	if (!GetWorld())
	{
		return false;
	}

	const FVector Start(
		Location.X,
		Location.Y,
		StartZ);

	const FVector End(
		Location.X,
		Location.Y,
		EndZ);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(Test1TopTrace),
		false);

	QueryParams.AddIgnoredActor(OwnerCharacter);

	const bool bHit = GetWorld()->SweepSingleByChannel(
		OutHit,
		Start,
		End,
		FQuat::Identity,
		TraceChannel,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			Start,
			End,
			bHit ? FColor::Green : FColor::Red,
			false,
			DebugDrawTime,
			0,
			1.5f);

		if (bHit)
		{
			DrawDebugSphere(
				GetWorld(),
				OutHit.ImpactPoint,
				TraceRadius,
				12,
				FColor::Yellow,
				false,
				DebugDrawTime);
		}
	}

	return bHit;
}

bool UTest1Component::HasRoomAtLocation(
	const FVector& Location) const
{
	if (!GetWorld() ||
		!IsValid(CapsuleComponent))
	{
		return false;
	}

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(Test1ClearanceTrace),
		false);

	QueryParams.AddIgnoredActor(OwnerCharacter);

	/*
	 * 稍微缩小检测胶囊，
	 * 避免刚好接触地面时被判断为重叠。
	 */
	const float CapsuleRadius =
		CapsuleComponent->GetScaledCapsuleRadius() *
		0.9f;

	const float CapsuleHalfHeight =
		CapsuleComponent->GetScaledCapsuleHalfHeight() *
		0.9f;

	const bool bBlocked =
		GetWorld()->OverlapBlockingTestByChannel(
			Location + FVector::UpVector * 2.0f,
			FQuat::Identity,
			ECC_Pawn,
			FCollisionShape::MakeCapsule(
				CapsuleRadius,
				CapsuleHalfHeight),
			QueryParams);

	if (bDrawDebug)
	{
		DrawDebugCapsule(
			GetWorld(),
			Location + FVector::UpVector * 2.0f,
			CapsuleHalfHeight,
			CapsuleRadius,
			FQuat::Identity,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			DebugDrawTime);
	}

	return !bBlocked;
}

void UTest1Component::SetupWarpTargets(
	const FTest1TraversalData& TraversalData)
{
	if (!IsValid(MotionWarpingComponent))
	{
		return;
	}

	MotionWarpingComponent->
		AddOrUpdateWarpTargetFromLocationAndRotation(
			StartWarpTargetName,
			TraversalData.StartLocation,
			TraversalData.TargetRotation);

	MotionWarpingComponent->
		AddOrUpdateWarpTargetFromLocationAndRotation(
			MiddleWarpTargetName,
			TraversalData.MiddleLocation,
			TraversalData.TargetRotation);

	MotionWarpingComponent->
		AddOrUpdateWarpTargetFromLocationAndRotation(
			LandWarpTargetName,
			TraversalData.LandLocation,
			TraversalData.TargetRotation);
}

bool UTest1Component::StartTraversal(
	const FTest1TraversalData& TraversalData)
{
	if (!IsValid(MeshComponent) ||
		!IsValid(CharacterMovement) ||
		!IsValid(CapsuleComponent) ||
		!IsValid(MotionWarpingComponent))
	{
		return false;
	}

	UAnimMontage* MontageToPlay =
		TraversalData.Type ==
			ETest1TraversalType::Vault
			? VaultMontage
			: ClimbMontage;

	if (!IsValid(MontageToPlay))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("Test1Component没有设置对应的Montage"));

		return false;
	}

	UAnimInstance* AnimInstance =
		MeshComponent->GetAnimInstance();

	if (!IsValid(AnimInstance))
	{
		return false;
	}

	SetupWarpTargets(TraversalData);

	PreviousCapsuleCollision =
		CapsuleComponent->GetCollisionEnabled();

	bIsTraversing = true;
	CurrentTraversalType = TraversalData.Type;

	// 停止原来的移动速度
	CharacterMovement->StopMovementImmediately();

	// 翻越过程中暂时关闭胶囊体碰撞
	CapsuleComponent->SetCollisionEnabled(
		ECollisionEnabled::NoCollision);

	// 避免重力影响Root Motion
	CharacterMovement->SetMovementMode(
		MOVE_Flying);

	const float MontageDuration =
		AnimInstance->Montage_Play(
			MontageToPlay,
			MontagePlayRate);

	if (MontageDuration <= 0.0f)
	{
		FinishTraversal();
		return false;
	}

	/*
	 * Montage结束和被打断都会进入回调，
	 * 比蓝图中使用固定Delay更加稳定。
	 */
	FOnMontageEnded MontageEndedDelegate;

	MontageEndedDelegate.BindUObject(
		this,
		&ThisClass::OnTraversalMontageEnded);

	AnimInstance->Montage_SetEndDelegate(
		MontageEndedDelegate,
		MontageToPlay);

	return true;
}

void UTest1Component::OnTraversalMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	FinishTraversal();
}

void UTest1Component::FinishTraversal()
{
	if (IsValid(CapsuleComponent))
	{
		CapsuleComponent->SetCollisionEnabled(
			PreviousCapsuleCollision);
	}

	if (IsValid(CharacterMovement))
	{
		/*
		 * 与蓝图逻辑一致：
		 * 动画结束后先进入Falling，
		 * CharacterMovement会自动落到地面。
		 */
		CharacterMovement->SetMovementMode(
			MOVE_Falling);
	}

	if (IsValid(MotionWarpingComponent))
	{
		MotionWarpingComponent->RemoveWarpTarget(
			StartWarpTargetName);

		MotionWarpingComponent->RemoveWarpTarget(
			MiddleWarpTargetName);

		MotionWarpingComponent->RemoveWarpTarget(
			LandWarpTargetName);
	}

	bIsTraversing = false;
	CurrentTraversalType =
		ETest1TraversalType::None;
}

void UTest1Component::DrawTraversalData(
	const FTest1TraversalData& TraversalData) const
{
	if (!bDrawDebug || !GetWorld())
	{
		return;
	}

	// 红色：开始贴近墙面的位置
	DrawDebugSphere(
		GetWorld(),
		TraversalData.StartLocation,
		10.0f,
		12,
		FColor::Red,
		false,
		DebugDrawTime);

	// 黄色：墙体顶部位置
	DrawDebugSphere(
		GetWorld(),
		TraversalData.MiddleLocation,
		10.0f,
		12,
		FColor::Yellow,
		false,
		DebugDrawTime);

	// 绿色：最终落点
	DrawDebugSphere(
		GetWorld(),
		TraversalData.LandLocation,
		10.0f,
		12,
		FColor::Green,
		false,
		DebugDrawTime);
}