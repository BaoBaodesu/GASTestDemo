#include "UI/Inventory/T_InventoryWidgets.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/WrapBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Input/Reply.h"
#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"
#include "Player/T_PlayerController.h"

void UT_InventoryDraggingWidget::SetItem(UT_InventoryComponent* InventoryComponent, int32 SlotIndex)
{
	if (!IsValid(ItemQuantity) && IsValid(WidgetTree))
	{
		ItemQuantity = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Amount")));
		if (!IsValid(ItemQuantity)) ItemQuantity = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ItemAmount")));
	}

	const FTInventoryStack ItemStack = IsValid(InventoryComponent) ? InventoryComponent->GetSlot(SlotIndex) : FTInventoryStack();
	if (IsValid(ItemImage)) ItemImage->SetBrushFromTexture(ItemStack.IsEmpty() ? nullptr : ItemStack.ItemDefinition->Thumbnail);
	if (IsValid(ItemQuantity)) ItemQuantity->SetText(ItemStack.IsEmpty() ? FText::GetEmpty() : FText::AsNumber(ItemStack.Quantity));
}

void UT_InventoryActionMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(UseButton)) UseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleUse);
	if (IsValid(EquipButton)) EquipButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleEquip);
	if (IsValid(DiscardButton)) DiscardButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDiscard);
	if (IsValid(SplitActionButton)) SplitActionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSplit);
	if (IsValid(SplitButton)) SplitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSplit);
}

void UT_InventoryActionMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bRefreshMenuPosition) return;
	bRefreshMenuPosition = false;
	SetMenuPosition(PendingAnchorGeometry);
}

void UT_InventoryActionMenuWidget::SetSelection(UT_InventoryComponent* InventoryComponent, int32 SlotIndex, const FGeometry& AnchorGeometry)
{
	SelectedInventory = InventoryComponent;
	SelectedSlotIndex = SlotIndex;
	const FTInventoryStack ItemStack = IsValid(SelectedInventory) ? SelectedInventory->GetSlot(SelectedSlotIndex) : FTInventoryStack();
	const bool bCanUseSelectedItem = !ItemStack.IsEmpty()
		&& ItemStack.ItemDefinition->bCanUse
		&& ItemStack.ItemDefinition->ItemType != ETItemType::Weapon;
	if (IsValid(UseButton)) UseButton->SetIsEnabled(bCanUseSelectedItem);
	if (IsValid(EquipButton)) EquipButton->SetIsEnabled(!ItemStack.IsEmpty() && IsValid(ItemStack.ItemDefinition->EquippedActorClass));
	if (IsValid(SplitButton)) SplitButton->SetIsEnabled(!ItemStack.IsEmpty() && ItemStack.Quantity > 1);
	if (IsValid(SplitActionButton)) SplitActionButton->SetIsEnabled(!ItemStack.IsEmpty() && ItemStack.Quantity > 1);
	if (IsValid(DiscardButton)) DiscardButton->SetIsEnabled(!ItemStack.IsEmpty());
	PendingAnchorGeometry = AnchorGeometry;
	bRefreshMenuPosition = true;
	SetVisibility(ESlateVisibility::Visible);
	InvalidateLayoutAndVolatility();
	if (UPanelWidget* ParentPanel = GetParent()) ParentPanel->ForceLayoutPrepass();
	ForceLayoutPrepass();
	SetMenuPosition(AnchorGeometry);
}

