#include "AI/BehaviorTree/T_BTService_GuardCombatMovement.h"

#include "AbilitySystemComponent.h"
#include "AI/T_ShooterAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/T_GuardCharacter.h"
#include "GameplayTags/TTags.h"
#include "NavigationSystem.h"

UT_BTService_GuardCombatMovement::UT_BTService_GuardCombatMovement()
{
	NodeName = TEXT("Guard Combat Movement");
	Interval = 1.5f;
	RandomDeviation = 0.5f;
	bCallTickOnSearchStart = false;
	CombatMoveLocationKey.SelectedKeyName = GuardBBKeys::CombatMoveLocation;
}

bool UT_BTService_GuardCombatMovement::IsProjectedMoveUsable(bool bProjected, bool bPathValid)
{
	return bProjected && bPathValid;
}

void UT_BTService_GuardCombatMovement::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AT_ShooterAIController* Controller = Cast<AT_ShooterAIController>(OwnerComp.GetAIOwner());
	AT_GuardCharacter* Guard = IsValid(Controller) ? Controller->GetGuardCharacter() : nullptr;
	AActor* Target = IsValid(Controller) ? Controller->GetCurrentTarget() : nullptr;
	if (!IsValid(Guard) || !IsValid(Target) || Controller->GetAIState() != ETGuardAIState::Combat) return;

	UAbilitySystemComponent* ASC = Guard->GetAbilitySystemComponent();
	if (IsValid(ASC) && (ASC->HasMatchingGameplayTag(TTags::State::Action::Shooting)
		|| ASC->HasMatchingGameplayTag(TTags::State::Action::Reloading))) return;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Guard->GetWorld());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!IsValid(NavSys) || !IsValid(BB)) return;

	const FVector Origin = Guard->GetActorLocation();
	const FVector ToTarget = (Target->GetActorLocation() - Origin).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero()) return;
	const FVector Right = FVector::CrossProduct(FVector::UpVector, ToTarget).GetSafeNormal();
	const float FirstSign = FMath::RandBool() ? 1.f : -1.f;

	for (int32 Attempt = 0; Attempt < 2; ++Attempt)
	{
		const float SideSign = Attempt == 0 ? FirstSign : -FirstSign;
		const FVector Candidate = Origin + Right * SideSign * FMath::FRandRange(150.f, 300.f);
		FNavLocation Projected;
		const bool bProjected = NavSys->ProjectPointToNavigation(Candidate, Projected, NavigationQueryExtent);
		if (!IsProjectedMoveUsable(bProjected, bProjected)) continue;

		BB->SetValueAsVector(CombatMoveLocationKey.SelectedKeyName, Projected.Location);
		Controller->MoveToLocation(Projected.Location, 60.f, true, true, true, false, nullptr, true);
		return;
	}
}
