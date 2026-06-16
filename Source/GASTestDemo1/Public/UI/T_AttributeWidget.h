// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/T_AttributeSet.h"

#include "T_AttributeWidget.generated.h"

/**
 * 用于显示 GAS 属性的 Widget 基类。
 */
UCLASS()
class GASTESTDEMO1_API UT_AttributeWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 当前属性
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|Attributes")
	FGameplayAttribute Attribute;

	// 当前属性对应的最大值
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|Attributes")
	FGameplayAttribute MaxAttribute;
	
	// 当属性发生变化时调用，用来读取当前属性值和最大属性值
	void OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UT_AttributeSet* AttributeSet);
	// 判断传入的属性组合是否和当前 Widget 绑定的属性一致
	bool MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;
	
	// 蓝图实现事件：把新属性值传给蓝图，让蓝图更新血条、蓝条等显示
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Attribute Change"))
	void BP_OnAttributeChange(float NewValue, float NewMaxValue);
};
