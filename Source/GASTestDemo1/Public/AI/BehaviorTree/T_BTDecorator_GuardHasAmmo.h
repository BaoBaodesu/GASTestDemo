// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "T_BTDecorator_GuardHasAmmo.generated.h"

UENUM(BlueprintType)
enum class EGuardAmmoCheck : uint8
{
	// 弹匣内有子弹
	HasLoadedAmmo,
	// 弹匣为空
	MagazineEmpty,
	// 弹匣未满（需要换弹）
	NeedsReload,
	// 有备弹
	HasReserveAmmo
};

/**
 * Guard 弹药装饰器：根据弹药状态控制射击/换弹分支。
 */
UCLASS()
class GASTESTDEMO1_API UT_BTDecorator_GuardHasAmmo : public UBTDecorator
{
	GENERATED_BODY()

public:

	UT_BTDecorator_GuardHasAmmo();

	// 弹药检查类型
	UPROPERTY(EditAnywhere, Category = "Guard|Ammo")
	EGuardAmmoCheck AmmoCheck{EGuardAmmoCheck::HasLoadedAmmo};

protected:

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
};
