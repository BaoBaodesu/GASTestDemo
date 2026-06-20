// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "T_AttributeChangeTask.generated.h"

struct FOnAttributeChangeData;
class UAbilitySystemComponent;
/**
 * 蓝图异步任务： 监听 GAS Attribute 的变化。
 * 用途：
 * - UI 监听 Health / Mana / Stamina
 * - 属性变化时通知蓝图更新显示
 * - 不再需要监听时调用 EndTask 解绑
 */
// 当指定 Attribute 发生变化时广播。
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAttributeChanged, FGameplayAttribute, Attribute, float, NewValue, float, OldValue);

UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class GASTESTDEMO1_API UT_AttributeChangeTask : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
	
public:
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChanged OnAttributeChange;
	
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"))
	static UT_AttributeChangeTask* ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute);
	
	UFUNCTION(BlueprintCallable)
	void EndTask();
	
	TWeakObjectPtr<UAbilitySystemComponent> ASC;
	FGameplayAttribute AttributeToListenFor;
	
	void AttributeChanged(const FOnAttributeChangeData& Data);
};
