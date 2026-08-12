// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTTask_GuardMoveTo.h"

#include "AIController.h"
#include "AI/T_ShooterAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/T_GuardCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"

UT_BTTask_GuardMoveTo::UT_BTTask_GuardMoveTo()
{
	NodeName = TEXT("Guard Move To");
	bCreateNodeInstance = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UT_BTTask_GuardMoveTo::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(AIController);
	AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(IsValid(AIController) ? AIController->GetPawn() : nullptr);
	if (IsValid(ShooterController) && ShooterController->GetAIState() == ETGuardAIState::Return
		&& IsValid(Guard) && IsValid(BlackboardComp) && BlackboardComp->IsVectorValueSet(MoveToKeyName))
	{
		const FVector ToDestination = BlackboardComp->GetValueAsVector(MoveToKeyName) - Guard->GetActorLocation();
		if (!ToDestination.IsNearlyZero())
		{
			const float YawDelta = FMath::Abs(FMath::FindDeltaAngleDegrees(
				Guard->GetActorRotation().Yaw,
				ToDestination.Rotation().Yaw));
			if (YawDelta > 5.f)
			{
				if (UCharacterMovementComponent* Movement = Guard->GetCharacterMovement())
				{
					Movement->bOrientRotationToMovement = false;
					Movement->bUseControllerDesiredRotation = true;
				}
				Guard->bUseControllerRotationYaw = false;
				ShooterController->SetFocalPoint(BlackboardComp->GetValueAsVector(MoveToKeyName), EAIFocusPriority::Gameplay);
				OwningComponent = &OwnerComp;
				ElapsedTime = 0.f;
				bWaitingForReturnFacing = true;
				return EBTNodeResult::InProgress;
			}
		}
	}

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
	if (bWaitingForReturnFacing)
	{
		AAIController* AIController = OwnerComp.GetAIOwner();
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(IsValid(AIController) ? AIController->GetPawn() : nullptr);
		if (!IsValid(AIController) || !IsValid(Guard) || !IsValid(BlackboardComp)
			|| !BlackboardComp->IsVectorValueSet(MoveToKeyName))
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
			return;
		}

		const FVector ToDestination = BlackboardComp->GetValueAsVector(MoveToKeyName) - Guard->GetActorLocation();
		const float YawDelta = ToDestination.IsNearlyZero() ? 0.f : FMath::Abs(FMath::FindDeltaAngleDegrees(
			Guard->GetActorRotation().Yaw,
			ToDestination.Rotation().Yaw));
		if (YawDelta <= 5.f)
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (UCharacterMovementComponent* Movement = Guard->GetCharacterMovement())
			{
				Movement->bOrientRotationToMovement = true;
				Movement->bUseControllerDesiredRotation = false;
			}
			bWaitingForReturnFacing = false;
			StartMove(OwnerComp);
			return;
		}
	}

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
		if (bWaitingForReturnFacing)
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
			if (AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(AIController->GetPawn()))
			{
				if (UCharacterMovementComponent* Movement = Guard->GetCharacterMovement())
				{
					Movement->bOrientRotationToMovement = true;
					Movement->bUseControllerDesiredRotation = false;
				}
			}
		}
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
	bWaitingForReturnFacing = false;

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
