#include "Player/Traversal/T_TraversalVault.h"

#include "Player/Components/T_TraversalComponent.h"

bool T_TraversalVault::Detect(
	const UT_TraversalComponent& TraversalComponent,
	FTraversalCheckResult& OutTraversalResult)
{
	FTraversalCheckResult Result;
	FHitResult WallHit;

	if (!TraversalComponent.DetectWall(WallHit)) return false;

	FHitResult TopHit;

	if (!TraversalComponent.DetectVaultTop(WallHit, TopHit)) return false;

	Result.bHasWall = true;
	Result.WallLocation = WallHit.ImpactPoint;
	Result.WallNormal = WallHit.ImpactNormal;
	Result.bHasTopSurface = true;
	Result.TopLocation = TopHit.ImpactPoint;
	Result.ObstacleHeight = TopHit.ImpactPoint.Z - TraversalComponent.GetCharacterFeetLocation().Z;

	Result.bHasFarEdge = TraversalComponent.MeasureObstacleDepth(
		WallHit,
		TopHit,
		Result.ObstacleDepth,
		Result.FarEdgeLocation
	);

	if (!Result.bHasFarEdge) return false;

	FVector LandingLocation;

	Result.bHasVaultLandingSpace = TraversalComponent.FindLandingLocation(
		WallHit,
		Result.FarEdgeLocation,
		LandingLocation
	);

	if (!CanVault(
			Result.ObstacleHeight,
			Result.ObstacleDepth,
			Result.bHasFarEdge,
			Result.bHasVaultLandingSpace,
			TraversalComponent.MinimumTraversalHeight,
			TraversalComponent.MaximumVaultHeight,
			TraversalComponent.MaximumVaultDepth))
	{
		return false;
	}

	Result.ActionType = ETraversalActionType::Vault;
	Result.LandingLocation = LandingLocation;
	TraversalComponent.BuildWarpTargets(Result);
	OutTraversalResult = Result;

	return true;
}

bool T_TraversalVault::CanVault(
	float ObstacleHeight,
	float ObstacleDepth,
	bool bHasFarEdge,
	bool bHasLandingSpace,
	float MinimumHeight,
	float MaximumHeight,
	float MaximumDepth)
{
	return FMath::IsWithinInclusive(
		ObstacleHeight,
		MinimumHeight,
		MaximumHeight
	) &&
		bHasFarEdge &&
		ObstacleDepth <= MaximumDepth &&
		bHasLandingSpace;
}
