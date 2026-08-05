#pragma once

#include "CoreMinimal.h"
#include "T_InventoryTypes.generated.h"

class UT_ItemDefinition;

USTRUCT(BlueprintType)
struct FTInventoryStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UT_ItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ClampMin="0"))
	int32 Quantity = 0;

	bool IsEmpty() const;
	bool CanStackWith(const FTInventoryStack& Other) const;
	int32 GetFreeSpace() const;
	void Clear();
};

USTRUCT(BlueprintType)
struct FTInitialInventoryStack
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory")
	TObjectPtr<UT_ItemDefinition> ItemDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Inventory", meta=(ClampMin="1"))
	int32 Quantity = 1;
};
