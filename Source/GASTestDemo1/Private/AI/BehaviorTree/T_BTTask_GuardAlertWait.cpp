// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTTask_GuardAlertWait.h"

#include "AI/T_ShooterAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UT_BTTask_GuardAlertWait::UT_BTTask_GuardAlertWait()
{
	NodeName = TEXT("Guard Alert Wait");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UT_BTTask_GuardAlertWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!IsValid(AIController)) return EBTNodeResult::Failed;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (IsValid(BlackboardComp) && BlackboardComp->IsVectorValueSet(GuardBBKeys::MoveLocation))
	{
		AIController->SetFocalPoint(BlackboardComp->GetValueAsVector(GuardBBKeys::MoveLocation), EAIFocusPriority::Gameplay);
	}

	WaitRemaining = FMath::Max(0.f, WaitDuration);
	return EBTNodeResult::InProgress;
}

void UT_BTTask_GuardAlertWait::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	WaitRemaining -= DeltaSeconds;
	if (WaitRemaining <= 0.f)
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
