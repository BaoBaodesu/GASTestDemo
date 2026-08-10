// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "T_GuardAmmoLibrary.generated.h"

class UT_AttributeSet;

/**
 * Guard 弹药规则：射击扣 1 发弹匣弹药；换弹量 = Min(MaxMagazineAmmo - MagazineAmmo, ReserveAmmo)。
 */
UCLASS()
class GASTESTDEMO1_API UT_GuardAmmoLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	// 计算当前换弹量
	UFUNCTION(BlueprintPure, Category = "Guard|Ammo")
	static float GetReloadAmount(const UT_AttributeSet* AttributeSet);

	// 纯数学版本：换弹量 = Min(MaxMagazineAmmo - MagazineAmmo, ReserveAmmo)，均不小于 0
	static float GetReloadAmount(float MagazineAmmo, float MaxMagazineAmmo, float ReserveAmmo);

	// 射击扣除 1 发弹匣弹药；弹匣为空时返回 false 且不扣除
	UFUNCTION(BlueprintCallable, Category = "Guard|Ammo")
	static bool ApplyShotCost(UT_AttributeSet* AttributeSet);

	// 换弹：仅在动画成功结束时调用，按换弹量把备弹转移到弹匣
	UFUNCTION(BlueprintCallable, Category = "Guard|Ammo")
	static bool ApplyReload(UT_AttributeSet* AttributeSet);
};
