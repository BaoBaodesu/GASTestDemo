#include "Inventory/T_InventoryTypes.h"

#include "Inventory/T_ItemDefinition.h"

bool FTInventoryStack::IsEmpty() const
{
	return !IsValid(ItemDefinition) || Quantity <= 0;
}

bool FTInventoryStack::CanStackWith(const FTInventoryStack& Other) const
{
	return !IsEmpty() && !Other.IsEmpty() && ItemDefinition == Other.ItemDefinition && ItemDefinition->MaxStackSize > 1;
}

int32 FTInventoryStack::GetFreeSpace() const
{
	return IsEmpty() ? 0 : FMath::Max(0, ItemDefinition->MaxStackSize - Quantity);
}

void FTInventoryStack::Clear()
{
	ItemDefinition = nullptr;
	Quantity = 0;
}
