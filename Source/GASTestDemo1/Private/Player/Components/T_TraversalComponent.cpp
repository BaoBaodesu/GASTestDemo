#include "Player/Components/T_TraversalComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Characters/T_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/TTags.h"
#include "Player/Traversal/T_TraversalClimb.h"
#include "Player/Traversal/T_TraversalMantle.h"
#include "Player/Traversal/T_TraversalVault.h"


UT_TraversalComponent::UT_TraversalComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UT_TraversalComponent::Jump()
{
	if (const AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(GetOwner());
		IsValid(BaseCharacter) && BaseCharacter->bIsFalling)
	{
		return false;
	}

	FTraversalCheckResult Result;
	if (!DetectTraversal(Result)) return false;

	UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerCharacter);
	if (!IsValid(AbilitySystemComponent)) return false;

	return AbilitySystemComponent->TryActivateAbilitiesByTag(
		TTags::TAbilities::Traversal.GetTag().GetSingleTagContainer()
	);
}


void UT_TraversalComponent::BeginPlay()
{
	Super::BeginPlay();

	CacheOwnerComponents();
}


void UT_TraversalComponent::CacheOwnerComponents()
{
	OwnerCharacter = Cast<ACharacter>(GetOwner());

	if (!IsValid(OwnerCharacter))
	{
		UE_LOG(LogTemp, Warning, TEXT("T_TraversalComponent 必须添加到 ACharacter 上。Owner: %s"), *GetNameSafe(GetOwner()));
		return;
	}

	OwnerCapsuleComponent = OwnerCharacter->GetCapsuleComponent();

}


bool UT_TraversalComponent::DetectTraversal(FTraversalCheckResult& OutTraversalResult)
{
	OutTraversalResult = FTraversalCheckResult();

	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCapsuleComponent))
	{
		CacheOwnerComponents();
	}

	if (!IsValid(OwnerCharacter) || !IsValid(OwnerCapsuleComponent) || !IsValid(GetWorld()))
	{
		return false;
	}

	if (const AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(OwnerCharacter);
		IsValid(BaseCharacter) && BaseCharacter->bIsFalling)
	{
		return false;
	}

	const bool bTraversalFound = T_TraversalVault::Detect(*this, OutTraversalResult) || T_TraversalMantle::Detect(*this, OutTraversalResult) || T_TraversalClimb::Detect(*this, OutTraversalResult);

	if (!bTraversalFound)
	{
		return false;
	}

	OutTraversalResult.bTraversalFound = true;
	DrawTraversalDebug(OutTraversalResult);

	return true;
}


bool UT_TraversalComponent::DetectWall(
	FHitResult& OutWallHit) const
{
	const FVector TraceStart = GetCharacterFeetLocation() + FVector::UpVector * WallTraceHeight;

	const FVector CharacterForward = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();

	const FVector TraceEnd = TraceStart + CharacterForward * WallTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const bool bHitWall = GetWorld()->SweepSingleByChannel(OutWallHit, TraceStart, TraceEnd, FQuat::Identity, TraversalTraceChannel, FCollisionShape::MakeSphere(WallTraceRadius), QueryParams, FCollisionResponseParams::DefaultResponseParam);

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			TraceStart,
			TraceEnd,
			bHitWall ? FColor::Green : FColor::Red,
			false,
			GetDebugDuration(),
			0,
			2.f
		);

		DrawDebugSphere(
			GetWorld(),
			bHitWall ? OutWallHit.ImpactPoint : TraceEnd,
			WallTraceRadius,
			12,
			bHitWall ? FColor::Green : FColor::Red,
			false,
			GetDebugDuration()
		);
	}

	if (!bHitWall)
	{
		return false;
	}

	// 法线 Z 过大时，更可能命中了斜坡或地面
	if (FMath::Abs(OutWallHit.ImpactNormal.Z) >
		MaximumWallNormalZ)
	{
		return false;
	}

	const FVector WallFacingDirection = -OutWallHit.ImpactNormal.GetSafeNormal2D();

	// 防止检测到角色侧面的墙
	if (FVector::DotProduct(
			CharacterForward,
			WallFacingDirection) < MinimumWallFacingDot)
	{
		return false;
	}

	return true;
}