void UT_InventoryActionMenuWidget::SetMenuPosition(const FGeometry& AnchorGeometry)
{
	UPanelWidget* ParentPanel = GetParent();
	if (!IsValid(ParentPanel)) return;

	ParentPanel->ForceLayoutPrepass();
	ForceLayoutPrepass();

	const FGeometry ParentGeometry = ParentPanel->GetCachedGeometry();
	const FVector2D ParentSize = ParentGeometry.GetLocalSize();
	const FVector2D MenuSize = GetDesiredSize();
	const FVector2D AnchorTopLeft = ParentGeometry.AbsoluteToLocal(AnchorGeometry.GetAbsolutePosition());
	const FVector2D AnchorTopRight = ParentGeometry.AbsoluteToLocal(
		AnchorGeometry.GetAbsolutePositionAtCoordinates(FVector2D(1.f, 0.f)));
	constexpr float MenuPadding = 8.f;

	FVector2D MenuPosition(AnchorTopRight.X + MenuPadding, AnchorTopRight.Y);

	if (MenuPosition.X + MenuSize.X > ParentSize.X)
	{
		MenuPosition.X = AnchorTopLeft.X - MenuSize.X - MenuPadding;
	}

	MenuPosition.X = FMath::Clamp(MenuPosition.X, 0.f, FMath::Max(0.f, ParentSize.X - MenuSize.X));
	MenuPosition.Y = FMath::Clamp(MenuPosition.Y, 0.f, FMath::Max(0.f, ParentSize.Y - MenuSize.Y));

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f));
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetPosition(MenuPosition);
		CanvasSlot->SetZOrder(100);
		SetRenderTranslation(FVector2D::ZeroVector);
		return;
	}

	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Slot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Left);
		OverlaySlot->SetVerticalAlignment(VAlign_Top);
		OverlaySlot->SetPadding(FMargin(0.f));
		SetRenderTranslation(MenuPosition);
		return;
	}

	const FVector2D CurrentTranslation = GetRenderTransform().Translation;
	const FVector2D CurrentLocalPosition = ParentGeometry.AbsoluteToLocal(GetCachedGeometry().GetAbsolutePosition());
	SetRenderTranslation(MenuPosition - (CurrentLocalPosition - CurrentTranslation));
}

void UT_InventoryActionMenuWidget::ClearSelection()
{
	SelectedInventory = nullptr;
	SelectedSlotIndex = INDEX_NONE;
	bRefreshMenuPosition = false;
	SetVisibility(ESlateVisibility::Collapsed);
}

void UT_InventoryActionMenuWidget::HandleUse()
{
	if (IsValid(SelectedInventory)) SelectedInventory->UseItem(SelectedSlotIndex);
	ClearSelection();
}

void UT_InventoryActionMenuWidget::HandleEquip()
{
	if (IsValid(SelectedInventory)) SelectedInventory->EquipItem(SelectedSlotIndex);
	ClearSelection();
}

void UT_InventoryActionMenuWidget::HandleDiscard()
{
	if (IsValid(SelectedInventory))
	{
		const FTInventoryStack ItemStack = SelectedInventory->GetSlot(SelectedSlotIndex);
		if (!ItemStack.IsEmpty()) SelectedInventory->DropItem(SelectedSlotIndex, ItemStack.Quantity);
	}
	ClearSelection();
}

void UT_InventoryActionMenuWidget::HandleSplit()
{
	if (IsValid(SelectedInventory))
	{
		const FTInventoryStack ItemStack = SelectedInventory->GetSlot(SelectedSlotIndex);
		if (!ItemStack.IsEmpty()) SelectedInventory->SplitStack(SelectedSlotIndex, ItemStack.Quantity / 2);
	}
	ClearSelection();
}

void UT_InventorySlotWidget::InitializeSlot(UT_InventoryComponent* InventoryComponent, int32 SlotIndex, UT_InventoryActionMenuWidget* ActionMenu)
{
	Inventory = InventoryComponent;
	InventorySlotIndex = SlotIndex;
	ActionMenuWidget = ActionMenu;
	RefreshSlot();
}

void UT_InventorySlotWidget::RefreshSlot()
{
	if (!IsValid(ItemQuantity) && IsValid(WidgetTree))
	{
		// WBP_Inventory_Slot1 等资源使用 ItemAmount，兼容旧 ItemQuantity / Amount 命名
		ItemQuantity = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("ItemAmount")));
		if (!IsValid(ItemQuantity)) ItemQuantity = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Amount")));
	}

	const FTInventoryStack ItemStack = IsValid(Inventory) ? Inventory->GetSlot(InventorySlotIndex) : FTInventoryStack();
	if (IsValid(ItemImage))
	{
		ItemImage->SetBrushFromTexture(ItemStack.IsEmpty() ? nullptr : ItemStack.ItemDefinition->Thumbnail);
		ItemImage->SetVisibility(ItemStack.IsEmpty() ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
	}
	if (IsValid(ItemQuantity)) ItemQuantity->SetText(ItemStack.IsEmpty() || ItemStack.Quantity <= 1 ? FText::GetEmpty() : FText::AsNumber(ItemStack.Quantity));
}

