// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/T_AttributeWidget.h"
/**
* 当属性变化时调用
* Pair.Key 是当前属性
* Pair.Value 是最大属性
*/
 
void UT_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UT_AttributeSet* AttributeSet)
{
	const float AttributeValue = Pair.Key.GetNumericValue(AttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(AttributeSet);
	
	BP_OnAttributeChange(AttributeValue, MaxAttributeValue);
}

/**
* 判断当前 Widget 绑定的属性是否和传入的属性组合一致
*/
bool UT_AttributeWidget::MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	return Attribute == Pair.Key && MaxAttribute == Pair.Value;
}
