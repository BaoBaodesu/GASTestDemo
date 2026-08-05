#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "T_InventoryItemHandler.generated.h"

class UT_ItemDefinition;

UINTERFACE(BlueprintType)
class GASTESTDEMO1_API UT_InventoryItemHandler : public UInterface
{
	GENERATED_BODY()
};

class GASTESTDEMO1_API IT_InventoryItemHandler
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Item")
	bool CanUseInventoryItem(UT_ItemDefinition* ItemDefinition);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Item")
	bool UseInventoryItem(UT_ItemDefinition* ItemDefinition);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Item")
	bool CanEquipInventoryItem(UT_ItemDefinition* ItemDefinition);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Item")
	bool EquipInventoryItem(UT_ItemDefinition* ItemDefinition);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Inventory|Item")
	bool UnequipInventoryItem(UT_ItemDefinition* ItemDefinition);
};
