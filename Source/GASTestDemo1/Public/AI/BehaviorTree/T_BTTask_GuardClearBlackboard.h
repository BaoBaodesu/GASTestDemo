// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "T_BTTask_GuardClearBlackboard.generated.h"

/**
 * Guard 状态完成 Task：按当前状态清理临时键并推进状态，永远保留 Home Location。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTTask_GuardClearBlackboard : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UT_BTTask_GuardClearBlackboard();

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
