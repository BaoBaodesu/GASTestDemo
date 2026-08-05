#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T_InventoryStorage.generated.h"

class UStaticMeshComponent;
class UT_InventoryComponent;

UCLASS(Blueprintable)
class GASTESTDEMO1_API AT_InventoryStorage : public AActor
{
	GENERATED_BODY()

public:
	AT_InventoryStorage();

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void OpenStorageInventory(APlayerController* PlayerController);

	UFUNCTION(BlueprintPure, Category="Inventory")
	UT_InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> StorageMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UT_InventoryComponent> InventoryComponent;
};
