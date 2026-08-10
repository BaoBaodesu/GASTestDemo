// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTService_GuardPatrol.h"

#include "AIController.h"
#include "AI/Navigation/NavigationTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "NavigationSystem.h"

UT_BTService_GuardPatrol::UT_BTService_GuardPatrol()
{
	NodeName = TEXT("Guard Patrol Points");
	Interval = 8.f;
	RandomDeviation = 2.f;
	bCallTickOnSearchStart = false;
}

void UT_BTService_GuardPatrol::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
	PickPatrolLocation(OwnerComp);
}

void UT_BTService_GuardPatrol::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	PickPatrolLocation(OwnerComp);
}

void UT_BTService_GuardPatrol::PickPatrolLocation(UBehaviorTreeComponent& OwnerComp)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!IsValid(Pawn) || !IsValid(BlackboardComp)) return;

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(Pawn->GetWorld());
	if (!IsValid(NavSys)) return;

	FNavLocation Result;
	if (NavSys->GetRandomReachablePointInRadius(Pawn->GetActorLocation(), PatrolRadius, Result))
	{
		BlackboardComp->SetValueAsVector(MoveLocationKey.SelectedKeyName, Result.Location);
	}
}
