// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T_PickUpItems.generated.h"

class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UTexture2D;

UENUM(BlueprintType)
enum class ETPickUpItemType : uint8
{
	Consumable,
	Weapon,
	Ammo,
	Material,
	Quest,
	Misc
};

USTRUCT(BlueprintType)
struct FTPickUpItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FName ItemId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	ETPickUpItemType ItemType = ETPickUpItemType::Misc;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	bool bStackable = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1", EditCondition="bStackable"))
	int32 MaxStackSize = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UTexture2D> Icon = nullptr;

	bool IsValid() const
	{
		return !ItemId.IsNone() && Quantity > 0;
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FTPickUpItemPickedUpSignature,
	AActor*, Picker,
	FTPickUpItemData, ItemData
);

UCLASS()
class GASTESTDEMO1_API AT_PickUpItems : public AActor
{
	GENERATED_BODY()

public:
	AT_PickUpItems();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category="Pick Up")
	const FTPickUpItemData& GetItemData() const
	{
		return ItemData;
	}

	UFUNCTION(BlueprintPure, Category="Pick Up")
	bool CanBePickedUp(AActor* Picker) const;

	UFUNCTION(BlueprintCallable, Category="Pick Up")
	bool PickUp(AActor* Picker);

	UPROPERTY(BlueprintAssignable, Category="Pick Up")
	FTPickUpItemPickedUpSignature OnPickedUp;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UStaticMeshComponent> StaticItemMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	FTPickUpItemData ItemData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pick Up")
	TEnumAsByte<ECollisionChannel> InteractionTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Pick Up", meta=(ClampMin="1.0"))
	float InteractionRadius = 50.f;

	UFUNCTION(BlueprintImplementableEvent, Category="Pick Up")
	void BP_OnPickedUp(AActor* Picker, const FTPickUpItemData& PickedItemData);

private:
	void RefreshComponents();

	bool bPickedUp = false;
};