bool UT_TraversalComponent::DetectTop(
	const FHitResult& WallHit,
	FHitResult& OutTopHit) const
{
	const FVector IntoObstacleDirection = GetIntoObstacleDirection(WallHit.ImpactNormal);

	FVector TraceStart = WallHit.ImpactPoint + IntoObstacleDirection * TopTraceInset;

	TraceStart.Z = GetCharacterFeetLocation().Z + MaximumClimbHeight + TopTraceExtraHeight;

	FVector TraceEnd = TraceStart;

	// 与 Test1 的顶部检测一致：允许从脚底略下方开始，避免低矮障碍漏检。
	TraceEnd.Z = GetCharacterFeetLocation().Z - 20.f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const bool bHitTop = GetWorld()->SweepSingleByChannel(OutTopHit, TraceStart, TraceEnd, FQuat::Identity, TraversalTraceChannel, FCollisionShape::MakeSphere(WallTraceRadius), QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			TraceStart,
			TraceEnd,
			bHitTop ? FColor::Cyan : FColor::Red,
			false,
			GetDebugDuration(),
			0,
			2.f
		);

		if (bHitTop)
		{
			DrawDebugSphere(
				GetWorld(),
				OutTopHit.ImpactPoint,
				8.f,
				12,
				FColor::Cyan,
				false,
				GetDebugDuration()
			);
		}
	}

	if (!bHitTop)
	{
		return false;
	}

	if (OutTopHit.ImpactNormal.Z < MinimumTopNormalZ)
	{
		return false;
	}

	const float ObstacleHeight = OutTopHit.ImpactPoint.Z - GetCharacterFeetLocation().Z;

	return ObstacleHeight >= MinimumTraversalHeight &&
		ObstacleHeight <= MaximumClimbHeight;
}


bool UT_TraversalComponent::DetectVaultTop(
	const FHitResult& WallHit,
	FHitResult& OutTopHit) const
{
	if (!IsValid(OwnerCharacter)) return false;
	if (!IsValid(GetWorld())) return false;

	const FVector FeetLocation = GetCharacterFeetLocation();
	const FVector IntoObstacleDirection = GetIntoObstacleDirection(WallHit.ImpactNormal);

	FVector TraceStart = WallHit.ImpactPoint + IntoObstacleDirection * TopTraceInset;

	/*
	 * 只从 Vault 最大高度上方向下检测，
	 * 避免命中窗口上方的墙体。
	 */
	TraceStart.Z = FeetLocation.Z + MaximumVaultHeight + VaultTopTraceExtraHeight;

	FVector TraceEnd = TraceStart;
	TraceEnd.Z = FeetLocation.Z - 20.f;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.bTraceComplex = false;

	const bool bHitTop = GetWorld()->SweepSingleByChannel(OutTopHit, TraceStart, TraceEnd, FQuat::Identity, TraversalTraceChannel, FCollisionShape::MakeSphere(WallTraceRadius), QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			TraceStart,
			TraceEnd,
			bHitTop ? FColor::Green : FColor::Red,
			false,
			GetDebugDuration(),
			0,
			2.f
		);

		if (bHitTop)
		{
			DrawDebugSphere(
				GetWorld(),
				OutTopHit.ImpactPoint,
				8.f,
				12,
				FColor::Green,
				false,
				GetDebugDuration()
			);
		}
	}

	if (!bHitTop) return false;
	if (OutTopHit.ImpactNormal.Z < MinimumTopNormalZ) return false;

	const float ObstacleHeight = OutTopHit.ImpactPoint.Z - FeetLocation.Z;

	return FMath::IsWithinInclusive(
		ObstacleHeight,
		MinimumTraversalHeight,
		MaximumVaultHeight
	);
}

