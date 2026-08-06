#include "Inventory/T_InventoryComponent.h"

#include "Engine/World.h"
#include "GameObjects/T_PickUpItems.h"
#include "Inventory/T_InventoryItemHandler.h"
#include "Inventory/T_ItemDefinition.h"

UT_InventoryComponent::UT_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	QuickSlots.SetNum(FMath::Max(1, QuickSlotCount));
}

void UT_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	Slots.SetNum(FMath::Clamp(InitialSize, 1, FMath::Max(1, MaxSize)));
	QuickSlots.SetNum(FMath::Max(1, QuickSlotCount));

	for (const FTInitialInventoryStack& InitialItem : InitialItems)
	{
		int32 RemainingQuantity = InitialItem.Quantity;
		AddItem(InitialItem.ItemDefinition, InitialItem.Quantity, RemainingQuantity);
	}
}

bool UT_InventoryComponent::AddItem(UT_ItemDefinition* ItemDefinition, int32 Quantity, int32& RemainingQuantity)
{
	RemainingQuantity = Quantity;
	if (!IsValid(ItemDefinition) || Quantity <= 0 || ItemDefinition->MaxStackSize <= 0) return false;

	for (FTInventoryStack& Slot : Slots)
	{
		if (RemainingQuantity <= 0) break;
		if (Slot.IsEmpty() || Slot.ItemDefinition != ItemDefinition || Slot.GetFreeSpace() <= 0) continue;

		const int32 AddedQuantity = FMath::Min(RemainingQuantity, Slot.GetFreeSpace());
		Slot.Quantity += AddedQuantity;
		RemainingQuantity -= AddedQuantity;
	}

	for (FTInventoryStack& Slot : Slots)
	{
		if (RemainingQuantity <= 0) break;
		if (!Slot.IsEmpty()) continue;

		const int32 AddedQuantity = FMath::Min(RemainingQuantity, FMath::Max(1, ItemDefinition->MaxStackSize));
		Slot.ItemDefinition = ItemDefinition;
		Slot.Quantity = AddedQuantity;
		RemainingQuantity -= AddedQuantity;
	}

	const int32 AddedQuantity = Quantity - RemainingQuantity;
	if (AddedQuantity <= 0) return false;

	OnItemAdded.Broadcast(ItemDefinition, AddedQuantity);
	BroadcastChanged();
	return true;
}

bool UT_InventoryComponent::RemoveItem(int32 SlotIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex) || Quantity <= 0 || Slots[SlotIndex].IsEmpty()) return false;

	FTInventoryStack& Slot = Slots[SlotIndex];
	const int32 RemovedQuantity = FMath::Min(Quantity, Slot.Quantity);
	UT_ItemDefinition* ItemDefinition = Slot.ItemDefinition;

	if (RemovedQuantity == Slot.Quantity && EquippedSlotIndex == SlotIndex && !UnequipItem()) return false;

	Slot.Quantity -= RemovedQuantity;
	if (Slot.Quantity <= 0) Slot.Clear();

	OnItemRemoved.Broadcast(ItemDefinition, RemovedQuantity);
	BroadcastChanged();
	return true;
}

bool UT_InventoryComponent::MoveItem(int32 SourceIndex, int32 TargetIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SourceIndex) || !Slots.IsValidIndex(TargetIndex) || SourceIndex == TargetIndex) return false;

	FTInventoryStack& Source = Slots[SourceIndex];
	FTInventoryStack& Target = Slots[TargetIndex];
	if (Source.IsEmpty()) return false;

	if (Quantity != INDEX_NONE && (Quantity <= 0 || Quantity > Source.Quantity)) return false;
	const int32 MoveQuantity = Quantity == INDEX_NONE ? Source.Quantity : Quantity;
	bool bSwapped = false;

	if (Target.IsEmpty())
	{
		Target.ItemDefinition = Source.ItemDefinition;
		Target.Quantity = MoveQuantity;
		Source.Quantity -= MoveQuantity;
		if (Source.Quantity <= 0) Source.Clear();
	}
	else if (Target.ItemDefinition == Source.ItemDefinition && Target.GetFreeSpace() > 0)
	{
		const int32 StackedQuantity = FMath::Min(MoveQuantity, Target.GetFreeSpace());
		Target.Quantity += StackedQuantity;
		Source.Quantity -= StackedQuantity;
		if (Source.Quantity <= 0) Source.Clear();
	}
	else
	{
		if (MoveQuantity != Source.Quantity) return false;
		Swap(Source, Target);
		bSwapped = true;
	}

	UpdateEquippedIndexAfterMove(SourceIndex, TargetIndex, Source.IsEmpty(), bSwapped);
	BroadcastChanged();
	return true;
}

