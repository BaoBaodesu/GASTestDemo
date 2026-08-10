// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "T_BTService_GuardUpdateSight.generated.h"

/**
 * Guard 视线更新 Service：战斗中刷新敌人/已发现/移动位置黑板键。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTService_GuardUpdateSight : public UBTService
{
	GENERATED_BODY()

public:

	UT_BTService_GuardUpdateSight();

protected:

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