bool UT_TraversalComponent::MeasureObstacleDepth(
	const FHitResult& WallHit,
	const FHitResult& TopHit,
	float& OutObstacleDepth,
	FVector& OutFarEdgeLocation) const
{
	const FVector IntoObstacleDirection = GetIntoObstacleDirection(WallHit.ImpactNormal);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	for (float CurrentDistance = TopTraceInset + DepthTraceStep;
		CurrentDistance <= MaximumDepthToMeasure;
		CurrentDistance += DepthTraceStep)
	{
		const FVector TraceBase = WallHit.ImpactPoint + IntoObstacleDirection * CurrentDistance;

		const FVector TraceStart = TraceBase + FVector::UpVector * DepthTraceHeight;

		const FVector TraceEnd = TraceStart - FVector::UpVector * DepthTraceDepth;

		FHitResult DepthHit;

		const bool bHitTop = GetWorld()->SweepSingleByChannel(DepthHit, TraceStart, TraceEnd, FQuat::Identity, TraversalTraceChannel, FCollisionShape::MakeSphere(WallTraceRadius), QueryParams);

		const bool bContinuousTop = bHitTop && DepthHit.ImpactNormal.Z >= MinimumTopNormalZ && FMath::Abs(DepthHit.ImpactPoint.Z - TopHit.ImpactPoint.Z) <= MaximumTopHeightDifference;

		if (bDrawDebug)
		{
			DrawDebugLine(
				GetWorld(),
				TraceStart,
				TraceEnd,
				bContinuousTop
				? FColor::Yellow
				: FColor::Orange,
				false,
				GetDebugDuration(),
				0,
				1.f
			);
		}

		if (!bContinuousTop)
		{
			/*
			 * 后沿大约位于最后一次成功检测和
			 * 第一次失败检测之间。
			 */
			OutObstacleDepth = FMath::Max(TopTraceInset, CurrentDistance - DepthTraceStep * 0.5f);

			OutFarEdgeLocation = WallHit.ImpactPoint + IntoObstacleDirection * OutObstacleDepth;

			OutFarEdgeLocation.Z =
				TopHit.ImpactPoint.Z;

			if (bDrawDebug)
			{
				DrawDebugSphere(
					GetWorld(),
					OutFarEdgeLocation,
					10.f,
					12,
					FColor::Orange,
					false,
					GetDebugDuration()
				);
			}

			return true;
		}
	}

	/*
	 * 在最大检测范围内仍然有顶部，
	 * 将它看作较宽的平台，不适合 Vault。
	 */
	OutObstacleDepth = MaximumDepthToMeasure;

	OutFarEdgeLocation = WallHit.ImpactPoint + IntoObstacleDirection * MaximumDepthToMeasure;

	OutFarEdgeLocation.Z = TopHit.ImpactPoint.Z;

	return false;
}


bool UT_TraversalComponent::FindTopStandingLocation(
	const FHitResult& WallHit,
	const FHitResult& TopHit,
	FVector& OutStandingLocation) const
{
	const FVector IntoObstacleDirection = GetIntoObstacleDirection(WallHit.ImpactNormal);

	const float StandingInset = FMath::Max(TopStandingInset, OwnerCapsuleComponent->GetScaledCapsuleRadius());
	const FVector FloorReferenceLocation = TopHit.ImpactPoint + IntoObstacleDirection * StandingInset;
	const FVector TraceStart = FloorReferenceLocation + FVector::UpVector * LandingTraceHeight;

	const FVector TraceEnd = FloorReferenceLocation - FVector::UpVector * LandingTraceDepth;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	FHitResult FloorHit;

	const bool bHitFloor = GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, TraversalTraceChannel, QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			TraceStart,
			TraceEnd,
			bHitFloor ? FColor::Blue : FColor::Red,
			false,
			GetDebugDuration(),
			0,
			2.f
		);
	}
	
	if (!bHitFloor) return false;
	if (FloorHit.ImpactNormal.Z < MinimumTopNormalZ) return false;
	if (FMath::Abs(FloorHit.ImpactPoint.Z - TopHit.ImpactPoint.Z) > MaximumTopHeightDifference) return false;

	/*
	 * ACharacter 的 ActorLocation 位于胶囊体中心，
	 * 所以站立位置需要增加胶囊体半高。
	 */
	OutStandingLocation = FloorHit.ImpactPoint + FVector::UpVector * OwnerCapsuleComponent->GetScaledCapsuleHalfHeight();

	return IsCapsuleLocationClear(OutStandingLocation);
}


bool UT_TraversalComponent::FindLandingLocation(
	const FHitResult& WallHit,
	const FVector& FarEdgeLocation,
	FVector& OutLandingLocation) const
{
	const FVector IntoObstacleDirection = GetIntoObstacleDirection(WallHit.ImpactNormal);

	const FVector LandingReferenceLocation = FarEdgeLocation + IntoObstacleDirection * VaultLandingForwardOffset;

	const FVector TraceStart = LandingReferenceLocation + FVector::UpVector * LandingTraceHeight;

	const FVector TraceEnd = LandingReferenceLocation - FVector::UpVector * LandingTraceDepth;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	FHitResult LandingHit;

	const bool bHitLanding = GetWorld()->LineTraceSingleByChannel(LandingHit, TraceStart, TraceEnd, TraversalTraceChannel, QueryParams);

	if (bDrawDebug)
	{
		DrawDebugLine(
			GetWorld(),
			TraceStart,
			TraceEnd,
			bHitLanding ? FColor::Emerald : FColor::Red,
			false,
			GetDebugDuration(),
			0,
			2.f
		);
	}

	if (!bHitLanding ||
		LandingHit.ImpactNormal.Z < MinimumTopNormalZ)
	{
		return false;
	}

	OutLandingLocation = LandingHit.ImpactPoint + FVector::UpVector * OwnerCapsuleComponent->GetScaledCapsuleHalfHeight();

	return IsCapsuleLocationClear(OutLandingLocation);
}


