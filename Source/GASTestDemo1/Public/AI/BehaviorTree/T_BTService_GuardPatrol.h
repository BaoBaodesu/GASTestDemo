// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "T_BTService_GuardPatrol.generated.h"

/**
 * Guard 巡逻点 Service：按角色 PatrolMode 写入 Move Location（站岗/往返/绕圈）。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTService_GuardPatrol : public UBTService
{
	GENERATED_BODY()

public:

	UT_BTService_GuardPatrol();

	// 巡逻点写入的黑板键
	UPROPERTY(EditAnywhere, Category = "Guard|Patrol")
	FBlackboardKeySelector MoveLocationKey;

protected:

	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:

	void PickPatrolLocation(UBehaviorTreeComponent& OwnerComp);
};
