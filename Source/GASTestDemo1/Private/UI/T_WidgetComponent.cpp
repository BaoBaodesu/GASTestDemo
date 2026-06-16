// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/T_WidgetComponent.h"

#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/T_BaseCharacter.h"
#include "UI/T_AttributeWidget.h"


void UT_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();
	
	InitAbilitySystemData();
	
	if (!IsASCInitialized())
	{
		if (CrashCharacter.IsValid())
		{
			CrashCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitialized);
		}
		return;
	}
	
	InitializeAttributeDelegate();
}

/**
* 初始化 GAS 相关数据
*/
void UT_WidgetComponent::InitAbilitySystemData()
{
	CrashCharacter = Cast<AT_BaseCharacter>(GetOwner());
	if (!CrashCharacter.IsValid()) return;
	
	AttributeSet = Cast<UT_AttributeSet>(CrashCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UT_AbilitySystemComponent>(CrashCharacter->GetAbilitySystemComponent());
}

/**
 * 判断 ASC 和 AttributeSet 是否已经有效
 *
 * 只有这两个都有效，才能安全绑定属性变化
 */
bool UT_WidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

/**
 * 初始化属性变化委托。
 *
 * 如果 AttributeSet 的属性还没有初始化，
 * 就等 AttributeSet 初始化完成后再绑定。
 *
 * 如果已经初始化过了，就直接绑定属性变化。
 */
void UT_WidgetComponent::InitializeAttributeDelegate()
{
	// BindToAttributeChanges();
	if (!AttributeSet->bAttributesInitialized)
	{
		// 等待 AttributeSet 初始化完成
		AttributeSet->OnAttributesInitialized.AddDynamic(this, &ThisClass::BindToAttributeChanges);
	}
	else
	{
		// AttributeSet 已经初始化，直接绑定
		BindToAttributeChanges();
	}
}

/**
 * 当角色 ASC 初始化完成后调用。
 *
 * 这个函数会接收角色广播出来的：
 * - AbilitySystemComponent
 * - AttributeSet
 *
 * 然后继续执行属性绑定流程。
 */
void UT_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	// 保存 ASC 引用
	AbilitySystemComponent = Cast<UT_AbilitySystemComponent>(ASC);
	// 保存 AttributeSet 引用
	AttributeSet = Cast<UT_AttributeSet>(AS);
	
	if (!IsASCInitialized()) return;
	// 继续初始化属性变化绑定
	InitializeAttributeDelegate();
}

/**
 * 绑定所有属性变化。
 *
 * 会遍历 AttributeMap 中配置的属性组。
 *
 * 例如：
 * Health -> MaxHealth
 * Mana -> MaxMana
 *
 * 然后在 WidgetComponent 的 UserWidget 以及它的所有子控件中，
 * 查找 UT_AttributeWidget，并绑定对应属性变化。
 */
void UT_WidgetComponent::BindToAttributeChanges()
{
	for (const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair : AttributeMap)
	{
		// 先检查根 Widget 本身是否是 UT_AttributeWidget
		BindWidgetToAttributeChanges(GetUserWidgetObject(), Pair);
		
		// 遍历根 Widget 下面的所有子 Widget
		GetUserWidgetObject()->WidgetTree->ForEachWidget([this, &Pair](UWidget* ChildWidget)
		{
			// 尝试把子 Widget 绑定到属性变化
			BindWidgetToAttributeChanges(ChildWidget, Pair);
		});
	}
	
}

/**
 * 尝试把某个 Widget 绑定到某一组属性变化。
 *
 * 只有满足下面条件才会绑定：
 * 1. Widget 是 UT_AttributeWidget 类型
 * 2. Widget 设置的 Attribute / MaxAttribute 和 Pair 匹配
 *
 * 绑定成功后：
 * - 先主动刷新一次 UI
 * - 再监听属性变化，后续属性变化时自动刷新 UI
 */
void UT_WidgetComponent::BindWidgetToAttributeChanges(UWidget* WidgetObject,
	const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	// 判断这个 Widget 是否是 UT_AttributeWidget
	UT_AttributeWidget* AttributeWidget = Cast<UT_AttributeWidget>(WidgetObject);
	if (!IsValid(AttributeWidget)) return;
	// 判断这个 Widget 绑定的属性是否和当前 Pair 一致
	if (!AttributeWidget->MatchesAttributes(Pair)) return;
	
	// 先主动刷新一次 UI，确保初始显示正确
	AttributeWidget->OnAttributeChange(Pair, AttributeSet.Get());
	
	// 监听当前属性的变化，例如监听 Health 变化
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key)
	.AddLambda([this, AttributeWidget, Pair](const FOnAttributeChangeData& AttributeChangeData)
	{
		// 属性变化后，重新读取当前值和最大值，并更新 UI
		AttributeWidget->OnAttributeChange(Pair, AttributeSet.Get());
	});
}
