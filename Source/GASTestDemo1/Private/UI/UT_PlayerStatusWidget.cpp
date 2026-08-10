#include "UI/UT_PlayerStatusWidget.h"

#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/PanelWidget.h"
#include "UI/T_AttributeWidget.h"

void UT_PlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BindToCharacter();
}

void UT_PlayerStatusWidget::NativeDestruct()
{
	if (IsValid(BoundAbilitySystemComponent))
	{
		for (const FGameplayAttribute& Key : BoundAttributeKeys)
		{
			BoundAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Key).RemoveAll(this);
		}
		BoundAttributeKeys.Reset();
	}

	if (IsValid(BoundPlayerCharacter))
	{
		BoundPlayerCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnPlayerASCInitialized);
		BoundPlayerCharacter = nullptr;
	}

	BoundAbilitySystemComponent = nullptr;
	BoundAttributeSet = nullptr;

	Super::NativeDestruct();
}

void UT_PlayerStatusWidget::BindToCharacter()
{
	BoundPlayerCharacter = Cast<AT_PlayerCharacter>(GetOwningPlayerPawn());
	if (!IsValid(BoundPlayerCharacter)) return;

	BoundAbilitySystemComponent = BoundPlayerCharacter->GetAbilitySystemComponent();
	BoundAttributeSet = Cast<UT_AttributeSet>(BoundPlayerCharacter->GetAttributeSet());
	if (!IsValid(BoundAbilitySystemComponent) || !IsValid(BoundAttributeSet))
	{
		BoundPlayerCharacter->OnASCInitialized.AddUniqueDynamic(this, &ThisClass::OnPlayerASCInitialized);
		return;
	}

	BindAttributeWidgets();
}

void UT_PlayerStatusWidget::OnPlayerASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	if (!IsValid(ASC) || IsValid(BoundAbilitySystemComponent)) return;

	BoundAbilitySystemComponent = ASC;
	BoundAttributeSet = Cast<UT_AttributeSet>(AS);
	if (!IsValid(BoundAbilitySystemComponent) || !IsValid(BoundAttributeSet)) return;

	if (IsValid(BoundPlayerCharacter)) BoundPlayerCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnPlayerASCInitialized);
	BindAttributeWidgets();
}

void UT_PlayerStatusWidget::BindAttributeWidgets()
{
	if (!IsValid(WidgetTree) || !IsValid(BoundAbilitySystemComponent) || !IsValid(BoundAttributeSet) || !IsValid(BoundPlayerCharacter)) return;

	// 先清理旧绑定，避免重复初始化时叠加
	for (const FGameplayAttribute& Key : BoundAttributeKeys)
	{
		BoundAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Key).RemoveAll(this);
	}
	BoundAttributeKeys.Reset();

	// 递归遍历：先对控件实例本身（包括 UserWidget 实例）做属性控件匹配，
	// 再下钻 Panel 子控件与 UserWidget 自己的 WidgetTree。
	// 不能只用 ForEachWidgetAndDescendants：引擎实现会跳过带 WidgetTree 的子 UserWidget 实例本身。
	TFunction<void(UWidget*)> VisitWidget = [&](UWidget* Widget)
	{
		if (UT_AttributeWidget* AttributeWidget = Cast<UT_AttributeWidget>(Widget))
		{
			const FGameplayAttribute Attribute = AttributeWidget->Attribute;
			const FGameplayAttribute MaxAttribute = AttributeWidget->MaxAttribute;
			if (Attribute.IsValid() && MaxAttribute.IsValid())
			{
				AttributeWidget->AvatarActor = BoundPlayerCharacter;
				AttributeWidget->OnAttributeChange(TTuple<FGameplayAttribute, FGameplayAttribute>(Attribute, MaxAttribute), BoundAttributeSet, 0.f);

				BoundAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute)
					.AddUObject(this, &ThisClass::OnAttributeChanged, TWeakObjectPtr<UT_AttributeWidget>(AttributeWidget), Attribute, MaxAttribute);
				BoundAttributeKeys.Add(Attribute);
			}
		}

		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
			{
				VisitWidget(Panel->GetChildAt(ChildIndex));
			}
		}
		else if (UUserWidget* UserWidgetChild = Cast<UUserWidget>(Widget))
		{
			if (IsValid(UserWidgetChild->WidgetTree) && IsValid(UserWidgetChild->WidgetTree->RootWidget))
			{
				VisitWidget(UserWidgetChild->WidgetTree->RootWidget);
			}
		}
	};

	if (IsValid(WidgetTree->RootWidget))
	{
		VisitWidget(WidgetTree->RootWidget);
	}
}

void UT_PlayerStatusWidget::OnAttributeChanged(const FOnAttributeChangeData& AttributeChangeData, TWeakObjectPtr<UT_AttributeWidget> Widget, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute)
{
	UT_AttributeWidget* AttributeWidget = Widget.Get();
	if (!IsValid(AttributeWidget) || !IsValid(BoundAttributeSet)) return;

	AttributeWidget->OnAttributeChange(TTuple<FGameplayAttribute, FGameplayAttribute>(Attribute, MaxAttribute), BoundAttributeSet, AttributeChangeData.OldValue);
}
