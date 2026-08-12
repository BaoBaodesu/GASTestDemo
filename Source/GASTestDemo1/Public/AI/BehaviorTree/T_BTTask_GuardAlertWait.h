// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "T_BTTask_GuardAlertWait.generated.h"

/**
 * Guard 警戒等待 Task：在最后感知位置短暂警戒后完成。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTTask_GuardAlertWait : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UT_BTTask_GuardAlertWait();

	// 警戒时长
	UPROPERTY(EditAnywhere, Category = "Guard|Alert")
	float WaitDuration{3.f};

	UPROPERTY(EditAnywhere, Category = "Guard|Alert", meta = (ClampMin = "0.0"))
	float RandomDeviation{0.f};

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

private:

	float WaitRemaining{0.f};
	float TotalWaitDuration{0.f};
	FVector FocusLocation{ForceInit};
	float BaseSearchYaw{0.f};
	uint8 LookPhase{0};
};
