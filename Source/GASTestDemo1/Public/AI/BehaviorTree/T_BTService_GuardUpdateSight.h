// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "T_BTService_GuardUpdateSight.generated.h"

/**
 * Guard 视线更新 Service：只同步控制器已经确认的目标与最后已知位置，不计算警觉度。
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
