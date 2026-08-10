// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTDecorator_GuardBlackboard.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UT_BTDecorator_GuardBlackboard::UT_BTDecorator_GuardBlackboard()
{
	NodeName = TEXT("Guard Blackboard");
	INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

bool UT_BTDecorator_GuardBlackboard::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComp)) return false;

	const FName KeyName = GetSelectedBlackboardKey();
	const FBlackboard::FKey KeyID = BlackboardComp->GetKeyID(KeyName);
	if (!BlackboardComp->IsValidKey(KeyID)) return false;

	const UClass* KeyType = BlackboardComp->GetKeyType(KeyID);
	bool bValue = false;
	if (KeyType && KeyType->IsChildOf(UBlackboardKeyType_Bool::StaticClass()))
	{
		bValue = BlackboardComp->GetValueAsBool(KeyName);
	}
	else if (KeyType && KeyType->IsChildOf(UBlackboardKeyType_Vector::StaticClass()))
	{
		bValue = BlackboardComp->IsVectorValueSet(KeyName);
	}
	else if (KeyType && KeyType->IsChildOf(UBlackboardKeyType_Object::StaticClass()))
	{
		bValue = IsValid(BlackboardComp->GetValueAsObject(KeyName));
	}

	switch (Condition)
	{
	case EGuardBlackboardCondition::IsSet:
		return bValue;
	case EGuardBlackboardCondition::IsNotSet:
		return !bValue;
	case EGuardBlackboardCondition::IsTrue:
		return bValue;
	case EGuardBlackboardCondition::IsFalse:
		return !bValue;
	default:
		return false;
	}
}