FReply UT_InventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IsValid(Inventory) || Inventory->GetSlot(InventorySlotIndex).IsEmpty())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (!IsValid(ActionMenuWidget))
		{
			UE_LOG(LogTemp, Error, TEXT("Inventory slot %d has no action menu."), InventorySlotIndex);
			return FReply::Handled();
		}

		ActionMenuWidget->SetSelection(Inventory, InventorySlotIndex, InGeometry);
		return FReply::Handled();
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UT_InventorySlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
	UT_InventoryDragDropOperation* Operation = Cast<UT_InventoryDragDropOperation>(InOperation);
	if (IsValid(Operation) && Operation->SourceInventory == Inventory)
	{
		Inventory->DropItem(Operation->SourceSlotIndex, Operation->Quantity);
	}
}

void UT_InventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);
	if (!IsValid(Inventory)) return;

	const FTInventoryStack ItemStack = Inventory->GetSlot(InventorySlotIndex);
	if (ItemStack.IsEmpty()) return;

	UT_InventoryDragDropOperation* Operation = NewObject<UT_InventoryDragDropOperation>(this);
	Operation->SourceInventory = Inventory;
	Operation->SourceSlotIndex = InventorySlotIndex;
	Operation->Quantity = ItemStack.Quantity;

	if (IsValid(DraggingWidgetClass))
	{
		UT_InventoryDraggingWidget* DragVisual = CreateWidget<UT_InventoryDraggingWidget>(GetOwningPlayer(), DraggingWidgetClass);
		if (IsValid(DragVisual))
		{
			DragVisual->SetItem(Inventory, InventorySlotIndex);
			Operation->DefaultDragVisual = DragVisual;
		}
	}
	OutOperation = Operation;
}

bool UT_InventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UT_InventoryDragDropOperation* Operation = Cast<UT_InventoryDragDropOperation>(InOperation);
	if (!IsValid(Operation) || !IsValid(Operation->SourceInventory) || !IsValid(Inventory)) return false;

	if (Operation->SourceQuickSlotIndex != INDEX_NONE)
	{
		if (Operation->SourceInventory != Inventory) return false;

		return Inventory->ClearQuickSlot(
			Operation->SourceQuickSlotIndex,
			true
		);
	}

	if (!Operation->SourceInventory->IsValidSlotIndex(Operation->SourceSlotIndex)) return false;

	if (Operation->SourceInventory == Inventory)
	{
		return Inventory->MoveItem(
			Operation->SourceSlotIndex,
			InventorySlotIndex,
			Operation->Quantity
		);
	}

	int32 RemainingQuantity = Operation->Quantity;

	return Operation->SourceInventory->TransferItem(
		Inventory,
		Operation->SourceSlotIndex,
		Operation->Quantity,
		RemainingQuantity
	);
}

void UT_InventoryGridWidget::LoadInventory(UT_InventoryComponent* InventoryComponent, UT_InventoryActionMenuWidget* ActionMenu)
{
	if (IsValid(Inventory)) Inventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RebuildSlots);
	Inventory = InventoryComponent;
	ActionMenuWidget = ActionMenu;
	if (IsValid(Inventory)) Inventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RebuildSlots);
	RebuildSlots();
}

void UT_InventoryGridWidget::RebuildSlots()
{
	if (!IsValid(SlotGrid) || !IsValid(Inventory) || !IsValid(SlotWidgetClass)) return;
	SlotGrid->ClearChildren();

	for (int32 Index = 0; Index < Inventory->GetInventorySize(); ++Index)
	{
		UT_InventorySlotWidget* SlotWidget = CreateWidget<UT_InventorySlotWidget>(GetOwningPlayer(), SlotWidgetClass);
		if (!IsValid(SlotWidget)) continue;
		SlotWidget->InitializeSlot(Inventory, Index, ActionMenuWidget);
		SlotGrid->AddChildToUniformGrid(SlotWidget, Index / GridColumns, Index % GridColumns);
	}
}

