#include "UI/WeaponState/UT_EquipmentCardWidget.h"

#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "GameplayTags/TTags.h"
#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"
#include "UObject/UObjectGlobals.h"

void UT_EquipmentCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 运行时创建命中标记图片（资产控件树无法脚本化修改，运行时构建效果一致）
	if (IsValid(WidgetTree) && !IsValid(HitMark))
	{
		UImage* NewHitMark = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("HitMark"));
		if (IsValid(NewHitMark))
		{
			NewHitMark->SetVisibility(ESlateVisibility::Collapsed);
			NewHitMark->SetDesiredSizeOverride(FVector2D(48.f, 48.f));
			UTexture2D* HitMarkerTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/VisualSandbox/2DAssets/Reticles/T_HitMarker.T_HitMarker"));
			if (IsValid(HitMarkerTexture)) NewHitMark->SetBrushFromTexture(HitMarkerTexture, true);

			UPanelWidget* HostPanel = nullptr;
			for (UWidget* Widget : FindWidgetsByPrefix(TEXT("ItemCard")))
			{
				if (UImage* ItemImage = Cast<UImage>(Widget))
				{
					HostPanel = ItemImage->GetParent();
					if (IsValid(HostPanel)) break;
				}
			}
			if (IsValid(HostPanel))
			{
				HostPanel->AddChild(NewHitMark);
				if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(NewHitMark->Slot))
				{
					OverlaySlot->SetHorizontalAlignment(HAlign_Center);
					OverlaySlot->SetVerticalAlignment(VAlign_Center);
				}
				HitMark = NewHitMark;
			}
		}
	}

	ItemCard = Cast<UImage>(WidgetTree->FindWidget(TEXT("ItemCard")));
	ItemName = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ItemName")));
	ReserveAmount = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ReserveAmount")));
	AmmunitionEmpty = Cast<UImage>(WidgetTree->FindWidget(TEXT("AmmunitionEmpty")));
	if (IsValid(AmmunitionEmpty)) AmmunitionEmpty->SetVisibility(ESlateVisibility::Collapsed);

	BindToCharacter();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AmmoRefreshTimer, this, &ThisClass::RefreshAmmoFromTimer, 0.2f, true);
	}
}

void UT_EquipmentCardWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HitMarkTimer);
		World->GetTimerManager().ClearTimer(AmmoRefreshTimer);
	}

	if (IsValid(Inventory))
	{
		Inventory->OnEquippedItemChanged.RemoveDynamic(this, &ThisClass::OnEquippedItemChanged);
		Inventory = nullptr;
	}

	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UT_AttributeSet::GetReserveAmmoAttribute()).RemoveAll(this);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UT_AttributeSet::GetMagazineAmmoAttribute()).RemoveAll(this);
		if (HitEventDelegateHandle.IsValid())
		{
			FGameplayTagContainer HitTagContainer;
			HitTagContainer.AddTag(TTags::Events::Player::Shoot::Hit);
			AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(HitTagContainer, HitEventDelegateHandle);
			HitEventDelegateHandle.Reset();
		}
		AbilitySystemComponent = nullptr;
	}

	if (IsValid(PlayerCharacter))
	{
		PlayerCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnPlayerASCInitialized);
		PlayerCharacter = nullptr;
	}

	Super::NativeDestruct();
}

void UT_EquipmentCardWidget::BindToCharacter()
{
	PlayerCharacter = Cast<AT_PlayerCharacter>(GetOwningPlayerPawn());
	if (!IsValid(PlayerCharacter)) return;

	Inventory = PlayerCharacter->GetInventoryComponent();
	if (IsValid(Inventory))
	{
		Inventory->OnEquippedItemChanged.RemoveDynamic(this, &ThisClass::OnEquippedItemChanged);
		Inventory->OnEquippedItemChanged.AddUniqueDynamic(this, &ThisClass::OnEquippedItemChanged);
	}

	AbilitySystemComponent = PlayerCharacter->GetAbilitySystemComponent();
	AttributeSet = Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet());
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UT_AttributeSet::GetReserveAmmoAttribute()).AddUObject(this, &ThisClass::OnReserveAmmoChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UT_AttributeSet::GetMagazineAmmoAttribute()).AddUObject(this, &ThisClass::OnMagazineAmmoChanged);
		BindHitMarkEvents();
	}
	else
	{
		PlayerCharacter->OnASCInitialized.AddUniqueDynamic(this, &ThisClass::OnPlayerASCInitialized);
	}

	UpdateEquipmentDisplay(IsValid(Inventory) ? Inventory->GetEquippedItem() : nullptr);
}

