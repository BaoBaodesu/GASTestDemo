#pragma once

#include "CoreMinimal.h"

class UT_TraversalComponent;
struct FTraversalCheckResult;

class GASTESTDEMO1_API T_TraversalVault
{
public:
	static bool Detect(
		const UT_TraversalComponent& TraversalComponent,
		FTraversalCheckResult& OutTraversalResult
	);

	static bool CanVault(
		float ObstacleHeight,
		float ObstacleDepth,
		bool bHasFarEdge,
		bool bHasLandingSpace,
		float MinimumHeight,
		float MaximumHeight,
		float MaximumDepth
	);
};
