#include "Inventory/T_ItemDefinition.h"

FPrimaryAssetId UT_ItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Item"), ItemId.IsNone() ? GetFName() : ItemId);
}