void UT_InventoryPanelWidget::LoadInventory(UT_InventoryComponent* InventoryComponent, UT_InventoryActionMenuWidget* ActionMenu)
{
	if (IsValid(Inventory)) Inventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshPanel);
	Inventory = InventoryComponent;
	if (IsValid(Inventory)) Inventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshPanel);
	if (IsValid(InventorySlotsGridWidget)) InventorySlotsGridWidget->LoadInventory(Inventory, ActionMenu);
	RefreshPanel();
}

void UT_InventoryPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InventorySlotsGridWidget = Cast<UT_InventoryGridWidget>(WidgetTree->FindWidget(TEXT("InventorySlotsGrid")));
	InventoryNameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("InventoryName")));
	InventorySizeText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("InventorySize")));
}

void UT_InventoryPanelWidget::RefreshPanel()
{
	if (IsValid(InventoryNameText)) InventoryNameText->SetText(IsValid(Inventory) ? Inventory->GetInventoryName() : FText::GetEmpty());
	if (IsValid(InventorySizeText))
	{
		InventorySizeText->SetText(IsValid(Inventory)
			? FText::Format(NSLOCTEXT("Inventory", "InventorySize", "{0}/{1}"), Inventory->GetNumberOfItems(), Inventory->GetInventorySize())
			: FText::GetEmpty());
	}
}

bool UT_InventoryDiscardZoneWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UT_InventoryDragDropOperation* Operation = Cast<UT_InventoryDragDropOperation>(InOperation);
	return IsValid(Operation) && IsValid(Operation->SourceInventory) && Operation->SourceInventory->DropItem(Operation->SourceSlotIndex, Operation->Quantity);
}

void UT_InventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);

	PlayerInventoryPanelWidget = Cast<UT_InventoryPanelWidget>(WidgetTree->FindWidget(TEXT("Inventory_Panel")));
	StoragePanelWidget = Cast<UT_InventoryPanelWidget>(WidgetTree->FindWidget(TEXT("StorageInventoryPanel")));
	if (!IsValid(StoragePanelWidget)) StoragePanelWidget = Cast<UT_InventoryPanelWidget>(WidgetTree->FindWidget(TEXT("StorageInventory")));

	QuickBarWidget = Cast<UT_WeaponBarWidget>(WidgetTree->FindWidget(TEXT("QuickBar")));

	InventoryWindowWidget = WidgetTree->FindWidget(TEXT("Overlay_81"));
	if (!IsValid(InventoryWindowWidget)) InventoryWindowWidget = WidgetTree->FindWidget(TEXT("WBP_Window_Panel"));

	InventoryActionMenuWidget = Cast<UT_InventoryActionMenuWidget>(WidgetTree->FindWidget(TEXT("ActionMenuWidget")));
	if (!IsValid(InventoryActionMenuWidget)) InventoryActionMenuWidget = Cast<UT_InventoryActionMenuWidget>(WidgetTree->FindWidget(TEXT("Action Menu")));

	if (!IsValid(InventoryActionMenuWidget) && IsValid(ActionMenuWidgetClass))
	{
		InventoryActionMenuWidget = CreateWidget<UT_InventoryActionMenuWidget>(GetOwningPlayer(), ActionMenuWidgetClass);

		if (UOverlay* RootOverlay = Cast<UOverlay>(WidgetTree->RootWidget))
		{
			if (UOverlaySlot* MenuSlot = RootOverlay->AddChildToOverlay(InventoryActionMenuWidget))
			{
				MenuSlot->SetHorizontalAlignment(HAlign_Left);
				MenuSlot->SetVerticalAlignment(VAlign_Top);
				MenuSlot->SetPadding(FMargin(0.f));
			}
		}
		else if (UPanelWidget* RootPanel = Cast<UPanelWidget>(WidgetTree->RootWidget))
		{
			RootPanel->AddChild(InventoryActionMenuWidget);
		}
	}

	if (!IsValid(InventoryActionMenuWidget) || !IsValid(InventoryActionMenuWidget->GetParent()))
	{
		UE_LOG(LogTemp, Error, TEXT("Inventory action menu creation failed. Root=%s Class=%s."),
			*GetNameSafe(WidgetTree->RootWidget), *GetNameSafe(ActionMenuWidgetClass));

		InventoryActionMenuWidget = nullptr;
	}
	else
	{
		InventoryActionMenuWidget->ClearSelection();
	}

	InitializeInventory(PendingPlayerInventory, PendingStorageInventory);
}

