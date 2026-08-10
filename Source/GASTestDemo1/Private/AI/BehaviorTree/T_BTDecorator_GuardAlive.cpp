// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTDecorator_GuardAlive.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Characters/T_BaseCharacter.h"

UT_BTDecorator_GuardAlive::UT_BTDecorator_GuardAlive()
{
	NodeName = TEXT("Guard Alive");
}

bool UT_BTDecorator_GuardAlive::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const AAIController* AIController = OwnerComp.GetAIOwner();
	const APawn* Pawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	const AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(Pawn);
	return IsValid(BaseCharacter) && BaseCharacter->IsAlive();
}