bool UT_TraversalComponent::IsCapsuleLocationClear(
	const FVector& CapsuleLocation,
	const float ClearanceShrinkOverride) const
{
	if (!IsValid(OwnerCapsuleComponent))
	{
		return false;
	}

	const float EffectiveClearanceShrink = ClearanceShrinkOverride >= 0.f
		? ClearanceShrinkOverride
		: CapsuleClearanceShrink;
	const float CapsuleRadius = FMath::Max(1.f, OwnerCapsuleComponent->GetScaledCapsuleRadius() - EffectiveClearanceShrink);

	const float CapsuleHalfHeight = FMath::Max(CapsuleRadius, OwnerCapsuleComponent->GetScaledCapsuleHalfHeight() - EffectiveClearanceShrink);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	const bool bBlocked = GetWorld()->OverlapBlockingTestByChannel(CapsuleLocation, FQuat::Identity, CapsuleTestChannel, FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight), QueryParams, FCollisionResponseParams::DefaultResponseParam);

	if (bDrawDebug)
	{
		DrawDebugCapsule(
			GetWorld(),
			CapsuleLocation,
			CapsuleHalfHeight,
			CapsuleRadius,
			FQuat::Identity,
			bBlocked ? FColor::Red : FColor::Green,
			false,
			GetDebugDuration()
		);
	}

	return !bBlocked;
}


bool UT_TraversalComponent::FindSafeStandingLocationBelow(
	const FVector& ReferenceLocation,
	const float FloorClearance,
	FVector& OutStandingLocation) const
{
	if (!IsValid(OwnerCharacter) ||
		!IsValid(OwnerCapsuleComponent) ||
		!IsValid(GetWorld()))
	{
		return false;
	}

	const float CapsuleHalfHeight = OwnerCapsuleComponent->GetScaledCapsuleHalfHeight();
	// 从角色中心开始向下找地面，避免命中角色头顶上方的其它表面。
	const FVector TraceStart = ReferenceLocation;
	const FVector TraceEnd =
		ReferenceLocation - FVector::UpVector * LandingTraceDepth;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerCharacter);

	FHitResult FloorHit;
	const bool bHitFloor = GetWorld()->LineTraceSingleByChannel(
		FloorHit,
		TraceStart,
		TraceEnd,
		TraversalTraceChannel,
		QueryParams);

	if (!bHitFloor || FloorHit.ImpactNormal.Z < MinimumTopNormalZ)
	{
		return false;
	}

	OutStandingLocation = FVector(
		ReferenceLocation.X,
		ReferenceLocation.Y,
		FloorHit.ImpactPoint.Z + CapsuleHalfHeight + FloorClearance);

	// 动作结束恢复真实碰撞，因此必须验证完整胶囊。
	return IsCapsuleLocationClear(OutStandingLocation, 0.f);
}


void UT_TraversalComponent::DrawTraversalDebug(
	const FTraversalCheckResult& Result) const
{
	if (!bDrawDebug) return;

	DrawDebugSphere(
		GetWorld(),
		Result.WallLocation,
		10.f,
		12,
		FColor::White,
		false,
		GetDebugDuration()
	);

	DrawDebugSphere(
		GetWorld(),
		Result.TopLocation,
		10.f,
		12,
		FColor::Cyan,
		false,
		GetDebugDuration()
	);

	DrawDebugSphere(
		GetWorld(),
		Result.FrontLedgeWarpTarget.GetLocation(),
		12.f,
		12,
		FColor::Purple,
		false,
		GetDebugDuration()
	);

	DrawDebugDirectionalArrow(
		GetWorld(),
		Result.FrontLedgeWarpTarget.GetLocation(),
		Result.FrontLedgeWarpTarget.GetLocation() + Result.FrontLedgeWarpTarget.GetRotation().GetForwardVector() * 35.f,
		10.f,
		FColor::Purple,
		false,
		GetDebugDuration(),
		0,
		2.f
	);

	DrawDebugSphere(
		GetWorld(),
		Result.FarEdgeLocation,
		10.f,
		12,
		FColor::Orange,
		false,
		GetDebugDuration()
	);

	DrawDebugSphere(
		GetWorld(),
		Result.LandingLocation,
		10.f,
		12,
		FColor::Emerald,
		false,
		GetDebugDuration()
	);

	DrawDebugCapsule(
		GetWorld(),
		Result.LandingLocation,
		OwnerCapsuleComponent->GetScaledCapsuleHalfHeight(),
		OwnerCapsuleComponent->GetScaledCapsuleRadius(),
		FQuat::Identity,
		FColor::Cyan,
		false,
		GetDebugDuration()
	);
}