void UT_InventoryWidget::InitializeInventory(UT_InventoryComponent* PlayerInventory, UT_InventoryComponent* StorageInventory)
{
	PendingPlayerInventory = PlayerInventory;
	PendingStorageInventory = StorageInventory;
	if (IsValid(PlayerInventoryPanelWidget)) PlayerInventoryPanelWidget->LoadInventory(PlayerInventory, InventoryActionMenuWidget);
	if (IsValid(QuickBarWidget)) QuickBarWidget->InitializeQuickBar(PlayerInventory);
	if (IsValid(StorageInventory) && !IsValid(StoragePanelWidget) && IsValid(StoragePanelClass))
	{
		StoragePanelWidget = CreateWidget<UT_InventoryPanelWidget>(GetOwningPlayer(), StoragePanelClass);
		if (IsValid(PlayerInventoryPanelWidget))
		{
			UPanelWidget* PanelParent = Cast<UPanelWidget>(PlayerInventoryPanelWidget->GetParent());
			if (!IsValid(PanelParent)) PanelParent = Cast<UPanelWidget>(WidgetTree->RootWidget);
			if (IsValid(PanelParent)) PanelParent->AddChild(StoragePanelWidget);
		}
		if (!StoragePanelWidget->GetParent()) StoragePanelWidget->AddToViewport(1);
	}
	if (IsValid(StoragePanelWidget))
	{
		StoragePanelWidget->SetVisibility(IsValid(StorageInventory) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		StoragePanelWidget->LoadInventory(StorageInventory, InventoryActionMenuWidget);
	}
}

void UT_WeaponBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	QuickSlotsBox = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("ShotcutSlots")));
	if (!IsValid(QuickSlotsBox)) QuickSlotsBox = Cast<UPanelWidget>(WidgetTree->FindWidget(TEXT("WrapBox")));
	RebuildQuickSlots();
}

void UT_WeaponBarWidget::InitializeQuickBar(UT_InventoryComponent* InventoryComponent)
{
	if (IsValid(Inventory))
	{
		Inventory->OnQuickSlotsChanged.RemoveDynamic(this, &ThisClass::RebuildQuickSlots);
		Inventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RebuildQuickSlots);
	}
	Inventory = InventoryComponent;
	if (IsValid(Inventory))
	{
		Inventory->OnQuickSlotsChanged.AddUniqueDynamic(this, &ThisClass::RebuildQuickSlots);
		Inventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RebuildQuickSlots);
	}
	RebuildQuickSlots();
}

void UT_WeaponBarWidget::RebuildQuickSlots()
{
	if (!IsValid(QuickSlotsBox) || !IsValid(Inventory) || !IsValid(QuickSlotWidgetClass)) return;
	for (UT_WeaponSlotWidget* QuickSlot : QuickSlotWidgets)
	{
		if (IsValid(QuickSlot)) QuickSlot->RemoveFromParent();
	}
	QuickSlotWidgets.Reset();
	for (int32 Index = 0; Index < Inventory->GetQuickSlotCount(); ++Index)
	{
		UT_WeaponSlotWidget* QuickSlot = CreateWidget<UT_WeaponSlotWidget>(GetOwningPlayer(), QuickSlotWidgetClass);
		if (!IsValid(QuickSlot)) continue;
		QuickSlot->InitializeQuickSlot(Inventory, Index);
		QuickSlotsBox->AddChild(QuickSlot);
		QuickSlotWidgets.Add(QuickSlot);
	}
}

void UT_WeaponSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	QuickSlotButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("Button_22")));
	if (!IsValid(QuickSlotButton)) QuickSlotButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("QuickSlotButton")));

	if (!IsValid(ItemImage)) ItemImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("EquipmentImage")));
	if (!IsValid(ItemQuantity)) ItemQuantity = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Amount")));

	ShortcutText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("Equip")));
	if (!IsValid(ShortcutText)) ShortcutText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("EquipmentName")));

	if (IsValid(QuickSlotButton)) QuickSlotButton->SetVisibility(ESlateVisibility::HitTestInvisible);

	RefreshSlot();
}

