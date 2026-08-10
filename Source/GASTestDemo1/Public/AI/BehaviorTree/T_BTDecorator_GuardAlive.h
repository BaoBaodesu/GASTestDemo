// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "T_BTDecorator_GuardAlive.generated.h"

/**
 * Guard 存活装饰器：只有角色存活时分支才可用。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTDecorator_GuardAlive : public UBTDecorator
{
	GENERATED_BODY()

public:

	UT_BTDecorator_GuardAlive();

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
