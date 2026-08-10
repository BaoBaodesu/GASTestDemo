// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTTask_GuardClearBlackboard.h"

#include "AI/T_ShooterAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UT_BTTask_GuardClearBlackboard::UT_BTTask_GuardClearBlackboard()
{
	NodeName = TEXT("Guard Clear Blackboard");
}

EBTNodeResult::Type UT_BTTask_GuardClearBlackboard::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(OwnerComp.GetAIOwner());
	if (IsValid(ShooterController))
	{
		ShooterController->ClearTargetState();
	}
	else
	{
		AT_ShooterAIController::ClearGuardBlackboard(OwnerComp.GetBlackboardComponent());
	}
	return EBTNodeResult::Succeeded;
}
