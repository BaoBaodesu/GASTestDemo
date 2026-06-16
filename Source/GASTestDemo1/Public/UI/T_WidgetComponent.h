// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/WidgetComponent.h"
#include "T_WidgetComponent.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UT_AttributeSet;
class UT_AbilitySystemComponent;
class AT_BaseCharacter;

/**
* 用于绑定 GAS 属性变化和 UI Widget 的组件。
*/
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_WidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:

protected:

	virtual void BeginPlay() override;

	// 属性映射表
	// Key：当前属性
	// Value：最大属性
	UPROPERTY(EditAnywhere)
	TMap<FGameplayAttribute, FGameplayAttribute> AttributeMap;

	
private:
	// 当前组件所属的角色
	TWeakObjectPtr<AT_BaseCharacter> CrashCharacter;
	// 角色的 AbilitySystemComponent
	TWeakObjectPtr<UT_AbilitySystemComponent> AbilitySystemComponent;
	// 角色的 AttributeSet
	TWeakObjectPtr<UT_AttributeSet> AttributeSet;

	// 初始化角色、ASC、AttributeSet 的引用
	void InitAbilitySystemData();
	// 判断 ASC 和 AttributeSet 是否已经有效
	bool IsASCInitialized() const;
	// 初始化属性变化绑定
	void InitializeAttributeDelegate();
	// 把某一个 Widget 绑定到某一组属性变化上
	void BindWidgetToAttributeChanges(UWidget* WidgetObject, const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;
	
	// 当角色 ASC 初始化完成后调用
	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	// 给所有需要监听的属性绑定变化回调
	UFUNCTION()
	void BindToAttributeChanges();
};