void UT_EquipmentCardWidget::OnPlayerASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	if (!IsValid(ASC) || IsValid(AbilitySystemComponent)) return;

	AbilitySystemComponent = ASC;
	AttributeSet = Cast<UT_AttributeSet>(AS);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UT_AttributeSet::GetReserveAmmoAttribute()).AddUObject(this, &ThisClass::OnReserveAmmoChanged);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UT_AttributeSet::GetMagazineAmmoAttribute()).AddUObject(this, &ThisClass::OnMagazineAmmoChanged);
	BindHitMarkEvents();

	if (IsValid(PlayerCharacter)) PlayerCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnPlayerASCInitialized);
	RefreshAmmoText();
}

void UT_EquipmentCardWidget::BindHitMarkEvents()
{
	if (!IsValid(AbilitySystemComponent) || HitEventDelegateHandle.IsValid()) return;

	FGameplayTagContainer HitTagContainer;
	HitTagContainer.AddTag(TTags::Events::Player::Shoot::Hit);
	HitEventDelegateHandle = AbilitySystemComponent->AddGameplayEventTagContainerDelegate(
		HitTagContainer,
		FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::OnHitMarkEvent));
}

void UT_EquipmentCardWidget::OnHitMarkEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (EventTag != TTags::Events::Player::Shoot::Hit) return;
	if (!IsValid(HitMark)) return;

	// 命中确认（仅命中 Pawn 且造成伤害时由服务器触发 Shoot::Hit）：短暂显示命中标记
	HitMark->SetVisibility(ESlateVisibility::Visible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(HitMarkTimer, this, &ThisClass::HideHitMark, 0.18f, false);
	}
}

void UT_EquipmentCardWidget::HideHitMark()
{
	if (IsValid(HitMark)) HitMark->SetVisibility(ESlateVisibility::Collapsed);
}

void UT_EquipmentCardWidget::OnEquippedItemChanged(UT_ItemDefinition* ItemDefinition)
{
	UpdateEquipmentDisplay(ItemDefinition);
}

void UT_EquipmentCardWidget::OnReserveAmmoChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	RefreshAmmoText();
}

void UT_EquipmentCardWidget::OnMagazineAmmoChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	RefreshAmmoText();
}

void UT_EquipmentCardWidget::UpdateEquipmentDisplay(UT_ItemDefinition* ItemDefinition)
{
	const bool bHasEquipment = IsValid(ItemDefinition);
	SetVisibility(bHasEquipment ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	// 兼容迁移残留的同名重复控件：对所有匹配名称的控件一并更新
	const FText NameText = bHasEquipment ? ItemDefinition->DisplayName : FText::GetEmpty();
	for (UWidget* Widget : FindWidgetsByPrefix(TEXT("ItemCard")))
	{
		if (UImage* Image = Cast<UImage>(Widget))
		{
			Image->SetBrushFromTexture(bHasEquipment ? ItemDefinition->Thumbnail : nullptr, true);
		}
	}
	for (UWidget* Widget : FindWidgetsByPrefix(TEXT("ItemName")))
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			Text->SetText(NameText);
		}
	}
	if (bHasEquipment) RefreshAmmoText();
}

void UT_EquipmentCardWidget::RefreshAmmoFromTimer()
{
	// 射击/换弹直接写 FGameplayAttributeData，单机 PIE 下不会触发属性变化委托，定时兜底刷新
	if (GetVisibility() == ESlateVisibility::Visible) RefreshAmmoText();
}

void UT_EquipmentCardWidget::RefreshAmmoText()
{
	if (!IsValid(AttributeSet)) return;

	const int32 MagazineAmmo = FMath::FloorToInt(AttributeSet->GetMagazineAmmo());
	const int32 ReserveAmmo = FMath::FloorToInt(AttributeSet->GetReserveAmmo());
	const FText AmmoText = FText::Format(
		FText::FromString(TEXT("{0} / {1}")),
		FText::AsNumber(MagazineAmmo),
		FText::AsNumber(ReserveAmmo));
	for (UWidget* Widget : FindWidgetsByPrefix(TEXT("ReserveAmount")))
	{
		if (UTextBlock* Text = Cast<UTextBlock>(Widget))
		{
			Text->SetText(AmmoText);
		}
	}

	// 仅弹匣打空时显示空弹提示图（与射击空膛判定一致）
	const ESlateVisibility EmptyVisibility = MagazineAmmo <= 0
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;
	for (UWidget* Widget : FindWidgetsByPrefix(TEXT("AmmunitionEmpty")))
	{
		Widget->SetVisibility(EmptyVisibility);
	}
}

TArray<UWidget*> UT_EquipmentCardWidget::FindWidgetsByPrefix(const FString& Prefix) const
{
	TArray<UWidget*> Result;
	if (!IsValid(WidgetTree)) return Result;

	WidgetTree->ForEachWidgetAndDescendants([&](UWidget* Widget)
	{
		if (IsValid(Widget) && Widget->GetName().StartsWith(Prefix))
		{
			Result.Add(Widget);
		}
	});
	return Result;
}
