#pragma once

#include "CoreMinimal.h"

class UT_TraversalComponent;
struct FTraversalCheckResult;

class GASTESTDEMO1_API T_TraversalClimb
{
public:
	static bool Detect(
		const UT_TraversalComponent& TraversalComponent,
		FTraversalCheckResult& OutTraversalResult
	);
};
