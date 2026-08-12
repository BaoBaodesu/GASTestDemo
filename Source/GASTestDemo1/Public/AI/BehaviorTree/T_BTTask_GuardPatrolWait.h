#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "T_BTTask_GuardPatrolWait.generated.h"

/**
 * 巡逻点停留：读取 Guard 实例上的 PatrolPointWaitSeconds / Deviation。
 * 请在 BT_Guard 的 Patrol 分支中用本节点替换原来的 BTTask_Wait（Patrol Pause）。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTTask_GuardPatrolWait : public UBTTaskNode
{
	GENERATED_BODY()

public:

	UT_BTTask_GuardPatrolWait();

	// Guard 无效时的回退等待
	UPROPERTY(EditAnywhere, Category = "Guard|Patrol", meta = (ClampMin = "0.0"))
	float FallbackWaitSeconds{1.f};

	UPROPERTY(EditAnywhere, Category = "Guard|Patrol", meta = (ClampMin = "0.0"))
	float FallbackRandomDeviation{0.5f};

protected:

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:

	float WaitRemaining{0.f};
};
