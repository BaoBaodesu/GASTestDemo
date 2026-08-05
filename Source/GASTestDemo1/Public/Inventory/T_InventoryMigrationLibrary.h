#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "T_InventoryMigrationLibrary.generated.h"

class UBlueprint;

UCLASS()
class GASTESTDEMO1_API UT_InventoryMigrationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Inventory|Editor")
	static bool CleanAndReparentBlueprint(UBlueprint* Blueprint, UClass* NewParentClass);

	UFUNCTION(BlueprintCallable, Category="Inventory|Editor")
	static bool RemapBlueprintReferences(UBlueprint* Blueprint, const TArray<UBlueprint*>& SourceBlueprints, const TArray<UBlueprint*>& TargetBlueprints);

	UFUNCTION(BlueprintCallable, Category="Inventory|Editor")
	static bool RemoveWidgets(UBlueprint* Blueprint, const TArray<FName>& WidgetNames);

	UFUNCTION(BlueprintCallable, Category="Inventory|Editor")
	static bool ReplaceWidgetClasses(UBlueprint* Blueprint, const TArray<UClass*>& SourceClasses, const TArray<UClass*>& TargetClasses);

	UFUNCTION(BlueprintCallable, Category="Inventory|Editor")
	static bool RepairWidgetVariables(UBlueprint* Blueprint);

	UFUNCTION(BlueprintCallable, Category="Inventory|Editor")
	static bool SetWidgetTickEnabled(UBlueprint* Blueprint, bool bEnabled);
};