FReply UT_WeaponSlotWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
		IsValid(Inventory) && IsValid(Inventory->GetQuickSlotItem(SlotIndex)))
	{
		bDragStarted = false;

		return UWidgetBlueprintLibrary::DetectDragIfPressed(
			InMouseEvent,
			this,
			EKeys::LeftMouseButton
		).NativeReply;
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UT_WeaponSlotWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (!bDragStarted && IsValid(Inventory) && IsValid(Inventory->GetQuickSlotItem(SlotIndex))) HandleActivate();

		bDragStarted = false;
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UT_WeaponSlotWidget::NativeOnDragDetected(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent,
	UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	if (!IsValid(Inventory) || !IsValid(Inventory->GetQuickSlotItem(SlotIndex))) return;

	bDragStarted = true;

	UT_InventoryDragDropOperation* Operation = NewObject<UT_InventoryDragDropOperation>(this);
	Operation->SourceInventory = Inventory;
	Operation->SourceSlotIndex = INDEX_NONE;
	Operation->SourceQuickSlotIndex = SlotIndex;
	Operation->Quantity = 1;

	OutOperation = Operation;
}

void UT_WeaponSlotWidget::InitializeQuickSlot(UT_InventoryComponent* InventoryComponent, int32 QuickSlotIndex)
{
	Inventory = InventoryComponent;
	SlotIndex = QuickSlotIndex;
	RefreshSlot();
}

bool UT_WeaponSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UT_InventoryDragDropOperation* Operation = Cast<UT_InventoryDragDropOperation>(InOperation);
	if (!IsValid(Operation) || !IsValid(Inventory) || Operation->SourceInventory != Inventory) return false;

	if (Operation->SourceQuickSlotIndex != INDEX_NONE)
	{
		return Inventory->MoveQuickSlot(Operation->SourceQuickSlotIndex, SlotIndex);
	}

	if (!Inventory->IsValidSlotIndex(Operation->SourceSlotIndex)) return false;

	const FTInventoryStack ItemStack = Inventory->GetSlot(Operation->SourceSlotIndex);
	return !ItemStack.IsEmpty() && Inventory->AssignQuickSlot(SlotIndex, ItemStack.ItemDefinition);
}

void UT_WeaponSlotWidget::HandleActivate()
{
	if (IsValid(Inventory)) Inventory->ActivateQuickSlot(SlotIndex);
}

void UT_WeaponSlotWidget::RefreshSlot()
{
	UT_ItemDefinition* ItemDefinition = IsValid(Inventory) ? Inventory->GetQuickSlotItem(SlotIndex) : nullptr;
	if (IsValid(ItemImage))
	{
		ItemImage->SetBrushFromTexture(IsValid(ItemDefinition) ? ItemDefinition->Thumbnail : nullptr);
		ItemImage->SetVisibility(IsValid(ItemDefinition) ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (IsValid(ItemQuantity))
	{
		const int32 Quantity = IsValid(ItemDefinition) ? Inventory->GetTotalQuantity(ItemDefinition->ItemId) : 0;
		ItemQuantity->SetText(Quantity > 1 ? FText::AsNumber(Quantity) : FText::GetEmpty());
	}
	if (IsValid(ShortcutText)) ShortcutText->SetText(FText::AsNumber(SlotIndex + 1));
}

FReply UT_InventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::B)
	{
		if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetOwningPlayer())) PlayerController->CloseInventory();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UT_InventoryWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton) return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);

	const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
	if (IsValid(InventoryActionMenuWidget) && InventoryActionMenuWidget->GetVisibility() == ESlateVisibility::Visible &&
		InventoryActionMenuWidget->GetCachedGeometry().IsUnderLocation(ScreenPosition))
	{
		return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (IsValid(InventoryWindowWidget) && !InventoryWindowWidget->GetCachedGeometry().IsUnderLocation(ScreenPosition))
	{
		if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetOwningPlayer())) PlayerController->CloseInventory();
		return FReply::Handled();
	}

	if (IsValid(InventoryActionMenuWidget)) InventoryActionMenuWidget->ClearSelection();
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}
