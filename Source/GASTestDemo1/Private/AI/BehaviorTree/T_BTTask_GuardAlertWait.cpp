// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTTask_GuardAlertWait.h"

#include "AI/T_ShooterAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UT_BTTask_GuardAlertWait::UT_BTTask_GuardAlertWait()
{
	NodeName = TEXT("Guard Alert Wait");
	bCreateNodeInstance = true;
	INIT_TASK_NODE_NOTIFY_FLAGS();
}

EBTNodeResult::Type UT_BTTask_GuardAlertWait::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!IsValid(AIController)) return EBTNodeResult::Failed;
	FocusLocation = FVector::ZeroVector;

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(AIController);
	if (IsValid(BlackboardComp))
	{
		if (IsValid(ShooterController) && ShooterController->GetAIState() == ETGuardAIState::Investigate
			&& BlackboardComp->IsVectorValueSet(GuardBBKeys::InvestigateLocation))
		{
			FocusLocation = BlackboardComp->GetValueAsVector(GuardBBKeys::InvestigateLocation);
		}
		else if (BlackboardComp->IsVectorValueSet(GuardBBKeys::LastKnownLocation))
		{
			FocusLocation = BlackboardComp->GetValueAsVector(GuardBBKeys::LastKnownLocation);
		}
		else if (BlackboardComp->IsVectorValueSet(GuardBBKeys::MoveLocation))
		{
			FocusLocation = BlackboardComp->GetValueAsVector(GuardBBKeys::MoveLocation);
		}
	}

	const bool bSearchObservation = IsValid(ShooterController)
		&& (ShooterController->GetAIState() == ETGuardAIState::Search
			|| ShooterController->GetAIState() == ETGuardAIState::Investigate);
	if (bSearchObservation)
	{
		const FVector Origin = IsValid(AIController->GetPawn()) ? AIController->GetPawn()->GetActorLocation() : FVector::ZeroVector;
		const FVector Direction = (FocusLocation - Origin).GetSafeNormal2D();
		BaseSearchYaw = Direction.IsNearlyZero() ? AIController->GetControlRotation().Yaw : Direction.Rotation().Yaw;
		AIController->ClearFocus(EAIFocusPriority::Gameplay);
		ShooterController->SetAlertObservationActive(true);
	}
	else if (!FocusLocation.IsNearlyZero())
	{
		AIController->SetFocalPoint(FocusLocation, EAIFocusPriority::Gameplay);
	}
	TotalWaitDuration = bSearchObservation
		? 3.f
		: FMath::Max(0.f, WaitDuration + FMath::FRandRange(-RandomDeviation, RandomDeviation));
	WaitRemaining = TotalWaitDuration;
	LookPhase = 0;
	return EBTNodeResult::InProgress;
}

void UT_BTTask_GuardAlertWait::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (IsValid(ShooterController) && ShooterController->GetAIState() == ETGuardAIState::Investigate
		&& IsValid(BlackboardComp) && BlackboardComp->IsVectorValueSet(GuardBBKeys::InvestigateLocation)
		&& !BlackboardComp->GetValueAsVector(GuardBBKeys::InvestigateLocation).Equals(FocusLocation, 1.f))
	{
		// 观察期间出现新声音，让 Investigate 序列从新位置重新寻路并重新计时。
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}

	WaitRemaining -= DeltaSeconds;
	const uint8 NewLookPhase = WaitRemaining <= TotalWaitDuration / 3.f ? 2
		: WaitRemaining <= TotalWaitDuration * 2.f / 3.f ? 1 : 0;
	if (IsValid(ShooterController)
		&& (ShooterController->GetAIState() == ETGuardAIState::Search
			|| ShooterController->GetAIState() == ETGuardAIState::Investigate))
	{
		if (AAIController* AIController = OwnerComp.GetAIOwner(); IsValid(AIController) && IsValid(AIController->GetPawn()))
		{
			const float TargetYaw = BaseSearchYaw + (NewLookPhase == 0 ? -35.f : NewLookPhase == 1 ? 35.f : 0.f);
			const FRotator NewRotation = FMath::RInterpConstantTo(
				AIController->GetControlRotation(), FRotator(0.f, TargetYaw, 0.f), DeltaSeconds, 120.f);
			AIController->SetControlRotation(NewRotation);
			AIController->GetPawn()->SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
		}
	}
	LookPhase = NewLookPhase;
	if (WaitRemaining <= 0.f)
	{
		if (IsValid(ShooterController) && ShooterController->GetAIState() == ETGuardAIState::Search)
		{
			// 第一次到达最后消失位置并观察完成后结束 Search，避免继续走旧的多个 Search Point。
			ShooterController->CompleteCurrentBehaviorState();
			return;
		}
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UT_BTTask_GuardAlertWait::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	if (AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(OwnerComp.GetAIOwner()))
	{
		ShooterController->SetAlertObservationActive(false);
	}
	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}