bool UT_InventoryComponent::SplitStack(int32 SourceIndex, int32 Amount, int32 TargetIndex)
{
	if (!Slots.IsValidIndex(SourceIndex) || Slots[SourceIndex].IsEmpty()) return false;
	if (Amount <= 0 || Amount >= Slots[SourceIndex].Quantity) return false;

	if (TargetIndex == INDEX_NONE) TargetIndex = FindEmptySlot();
	if (!Slots.IsValidIndex(TargetIndex) || !Slots[TargetIndex].IsEmpty()) return false;

	return MoveItem(SourceIndex, TargetIndex, Amount);
}

bool UT_InventoryComponent::TransferItem(UT_InventoryComponent* TargetInventory, int32 SourceIndex, int32 Quantity, int32& RemainingQuantity)
{
	RemainingQuantity = Quantity;
	if (!IsValid(TargetInventory) || TargetInventory == this || !Slots.IsValidIndex(SourceIndex)) return false;
	if (Slots[SourceIndex].IsEmpty() || Quantity <= 0) return false;

	const int32 RequestedQuantity = FMath::Min(Quantity, Slots[SourceIndex].Quantity);
	if (EquippedSlotIndex == SourceIndex && RequestedQuantity == Slots[SourceIndex].Quantity && !UnequipItem()) return false;
	int32 TargetRemainingQuantity = RequestedQuantity;
	TargetInventory->AddItem(Slots[SourceIndex].ItemDefinition, RequestedQuantity, TargetRemainingQuantity);

	const int32 TransferredQuantity = RequestedQuantity - TargetRemainingQuantity;
	RemainingQuantity = Quantity - TransferredQuantity;
	if (TransferredQuantity <= 0) return false;

	return RemoveItem(SourceIndex, TransferredQuantity);
}

bool UT_InventoryComponent::DropItem(int32 SlotIndex, int32 Quantity)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty() || Quantity <= 0) return false;

	const int32 DroppedQuantity = FMath::Min(Quantity, Slots[SlotIndex].Quantity);
	AT_PickUpItems* DroppedItem = SpawnDroppedItem(Slots[SlotIndex].ItemDefinition, DroppedQuantity);
	if (!IsValid(DroppedItem)) return false;
	if (EquippedSlotIndex == SlotIndex && DroppedQuantity == Slots[SlotIndex].Quantity && !UnequipItem())
	{
		DroppedItem->Destroy();
		return false;
	}

	return RemoveItem(SlotIndex, DroppedQuantity);
}

bool UT_InventoryComponent::UseItem(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return false;

	AActor* Owner = GetOwner();
	UT_ItemDefinition* ItemDefinition = Slots[SlotIndex].ItemDefinition;
	if (!IsValid(Owner) || !Owner->GetClass()->ImplementsInterface(UT_InventoryItemHandler::StaticClass())) return false;
	if (!IT_InventoryItemHandler::Execute_CanUseInventoryItem(Owner, ItemDefinition)) return false;
	if (EquippedSlotIndex == SlotIndex && Slots[SlotIndex].Quantity == 1 && !UnequipItem()) return false;
	if (!IT_InventoryItemHandler::Execute_UseInventoryItem(Owner, ItemDefinition)) return false;

	return RemoveItem(SlotIndex, 1);
}

bool UT_InventoryComponent::EquipItem(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || Slots[SlotIndex].IsEmpty()) return false;

	AActor* Owner = GetOwner();
	UT_ItemDefinition* ItemDefinition = Slots[SlotIndex].ItemDefinition;
	if (!IsValid(Owner) || !Owner->GetClass()->ImplementsInterface(UT_InventoryItemHandler::StaticClass())) return false;
	if (!IT_InventoryItemHandler::Execute_CanEquipInventoryItem(Owner, ItemDefinition)) return false;
	if (EquippedSlotIndex != INDEX_NONE && !UnequipItem()) return false;
	if (!IT_InventoryItemHandler::Execute_EquipInventoryItem(Owner, ItemDefinition)) return false;

	EquippedSlotIndex = SlotIndex;
	if (!QuickSlots.Contains(ItemDefinition))
	{
		int32 QuickSlotIndex = QuickSlots.IndexOfByPredicate([](const TObjectPtr<UT_ItemDefinition>& QuickSlotItem)
		{
			return !IsValid(QuickSlotItem);
		});
		if (QuickSlotIndex == INDEX_NONE) QuickSlotIndex = 0;
		AssignQuickSlot(QuickSlotIndex, ItemDefinition);
	}
	OnEquippedItemChanged.Broadcast(ItemDefinition);
	BroadcastChanged();
	return true;
}