void UT_TraversalComponent::BuildWarpTargets(FTraversalCheckResult& TraversalResult) const
{
	if (!IsValid(OwnerCharacter)) return;
	if (!IsValid(OwnerCapsuleComponent)) return;

	TraversalResult.TargetRotation = GetFacingWallRotation(TraversalResult.WallNormal);

	const FQuat TargetRotationQuaternion = TraversalResult.TargetRotation.Quaternion();
	const FVector HorizontalWallNormal = TraversalResult.WallNormal.GetSafeNormal2D();

	FVector FrontLedgeLocation = TraversalResult.WallLocation + HorizontalWallNormal * (OwnerCapsuleComponent->GetScaledCapsuleRadius() + 2.f);

	switch (TraversalResult.ActionType)
	{
	case ETraversalActionType::Climb:
	{
		// FrontLedge represents the animation root while the hands are on the ledge,
		// not the centre of a standing capsule. Its height therefore comes from the
		// animation's root-to-ledge distance and must not include capsule half height.
		FrontLedgeLocation.Z = TraversalResult.TopLocation.Z - ClimbRootToLedgeHeight;
		const FVector ClimbHorizontalOffset(
			ClimbFrontLedgeWarpOffset.X,
			ClimbFrontLedgeWarpOffset.Y,
			0.f);
		FrontLedgeLocation += TargetRotationQuaternion.RotateVector(ClimbHorizontalOffset);
		break;
	}
	case ETraversalActionType::Mantle:
		FrontLedgeLocation.Z = TraversalResult.TopLocation.Z - MantleRootToLedgeHeight;
		FrontLedgeLocation += TargetRotationQuaternion.RotateVector(MantleFrontLedgeWarpOffset);
		break;
	default:
		FrontLedgeLocation.Z = OwnerCharacter->GetActorLocation().Z;
		FrontLedgeLocation += TargetRotationQuaternion.RotateVector(FrontLedgeWarpOffset);
		break;
	}

	FVector BackLedgeLocation;
	switch (TraversalResult.ActionType)
	{
	case ETraversalActionType::Vault:
		BackLedgeLocation =
			TraversalResult.FarEdgeLocation +
			TargetRotationQuaternion.RotateVector(BackLedgeWarpOffset);
		break;
	case ETraversalActionType::Climb:
	case ETraversalActionType::Mantle:
	default:
		BackLedgeLocation = TraversalResult.LandingLocation;
		break;
	}
	TraversalResult.FrontLedgeWarpTarget = FTransform(TraversalResult.TargetRotation, FrontLedgeLocation);
	TraversalResult.BackLedgeWarpTarget = FTransform(TraversalResult.TargetRotation, BackLedgeLocation);

	UE_LOG(LogTemp, Verbose, TEXT("Traversal targets: FrontZ=%.1f BackZ=%.1f TopZ=%.1f LandingZ=%.1f"), FrontLedgeLocation.Z, BackLedgeLocation.Z, TraversalResult.TopLocation.Z, TraversalResult.LandingLocation.Z);
}

FVector UT_TraversalComponent::GetCharacterFeetLocation() const
{
	if (!IsValid(OwnerCharacter) ||
		!IsValid(OwnerCapsuleComponent))
	{
		return FVector::ZeroVector;
	}

	return OwnerCharacter->GetActorLocation()
		- FVector::UpVector *
		OwnerCapsuleComponent->
			GetScaledCapsuleHalfHeight();
}


FVector UT_TraversalComponent::GetIntoObstacleDirection(
	const FVector& WallNormal) const
{
	return -WallNormal.GetSafeNormal2D();
}


FRotator UT_TraversalComponent::GetFacingWallRotation(
	const FVector& WallNormal) const
{
	const float TargetYaw = (-WallNormal.GetSafeNormal2D()).Rotation().Yaw;

	return FRotator(0.f, TargetYaw, 0.f);
}


float UT_TraversalComponent::GetDebugDuration() const
{
	return bDrawDebug ? DebugDrawDuration : 0.f;
}
