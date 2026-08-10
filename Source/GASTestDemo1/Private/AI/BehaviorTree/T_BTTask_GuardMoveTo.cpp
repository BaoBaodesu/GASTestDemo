// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTTask_GuardMoveTo.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Navigation/PathFollowingComponent.h"

UT_BTTask_GuardMoveTo::UT_BTTask_GuardMoveTo()
{
	NodeName = TEXT("Guard Move To");
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UT_BTTask_GuardMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	StartMove(OwnerComp);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UT_BTTask_GuardMoveTo::MapMoveRequestResult(EPathFollowingRequestResult::Type RequestResult)
{
	switch (RequestResult)
	{
	case EPathFollowingRequestResult::RequestSuccessful:
		return EBTNodeResult::InProgress;
	case EPathFollowingRequestResult::AlreadyAtGoal:
		return EBTNodeResult::Succeeded;
	default:
		return EBTNodeResult::Failed;
	}
}

void UT_BTTask_GuardMoveTo::StartMove(UBehaviorTreeComponent& OwnerComp)
{
	// 先作废旧请求 ID，忽略被替换/中止的旧移动回调
	CurrentMoveRequestID = FAIRequestID::InvalidRequest;
	ElapsedTime = 0.f;

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!IsValid(AIController) || !IsValid(BlackboardComp) || !BlackboardComp->IsVectorValueSet(MoveToKeyName))
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	const FVector Destination = BlackboardComp->GetValueAsVector(MoveToKeyName);
	const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(
		Destination, AcceptanceRadius, true, true, true, false, nullptr, true);
	const EBTNodeResult::Type TaskResult = MapMoveRequestResult(MoveResult);
	if (TaskResult != EBTNodeResult::InProgress)
	{
		FinishLatentTask(OwnerComp, TaskResult);
		return;
	}

	OwningComponent = &OwnerComp;

	if (UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent())
	{
		PathComp->OnRequestFinished.RemoveAll(this);
		PathComp->OnRequestFinished.AddUObject(this, &ThisClass::OnMoveFinished);
		CurrentMoveRequestID = PathComp->GetCurrentRequestId();
	}
	else
	{
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	if (bObserveBlackboardValue)
	{
		ObservedKeyID = BlackboardComp->GetKeyID(MoveToKeyName);
		BlackboardComp->RegisterObserver(ObservedKeyID, this,
			FOnBlackboardChangeNotification::CreateUObject(this, &ThisClass::OnBlackboardValueChanged));
	}
}

void UT_BTTask_GuardMoveTo::OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	// 只处理当前移动请求；被替换/中止的旧请求回调直接忽略
	if (!RequestID.IsEquivalent(CurrentMoveRequestID)) return;

	UBehaviorTreeComponent* Comp = OwningComponent.Get();
	if (!IsValid(Comp)) return;

	FinishLatentTask(*Comp, Result.IsSuccess() ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
}

void UT_BTTask_GuardMoveTo::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	ElapsedTime += DeltaSeconds;
	if (ElapsedTime >= MoveTimeout)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: 移动超时 %.1fs，任务失败。"), *GetName(), MoveTimeout);
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
	}
}

EBlackboardNotificationResult UT_BTTask_GuardMoveTo::OnBlackboardValueChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID)
{
	if (ChangedKeyID == ObservedKeyID)
	{
		if (UBehaviorTreeComponent* Comp = OwningComponent.Get())
		{
			StartMove(*Comp);
		}
	}
	return EBlackboardNotificationResult::ContinueObserving;
}

void UT_BTTask_GuardMoveTo::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (IsValid(AIController))
	{
		if (UPathFollowingComponent* PathComp = AIController->GetPathFollowingComponent())
		{
			PathComp->OnRequestFinished.RemoveAll(this);
		}
	}

	if (UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent())
	{
		BlackboardComp->UnregisterObserversFrom(this);
	}

	OwningComponent = nullptr;
	ObservedKeyID = FBlackboard::InvalidKey;
	CurrentMoveRequestID = FAIRequestID::InvalidRequest;
	ElapsedTime = 0.f;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
