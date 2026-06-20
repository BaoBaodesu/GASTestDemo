// Fill out your copyright notice in the Description page of Project Settings.


#include "Tasks/T_AttributeChangeTask.h"

#include "AbilitySystemComponent.h"

/**
 * 创建并启动一个属性变化监听任务。
 */
UT_AttributeChangeTask* UT_AttributeChangeTask::ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute)
{
	UT_AttributeChangeTask* WaitForAttributeChangeTask = NewObject<UT_AttributeChangeTask>();
	WaitForAttributeChangeTask->ASC = AbilitySystemComponent;
	WaitForAttributeChangeTask->AttributeToListenFor = Attribute;

	// 如果传入的 ASC 无效，直接返回 nullptr
	if (!IsValid(AbilitySystemComponent))
	{
		WaitForAttributeChangeTask->RemoveFromRoot();
		return nullptr;
	}
	
	// 绑定指定 Attribute 的变化委托
	// 当这个 Attribute 的值发生变化时，会调用 AttributeChanged()
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(WaitForAttributeChangeTask, &UT_AttributeChangeTask::AttributeChanged);
	
	// 返回任务对象给蓝图
	return WaitForAttributeChangeTask;
}

void UT_AttributeChangeTask::EndTask()
{
	if (ASC.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeToListenFor).RemoveAll(this);
	}
	
	// 通知引擎这个异步任务可以销毁
	SetReadyToDestroy();
	// 标记为垃圾对象，等待 GC 回收
	MarkAsGarbage();
}

void UT_AttributeChangeTask::AttributeChanged(const FOnAttributeChangeData& Data)
{
	// 广播给蓝图，让蓝图可以更新 UI
	OnAttributeChange.Broadcast(Data.Attribute, Data.NewValue, Data.OldValue);
}
