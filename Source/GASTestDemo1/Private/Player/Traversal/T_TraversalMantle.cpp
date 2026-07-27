#include "Player/Traversal/T_TraversalMantle.h"

#include "Player/Components/T_TraversalComponent.h"

bool T_TraversalMantle::Detect(
	const UT_TraversalComponent& TraversalComponent,
	FTraversalCheckResult& OutTraversalResult)
{
	FTraversalCheckResult Result;
	FHitResult WallHit;

	if (!TraversalComponent.DetectWall(WallHit)) return false;

	FHitResult TopHit;

	if (!TraversalComponent.DetectTop(WallHit, TopHit)) return false;

	Result.bHasWall = true;
	Result.WallLocation = WallHit.ImpactPoint;
	Result.WallNormal = WallHit.ImpactNormal;
	Result.bHasTopSurface = true;
	Result.TopLocation = TopHit.ImpactPoint;
	Result.ObstacleHeight = TopHit.ImpactPoint.Z - TraversalComponent.GetCharacterFeetLocation().Z;

	if (Result.ObstacleHeight > TraversalComponent.MaximumMantleHeight) return false;

	Result.bHasTopStandingSpace = TraversalComponent.FindTopStandingLocation(
		WallHit,
		TopHit,
		Result.LandingLocation
	);

	if (!Result.bHasTopStandingSpace) return false;
	
	Result.ActionType = ETraversalActionType::Mantle;
	TraversalComponent.BuildWarpTargets(Result);
	OutTraversalResult = Result;

	return true;
}
