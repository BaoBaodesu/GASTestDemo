#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "T_InventoryWidgets.generated.h"

class UButton;
class UImage;
class UPanelWidget;
class UTextBlock;
class UUniformGridPanel;
class UWidget;
class UWrapBox;
class UT_InventoryComponent;
class UT_InventoryActionMenuWidget;
class UT_InventoryGridWidget;
class UT_InventoryPanelWidget;
class UT_InventorySlotWidget;
class UT_WeaponBarWidget;

UCLASS()
class GASTESTDEMO1_API UT_InventoryDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UT_InventoryComponent> SourceInventory;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	int32 SourceSlotIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	int32 Quantity = 0;
	
	UPROPERTY(BlueprintReadOnly, Category="Inventory|Drag")
	int32 SourceQuickSlotIndex = INDEX_NONE;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_InventoryDraggingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetItem(UT_InventoryComponent* InventoryComponent, int32 SlotIndex);

protected:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> ItemImage;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemQuantity;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_InventoryActionMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetSelection(UT_InventoryComponent* InventoryComponent, int32 SlotIndex, const FGeometry& AnchorGeometry);
	void ClearSelection();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> UseButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> EquipButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> DiscardButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> SplitActionButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UButton> SplitButton;

private:
	void SetMenuPosition(const FGeometry& AnchorGeometry);

	UFUNCTION()
	void HandleUse();

	UFUNCTION()
	void HandleEquip();

	UFUNCTION()
	void HandleDiscard();

	UFUNCTION()
	void HandleSplit();

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> SelectedInventory;

	int32 SelectedSlotIndex = INDEX_NONE;
	
	FGeometry PendingAnchorGeometry;
	
	bool bRefreshMenuPosition = false;
	

};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_InventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeSlot(UT_InventoryComponent* InventoryComponent, int32 SlotIndex, UT_InventoryActionMenuWidget* ActionMenu);
	void RefreshSlot();

protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TSubclassOf<UT_InventoryDraggingWidget> DraggingWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> ItemImage;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemQuantity;


private:
	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryActionMenuWidget> ActionMenuWidget;

	int32 InventorySlotIndex = INDEX_NONE;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_InventoryGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void LoadInventory(UT_InventoryComponent* InventoryComponent, UT_InventoryActionMenuWidget* ActionMenu);

	UFUNCTION()
	void RebuildSlots();

protected:
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TSubclassOf<UT_InventorySlotWidget> SlotWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category="Inventory", meta=(ClampMin="1"))
	int32 GridColumns = 5;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UUniformGridPanel> SlotGrid;

private:
	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryActionMenuWidget> ActionMenuWidget;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_InventoryPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void LoadInventory(UT_InventoryComponent* InventoryComponent, UT_InventoryActionMenuWidget* ActionMenu);

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void RefreshPanel();

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryGridWidget> InventorySlotsGridWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventoryNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventorySizeText;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_InventoryDiscardZoneWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_InventoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeInventory(UT_InventoryComponent* PlayerInventory, UT_InventoryComponent* StorageInventory = nullptr);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TSubclassOf<UT_InventoryPanelWidget> StoragePanelClass;

	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	TSubclassOf<UT_InventoryActionMenuWidget> ActionMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UWrapBox> WrapBox;

private:
	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> PendingPlayerInventory;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> PendingStorageInventory;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryPanelWidget> PlayerInventoryPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryPanelWidget> StoragePanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryActionMenuWidget> InventoryActionMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UT_WeaponBarWidget> QuickBarWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> InventoryWindowWidget;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_WeaponBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeQuickBar(UT_InventoryComponent* InventoryComponent);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(EditDefaultsOnly, Category="Inventory|Quick Slots")
	TSubclassOf<class UT_WeaponSlotWidget> QuickSlotWidgetClass;

private:
	UFUNCTION()
	void RebuildQuickSlots();

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UPanelWidget> QuickSlotsBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class UT_WeaponSlotWidget>> QuickSlotWidgets;
};

UCLASS(Abstract, Blueprintable)
class GASTESTDEMO1_API UT_WeaponSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeQuickSlot(UT_InventoryComponent* InventoryComponent, int32 QuickSlotIndex);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UImage> ItemImage;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemQuantity;

private:
	UFUNCTION()
	void HandleActivate();

	void RefreshSlot();

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UButton> QuickSlotButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ShortcutText;

	int32 SlotIndex = INDEX_NONE;

	bool bDragStarted = false;
};
