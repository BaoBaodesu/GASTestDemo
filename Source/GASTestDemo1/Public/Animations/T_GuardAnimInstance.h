#pragma once

#include "CoreMinimal.h"
#include "Animations/T_PlayerAnimInstance.h"
#include "T_GuardAnimInstance.generated.h"

/**
 * Guard 射击敌人动画实例：沿用玩家 AnimInstance 数据，默认始终保持瞄准姿势。
 * 当前 ABP_Shooter 仍继承引擎 AnimInstance；举枪由 AT_GuardCharacter 同步 ABP 的 IsAiming。
 */
UCLASS()
class GASTESTDEMO1_API UT_GuardAnimInstance : public UT_PlayerAnimInstance
{
	GENERATED_BODY()

public:

	UT_GuardAnimInstance();
};
