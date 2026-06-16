// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/T_WidgetComponent.h"

#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Characters/T_BaseCharacter.h"


void UT_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	InitAbilitySystemData();
	
	if (!IsASCInitialized())
	{
		CrashCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitialized);
		return;
	}
	
	InitializeAttributeDelegate();
}

void UT_WidgetComponent::InitAbilitySystemData()
{
	CrashCharacter = Cast<AT_BaseCharacter>(GetOwner());
	AttributeSet = Cast<UT_AttributeSet>(CrashCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UT_AbilitySystemComponent>(CrashCharacter->GetAbilitySystemComponent());
}

bool UT_WidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

void UT_WidgetComponent::InitializeAttributeDelegate()
{
	if (!AttributeSet->bAttributesInitialized)
	{
		AttributeSet->OnAttributesInitialized.AddDynamic(this, &ThisClass::BindToAttributeChanges);
	}
	else
	{
		BindToAttributeChanges();
	}
}

void UT_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	AbilitySystemComponent = Cast<UT_AbilitySystemComponent>(ASC);
	AttributeSet = Cast<UT_AttributeSet>(AS);
	
	// TODO: 检查属性集是否已使用第一个 GE 进行初始化。
	// 如果没有，则将其绑定到某个将在初始化时进行广播的委托上。
	
	if (!IsASCInitialized()) return;
	InitializeAttributeDelegate();
}
void UT_WidgetComponent::BindToAttributeChanges()
{
	
}

