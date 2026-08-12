// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "T_BTTask_GuardMoveTo.generated.h"

class AAIController;
class UBlackboardComponent;
struct FPathFollowingResult;

/**
 * Guard 移动 Task：读取黑板向量键并请求移动到目标，可观察键变化重新寻路。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTTask_GuardMoveTo : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UT_BTTask_GuardMoveTo();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override;

	// 移动请求结果码到任务结果的映射：AlreadyAtGoal 视为成功，RequestSuccessful 等待完成回调
	static EBTNodeResult::Type MapMoveRequestResult(EPathFollowingRequestResult::Type RequestResult);

	// 目标位置黑板键名
	UPROPERTY(EditAnywhere, Category = "Guard|Move")
	FName MoveToKeyName{TEXT("Move Location")};

	// 到达半径
	UPROPERTY(EditAnywhere, Category = "Guard|Move")
	float AcceptanceRadius{100.f};

	// 移动超时：目标不可达时任务失败，避免永久挂起
	UPROPERTY(EditAnywhere, Category = "Guard|Move")
	float MoveTimeout{10.f};

	// 黑板键变化时重新寻路
	UPROPERTY(EditAnywhere, Category = "Guard|Move")
	bool bObserveBlackboardValue{true};

private:

	void StartMove(UBehaviorTreeComponent& OwnerComp);

	void OnMoveFinished(FAIRequestID RequestID, const FPathFollowingResult& Result);

	EBlackboardNotificationResult OnBlackboardValueChanged(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID);

	TWeakObjectPtr<UBehaviorTreeComponent> OwningComponent;
	FBlackboard::FKey ObservedKeyID;
	FAIRequestID CurrentMoveRequestID;
	float ElapsedTime{0.f};
	bool bWaitingForReturnFacing{false};
};
