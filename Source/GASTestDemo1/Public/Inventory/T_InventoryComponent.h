#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/T_InventoryTypes.h"
#include "T_InventoryComponent.generated.h"

class AT_PickUpItems;
class UT_ItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FQuickSlotsChangedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FInventoryItemChangedSignature, UT_ItemDefinition*, ItemDefinition, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquippedItemChangedSignature, UT_ItemDefinition*, ItemDefinition);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT_InventoryComponent();

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool AddItem(UT_ItemDefinition* ItemDefinition, int32 Quantity, int32& RemainingQuantity);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveItem(int32 SlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool MoveItem(int32 SourceIndex, int32 TargetIndex, int32 Quantity = -1);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SplitStack(int32 SourceIndex, int32 Amount, int32 TargetIndex = -1);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool TransferItem(UT_InventoryComponent* TargetInventory, int32 SourceIndex, int32 Quantity, int32& RemainingQuantity);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool DropItem(int32 SlotIndex, int32 Quantity);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool UseItem(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool EquipItem(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool UnequipItem();

	UFUNCTION(BlueprintCallable, Category="Inventory|Quick Slots")
	bool AssignQuickSlot(int32 QuickSlotIndex, UT_ItemDefinition* ItemDefinition);

	UFUNCTION(BlueprintCallable, Category="Inventory|Quick Slots")
	bool MoveQuickSlot(int32 SourceQuickSlotIndex, int32 TargetQuickSlotIndex);

	UFUNCTION(BlueprintCallable, Category="Inventory|Quick Slots")
	bool ClearQuickSlot(int32 QuickSlotIndex, bool bUnequipIfEquipped = true);

	UFUNCTION(BlueprintCallable, Category="Inventory|Quick Slots")
	bool ActivateQuickSlot(int32 QuickSlotIndex);

	UFUNCTION(BlueprintPure, Category="Inventory|Quick Slots")
	UT_ItemDefinition* GetQuickSlotItem(int32 QuickSlotIndex) const;

	UFUNCTION(BlueprintPure, Category="Inventory|Quick Slots")
	int32 GetQuickSlotCount() const { return QuickSlots.Num(); }

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool ResizeInventory(int32 NewSize);

	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 FindItemById(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	bool ContainsItem(FName ItemId, int32 Quantity = 1) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 GetTotalQuantity(FName ItemId) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 GetInventorySize() const { return Slots.Num(); }

	UFUNCTION(BlueprintPure, Category="Inventory")
	FText GetInventoryName() const { return InventoryName; }

	UFUNCTION(BlueprintPure, Category="Inventory")
	int32 GetNumberOfItems() const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	bool IsFull() const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	bool IsValidSlotIndex(int32 SlotIndex) const { return Slots.IsValidIndex(SlotIndex); }

	UFUNCTION(BlueprintPure, Category="Inventory")
	FTInventoryStack GetSlot(int32 SlotIndex) const;

	const TArray<FTInventoryStack>& GetSlots() const { return Slots; }

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FInventoryChangedSignature OnInventoryChanged;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FInventoryItemChangedSignature OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FInventoryItemChangedSignature OnItemRemoved;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FEquippedItemChangedSignature OnEquippedItemChanged;

	UPROPERTY(BlueprintAssignable, Category="Inventory|Quick Slots")
	FQuickSlotsChangedSignature OnQuickSlotsChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|Settings")
	FText InventoryName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|Settings", meta=(ClampMin="1"))
	int32 InitialSize = 20;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|Settings", meta=(ClampMin="1"))
	int32 MaxSize = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|Settings")
	TArray<FTInitialInventoryStack> InitialItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory|Quick Slots", meta=(ClampMin="1"))
	int32 QuickSlotCount = 4;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Inventory")
	TArray<FTInventoryStack> Slots;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Inventory|Quick Slots")
	TArray<TObjectPtr<UT_ItemDefinition>> QuickSlots;

private:
	int32 FindEmptySlot() const;
	AT_PickUpItems* SpawnDroppedItem(UT_ItemDefinition* ItemDefinition, int32 Quantity) const;
	void BroadcastChanged();
	void CleanupQuickSlots();
	void UpdateEquippedIndexAfterMove(int32 SourceIndex, int32 TargetIndex, bool bSourceEmptied, bool bSwapped);

	UPROPERTY(Transient)
	int32 EquippedSlotIndex = INDEX_NONE;
};
