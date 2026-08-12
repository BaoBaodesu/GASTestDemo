#include "AI/T_GuardAlertSubsystem.h"

#include "AI/T_ShooterAIController.h"
#include "Characters/T_GuardCharacter.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Perception/AISense_Hearing.h"

AT_GuardCharacter* UT_GuardAlertSubsystem::ReportThrowableImpactNoise(
	FVector Location, AActor* Instigator, float Loudness, float MaxRange)
{
	UWorld* World = GetWorld();
	if (!IsValid(World)) return nullptr;

	const float EffectiveRange = FMath::Max(MaxRange, 0.f);
	UAISense_Hearing::ReportNoiseEvent(
		World,
		Location,
		FMath::Max(Loudness, 0.f),
		Instigator,
		EffectiveRange,
		FName(TEXT("GuardNoise.Throwable.Impact")));

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	TArray<AT_GuardCharacter*> ReachableCandidates;
	TArray<AT_GuardCharacter*> InRangeCandidates;
	for (TActorIterator<AT_GuardCharacter> It(World); It; ++It)
	{
		AT_GuardCharacter* Guard = *It;
		AT_ShooterAIController* Controller = IsValid(Guard) ? Cast<AT_ShooterAIController>(Guard->GetController()) : nullptr;
		if (!IsValid(Controller) || !Guard->IsAlive() || Controller->GetAIState() == ETGuardAIState::Combat) continue;
		if (FVector::DistSquared(Guard->GetActorLocation(), Location) > FMath::Square(EffectiveRange)) continue;

		InRangeCandidates.Add(Guard);

		if (IsValid(NavSys))
		{
			UNavigationPath* Path = NavSys->FindPathToLocationSynchronously(World, Guard->GetActorLocation(), Location, Guard);
			if (!IsValid(Path) || !Path->IsValid() || Path->IsPartial()) continue;
		}
		ReachableCandidates.Add(Guard);
	}

	AT_GuardCharacter* SelectedGuard = SelectNearestEligibleGuard(ReachableCandidates, Location, EffectiveRange);
	for (AT_GuardCharacter* Guard : InRangeCandidates)
	{
		AT_ShooterAIController* Controller = Cast<AT_ShooterAIController>(Guard->GetController());
		if (!IsValid(Controller)) continue;

		if (Guard == SelectedGuard)
		{
			Controller->BeginInvestigation(Location);
		}
		else
		{
			Controller->ReactToDistraction(Location);
		}
	}
	return SelectedGuard;
}

AT_GuardCharacter* UT_GuardAlertSubsystem::SelectNearestEligibleGuard(
	const TArray<AT_GuardCharacter*>& Candidates, const FVector& Location, float MaxRange)
{
	AT_GuardCharacter* BestGuard = nullptr;
	float BestDistanceSquared = FMath::Square(FMath::Max(MaxRange, 0.f));
	for (AT_GuardCharacter* Guard : Candidates)
	{
		AT_ShooterAIController* Controller = IsValid(Guard) ? Cast<AT_ShooterAIController>(Guard->GetController()) : nullptr;
		if (!IsValid(Controller) || !Guard->IsAlive() || Controller->GetAIState() == ETGuardAIState::Combat) continue;
		const float DistanceSquared = FVector::DistSquared(Guard->GetActorLocation(), Location);
		if (DistanceSquared <= BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestGuard = Guard;
		}
	}
	return BestGuard;
}
