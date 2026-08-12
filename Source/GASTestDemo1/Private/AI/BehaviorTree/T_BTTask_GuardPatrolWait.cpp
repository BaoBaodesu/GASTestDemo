#include "AI/BehaviorTree/T_BTTask_GuardPatrolWait.h"

#include "AIController.h"
#include "AI/T_ShooterAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/T_GuardCharacter.h"

namespace
{
	void FinishPatrolWaitAndAdvance(UBehaviorTreeComponent& OwnerComp, AT_GuardCharacter* Guard)
	{
		UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();

		if (IsValid(Guard))
		{
			// MoveTo 已成功并完成停留：强制切点，并立刻刷新黑板，避免下一拍 MoveTo 仍用旧点 AlreadyAtGoal
			Guard->AdvancePatrolPointAfterWait();
			const FVector NewDest = Guard->GetCurrentPatrolDestination();
			if (IsValid(BB))
			{
				BB->SetValueAsVector(GuardBBKeys::MoveLocation, NewDest);
			}
		}
	}
}

UT_BTTask_GuardPatrolWait::UT_BTTask_GuardPatrolWait()
{
	NodeName = TEXT("Guard Patrol Wait");
	bCreateNodeInstance = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UT_BTTask_GuardPatrolWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(IsValid(AIController) ? AIController->GetPawn() : nullptr);
	if (IsValid(Guard))
	{
		WaitRemaining = Guard->GetRolledPatrolPointWaitDuration();
	}
	else
	{
		WaitRemaining = FMath::Max(0.f,
			FallbackWaitSeconds + FMath::FRandRange(-FallbackRandomDeviation, FallbackRandomDeviation));
	}

	if (WaitRemaining <= 0.f)
	{
		FinishPatrolWaitAndAdvance(OwnerComp, Guard);
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::InProgress;
}

void UT_BTTask_GuardPatrolWait::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	WaitRemaining -= DeltaSeconds;
	if (WaitRemaining <= 0.f)
	{
		const AAIController* AIController = OwnerComp.GetAIOwner();
		AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(IsValid(AIController) ? AIController->GetPawn() : nullptr);
		FinishPatrolWaitAndAdvance(OwnerComp, Guard);
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}
