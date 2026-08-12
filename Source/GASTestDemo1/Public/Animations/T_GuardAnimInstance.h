#pragma once

#include "CoreMinimal.h"
#include "Animations/T_PlayerAnimInstance.h"
#include "T_GuardAnimInstance.generated.h"

/**
 * Guard 射击敌人动画实例：沿用玩家 AnimInstance 数据，仅在警觉状态保持瞄准姿势。
 * 巡逻/返回时 bAlwaysAiming=false；Suspicious/Investigate/Combat/Search 时为 true。
 */
UCLASS()
class GASTESTDEMO1_API UT_GuardAnimInstance : public UT_PlayerAnimInstance
{
	GENERATED_BODY()

public:

	UT_GuardAnimInstance();
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
