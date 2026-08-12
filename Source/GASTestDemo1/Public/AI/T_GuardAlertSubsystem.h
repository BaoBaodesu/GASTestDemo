#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "T_GuardAlertSubsystem.generated.h"

class AT_GuardCharacter;

UCLASS()
class GASTESTDEMO1_API UT_GuardAlertSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Guard|Alert")
	AT_GuardCharacter* ReportThrowableImpactNoise(
		FVector Location,
		AActor* Instigator,
		float Loudness = 1.f,
		float MaxRange = 1500.f);

	static AT_GuardCharacter* SelectNearestEligibleGuard(
		const TArray<AT_GuardCharacter*>& Candidates,
		const FVector& Location,
		float MaxRange);
};