bool UT_InventoryComponent::UnequipItem()
{
	if (EquippedSlotIndex == INDEX_NONE) return true;

	AActor* Owner = GetOwner();
	UT_ItemDefinition* ItemDefinition = Slots.IsValidIndex(EquippedSlotIndex) ? Slots[EquippedSlotIndex].ItemDefinition : nullptr;
	if (IsValid(Owner) && Owner->GetClass()->ImplementsInterface(UT_InventoryItemHandler::StaticClass()) &&
		!IT_InventoryItemHandler::Execute_UnequipInventoryItem(Owner, ItemDefinition)) return false;

	EquippedSlotIndex = INDEX_NONE;
	OnEquippedItemChanged.Broadcast(nullptr);
	BroadcastChanged();
	return true;
}

bool UT_InventoryComponent::AssignQuickSlot(int32 QuickSlotIndex, UT_ItemDefinition* ItemDefinition)
{
	if (!QuickSlots.IsValidIndex(QuickSlotIndex) || !IsValid(ItemDefinition) || FindItemById(ItemDefinition->ItemId) == INDEX_NONE) return false;

	bool bChanged = false;

	for (int32 Index = 0; Index < QuickSlots.Num(); ++Index)
	{
		if (Index == QuickSlotIndex || QuickSlots[Index] != ItemDefinition) continue;

		QuickSlots[Index] = nullptr;
		bChanged = true;
	}

	if (QuickSlots[QuickSlotIndex] != ItemDefinition)
	{
		QuickSlots[QuickSlotIndex] = ItemDefinition;
		bChanged = true;
	}

	if (bChanged) OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UT_InventoryComponent::MoveQuickSlot(int32 SourceQuickSlotIndex, int32 TargetQuickSlotIndex)
{
	if (!QuickSlots.IsValidIndex(SourceQuickSlotIndex) || !QuickSlots.IsValidIndex(TargetQuickSlotIndex)) return false;
	if (!IsValid(QuickSlots[SourceQuickSlotIndex])) return false;
	if (SourceQuickSlotIndex == TargetQuickSlotIndex) return true;

	Swap(QuickSlots[SourceQuickSlotIndex], QuickSlots[TargetQuickSlotIndex]);
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UT_InventoryComponent::ClearQuickSlot(int32 QuickSlotIndex, bool bUnequipIfEquipped)
{
	if (!QuickSlots.IsValidIndex(QuickSlotIndex) || !IsValid(QuickSlots[QuickSlotIndex])) return false;

	UT_ItemDefinition* ItemDefinition = QuickSlots[QuickSlotIndex];

	if (bUnequipIfEquipped && Slots.IsValidIndex(EquippedSlotIndex) &&
		Slots[EquippedSlotIndex].ItemDefinition == ItemDefinition && !UnequipItem()) return false;

	QuickSlots[QuickSlotIndex] = nullptr;
	OnQuickSlotsChanged.Broadcast();
	return true;
}

bool UT_InventoryComponent::ActivateQuickSlot(int32 QuickSlotIndex)
{
	UT_ItemDefinition* ItemDefinition = GetQuickSlotItem(QuickSlotIndex);
	if (!IsValid(ItemDefinition)) return false;

	const int32 SlotIndex = FindItemById(ItemDefinition->ItemId);
	if (SlotIndex == INDEX_NONE) return false;
	if (IsValid(ItemDefinition->EquippedActorClass))
	{
		if (Slots.IsValidIndex(EquippedSlotIndex) && Slots[EquippedSlotIndex].ItemDefinition == ItemDefinition) return UnequipItem();
		return EquipItem(SlotIndex);
	}
	return UseItem(SlotIndex);
}

UT_ItemDefinition* UT_InventoryComponent::GetQuickSlotItem(int32 QuickSlotIndex) const
{
	return QuickSlots.IsValidIndex(QuickSlotIndex) ? QuickSlots[QuickSlotIndex].Get() : nullptr;
}

bool UT_InventoryComponent::ResizeInventory(int32 NewSize)
{
	NewSize = FMath::Clamp(NewSize, 1, FMath::Max(1, MaxSize));
	if (NewSize == Slots.Num()) return true;

	if (NewSize < Slots.Num())
	{
		for (int32 Index = NewSize; Index < Slots.Num(); ++Index)
		{
			if (!Slots[Index].IsEmpty() && !IsValid(Slots[Index].ItemDefinition->PickUpActorClass)) return false;
		}

		TArray<TObjectPtr<AT_PickUpItems>> SpawnedItems;
		for (int32 Index = NewSize; Index < Slots.Num(); ++Index)
		{
			if (Slots[Index].IsEmpty()) continue;
			AT_PickUpItems* SpawnedItem = SpawnDroppedItem(Slots[Index].ItemDefinition, Slots[Index].Quantity);
			if (!IsValid(SpawnedItem))
			{
				for (AT_PickUpItems* Item : SpawnedItems) if (IsValid(Item)) Item->Destroy();
				return false;
			}
			SpawnedItems.Add(SpawnedItem);
		}

		if (EquippedSlotIndex >= NewSize && !UnequipItem())
		{
			for (AT_PickUpItems* Item : SpawnedItems) if (IsValid(Item)) Item->Destroy();
			return false;
		}
	}

	Slots.SetNum(NewSize);
	BroadcastChanged();
	return true;
}

int32 UT_InventoryComponent::FindItemById(FName ItemId) const
{
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		if (!Slots[Index].IsEmpty() && Slots[Index].ItemDefinition->ItemId == ItemId) return Index;
	}
	return INDEX_NONE;
}

bool UT_InventoryComponent::ContainsItem(FName ItemId, int32 Quantity) const
{
	return Quantity > 0 && GetTotalQuantity(ItemId) >= Quantity;
}

int32 UT_InventoryComponent::GetTotalQuantity(FName ItemId) const
{
	int32 TotalQuantity = 0;
	for (const FTInventoryStack& Slot : Slots)
	{
		if (!Slot.IsEmpty() && Slot.ItemDefinition->ItemId == ItemId) TotalQuantity += Slot.Quantity;
	}
	return TotalQuantity;
}

int32 UT_InventoryComponent::GetNumberOfItems() const
{
	int32 NumberOfItems = 0;
	for (const FTInventoryStack& Slot : Slots) if (!Slot.IsEmpty()) ++NumberOfItems;
	return NumberOfItems;
}

bool UT_InventoryComponent::IsFull() const
{
	for (const FTInventoryStack& Slot : Slots)
	{
		if (Slot.IsEmpty() || Slot.GetFreeSpace() > 0) return false;
	}
	return true;
}

FTInventoryStack UT_InventoryComponent::GetSlot(int32 SlotIndex) const
{
	return Slots.IsValidIndex(SlotIndex) ? Slots[SlotIndex] : FTInventoryStack();
}

int32 UT_InventoryComponent::FindEmptySlot() const
{
	for (int32 Index = 0; Index < Slots.Num(); ++Index) if (Slots[Index].IsEmpty()) return Index;
	return INDEX_NONE;
}

AT_PickUpItems* UT_InventoryComponent::SpawnDroppedItem(UT_ItemDefinition* ItemDefinition, int32 Quantity) const
{
	if (!IsValid(ItemDefinition) || !IsValid(ItemDefinition->PickUpActorClass) || Quantity <= 0 || !IsValid(GetWorld()) || !IsValid(GetOwner())) return nullptr;

	const FVector SpawnLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 100.f;
	const FTransform SpawnTransform(GetOwner()->GetActorRotation(), SpawnLocation);
	AT_PickUpItems* Item = GetWorld()->SpawnActorDeferred<AT_PickUpItems>(ItemDefinition->PickUpActorClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
	if (!IsValid(Item)) return nullptr;

	Item->SetItemDefinition(ItemDefinition);
	Item->SetQuantity(Quantity);
	Item->FinishSpawning(SpawnTransform);
	return Item;
}

void UT_InventoryComponent::BroadcastChanged()
{
	CleanupQuickSlots();
	OnInventoryChanged.Broadcast();
}

void UT_InventoryComponent::CleanupQuickSlots()
{
	bool bChanged = false;
	TSet<UT_ItemDefinition*> AssignedItems;

	for (TObjectPtr<UT_ItemDefinition>& ItemDefinition : QuickSlots)
	{
		if (!IsValid(ItemDefinition)) continue;

		if (FindItemById(ItemDefinition->ItemId) == INDEX_NONE || AssignedItems.Contains(ItemDefinition.Get()))
		{
			ItemDefinition = nullptr;
			bChanged = true;
			continue;
		}

		AssignedItems.Add(ItemDefinition.Get());
	}

	if (bChanged) OnQuickSlotsChanged.Broadcast();
}

void UT_InventoryComponent::UpdateEquippedIndexAfterMove(int32 SourceIndex, int32 TargetIndex, bool bSourceEmptied, bool bSwapped)
{
	if (EquippedSlotIndex == SourceIndex && (bSourceEmptied || bSwapped)) EquippedSlotIndex = TargetIndex;
	else if (EquippedSlotIndex == TargetIndex && bSwapped) EquippedSlotIndex = SourceIndex;
}
