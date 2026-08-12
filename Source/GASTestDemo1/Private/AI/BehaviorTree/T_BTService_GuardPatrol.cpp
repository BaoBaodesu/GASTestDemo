// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTService_GuardPatrol.h"

#include "AIController.h"
#include "AI/T_ShooterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/T_GuardCharacter.h"

UT_BTService_GuardPatrol::UT_BTService_GuardPatrol()
{
	NodeName = TEXT("Guard Patrol Points");
	Interval = 0.5f;
	RandomDeviation = 0.1f;
	bCallTickOnSearchStart = false;
	MoveLocationKey.SelectedKeyName = GuardBBKeys::MoveLocation;
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
	const AT_ShooterAIController* AIController = Cast<AT_ShooterAIController>(OwnerComp.GetAIOwner());
	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(IsValid(AIController) ? AIController->GetPawn() : nullptr);
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!IsValid(Guard) || !IsValid(BlackboardComp)) return;

	Guard->EnsurePatrolRouteInitialized();

	const FName MoveKeyName = MoveLocationKey.SelectedKeyName.IsNone()
		? GuardBBKeys::MoveLocation
		: MoveLocationKey.SelectedKeyName;

	// Search 分支复用了巡逻服务；此时观察点必须固定为玩家最后消失位置，不能继续取巡逻路点。
	if (AIController->GetAIState() == ETGuardAIState::Search
		&& BlackboardComp->IsVectorValueSet(GuardBBKeys::LastKnownLocation))
	{
		BlackboardComp->SetValueAsVector(MoveKeyName,
			BlackboardComp->GetValueAsVector(GuardBBKeys::LastKnownLocation));
		return;
	}

	if (Guard->IsStationaryPatrol())
	{
		const FVector Loc = BlackboardComp->IsVectorValueSet(GuardBBKeys::HomeLocation)
			? BlackboardComp->GetValueAsVector(GuardBBKeys::HomeLocation)
			: Guard->GetActorLocation();
		if (!BlackboardComp->IsVectorValueSet(MoveKeyName)
			|| FVector::DistSquared(BlackboardComp->GetValueAsVector(MoveKeyName), Loc) > FMath::Square(50.f))
		{
			BlackboardComp->SetValueAsVector(MoveKeyName, Loc);
		}
		return;
	}

	const FVector Destination = Guard->GetCurrentPatrolDestination();
	const bool bNeedWrite = !BlackboardComp->IsVectorValueSet(MoveKeyName)
		|| !BlackboardComp->GetValueAsVector(MoveKeyName).Equals(Destination, 1.f);
	if (bNeedWrite)
	{
		BlackboardComp->SetValueAsVector(MoveKeyName, Destination);
	}
}
