// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "T_BTTask_GuardClearBlackboard.generated.h"

/**
 * Guard 清理黑板 Task：清除目标状态与 Enemy/Enemy Spotted/Move Location/Noise Location 键。
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
