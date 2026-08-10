// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "T_BTTask_ActivateAbilityByTag.generated.h"

class UAbilitySystemComponent;
struct FAbilityEndedData;

/**
 * 通用能力激活 Task：按能力标签激活，可等待能力结束（射击/换弹），也可只触发（瞄准）。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTTask_ActivateAbilityByTag : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UT_BTTask_ActivateAbilityByTag();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// 需要激活的能力标签
	UPROPERTY(EditAnywhere, Category = "Guard|Ability")
	FGameplayTag AbilityTag;

	// 是否等待能力成功/结束/失败；瞄准为 false，射击/换弹为 true
	UPROPERTY(EditAnywhere, Category = "Guard|Ability")
	bool bWaitForCompletion{true};

	// 等待能力结束的超时保护
	UPROPERTY(EditAnywhere, Category = "Guard|Ability")
	float ActivationTimeout{10.f};

private:

	void FinishTask(UBehaviorTreeComponent& OwnerComp, EBTNodeResult::Type Result);

	void OnAbilityEnded(const FAbilityEndedData& EndedData);

	TWeakObjectPtr<UBehaviorTreeComponent> OwningComponent;
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	float ElapsedTime{0.f};
	bool bWaitingForAbility{false};
};
