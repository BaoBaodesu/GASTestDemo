// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "T_BTService_GuardFaceTarget.generated.h"

/**
 * Guard 朝向目标 Service：战斗中持续让控制器 Focus 当前目标。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTService_GuardFaceTarget : public UBTService
{
	GENERATED_BODY()

public:

	UT_BTService_GuardFaceTarget();

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
