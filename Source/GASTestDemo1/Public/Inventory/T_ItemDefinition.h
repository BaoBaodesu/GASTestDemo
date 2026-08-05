#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "T_ItemDefinition.generated.h"

class AT_PickUpItems;
class USkeletalMesh;
class UStaticMesh;
class UTexture2D;

UENUM(BlueprintType)
enum class ETItemType : uint8
{
	Consumable,
	Weapon,
	Ammo,
	Material,
	Throwable,
	Quest,
	Misc
};

UCLASS(BlueprintType)
class GASTESTDEMO1_API UT_ItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item", meta=(MultiLine="true"))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	ETItemType ItemType = ETItemType::Misc;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item", meta=(ClampMin="1"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	TObjectPtr<UTexture2D> Thumbnail;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|World")
	TObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|World")
	TObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|World")
	TSubclassOf<AT_PickUpItems> PickUpActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Equip")
	TSubclassOf<AActor> EquippedActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Equip")
	FName EquipSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Use")
	bool bCanUse = false;
};
