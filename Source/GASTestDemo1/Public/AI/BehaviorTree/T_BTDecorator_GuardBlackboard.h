// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AI/T_ShooterAIController.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "T_BTDecorator_GuardBlackboard.generated.h"

UENUM(BlueprintType)
enum class EGuardBlackboardCondition : uint8
{
	// 键已设置
	IsSet,
	// 键未设置
	IsNotSet,
	// 布尔键为 true
	IsTrue,
	// 布尔键为 false
	IsFalse,
	// Enum 键等于指定状态
	EnumEquals,
	// Enum 键不等于指定状态
	EnumNotEquals
};

/**
 * Guard 黑板条件装饰器：按键名与条件判断，派生自引擎黑板装饰器基类以获得键变化观察。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTDecorator_GuardBlackboard : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:

	UT_BTDecorator_GuardBlackboard();

	// 供行为树构建工具设置黑板键名（基类 BlackboardKey 为 protected）
	void SetBlackboardKeyName(const FName& KeyName) { BlackboardKey.SelectedKeyName = KeyName; }

	// 供行为树构建工具设置中止模式（基类 FlowAbortMode 为 protected）
	void SetFlowAbortMode(EBTFlowAbortMode::Type AbortMode) { FlowAbortMode = AbortMode; }

	// 条件类型
	UPROPERTY(EditAnywhere, Category = "Guard|Blackboard")
	EGuardBlackboardCondition Condition{EGuardBlackboardCondition::IsSet};

	UPROPERTY(EditAnywhere, Category = "Guard|Blackboard", meta = (EditCondition = "Condition == EGuardBlackboardCondition::EnumEquals || Condition == EGuardBlackboardCondition::EnumNotEquals", EditConditionHides))
	ETGuardAIState EnumValue{ETGuardAIState::Patrol};

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
