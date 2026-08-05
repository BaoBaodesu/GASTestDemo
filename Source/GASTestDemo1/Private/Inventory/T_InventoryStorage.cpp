#include "Inventory/T_InventoryStorage.h"

#include "Components/StaticMeshComponent.h"
#include "Inventory/T_InventoryComponent.h"
#include "Player/T_PlayerController.h"

AT_InventoryStorage::AT_InventoryStorage()
{
	PrimaryActorTick.bCanEverTick = false;

	StorageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StorageMesh"));
	SetRootComponent(StorageMesh);
	InventoryComponent = CreateDefaultSubobject<UT_InventoryComponent>(TEXT("InventoryComponent"));
}

void AT_InventoryStorage::OpenStorageInventory(APlayerController* PlayerController)
{
	if (AT_PlayerController* TPlayerController = Cast<AT_PlayerController>(PlayerController))
	{
		TPlayerController->OpenInventory(InventoryComponent);
	}
}
