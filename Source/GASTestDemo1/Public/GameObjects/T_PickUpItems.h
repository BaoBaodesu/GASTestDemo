// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/T_ItemDefinition.h"
#include "T_PickUpItems.generated.h"

class UBoxComponent;
class UPrimitiveComponent;
class USphereComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;

USTRUCT(BlueprintType)
struct FTPickUpItemData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item")
	TObjectPtr<UT_ItemDefinition> ItemDefinition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Item", meta=(ClampMin="1"))
	int32 Quantity = 1;

	bool IsValid() const
	{
		return ::IsValid(ItemDefinition) && Quantity > 0;
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
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category="Pick Up")
	const FTPickUpItemData& GetItemData() const
	{
		return ItemData;
	}

	UFUNCTION(BlueprintPure, Category="Pick Up")
	UT_ItemDefinition* GetItemDefinition() const { return ItemData.ItemDefinition; }

	UFUNCTION(BlueprintPure, Category="Pick Up")
	int32 GetQuantity() const { return ItemData.Quantity; }

	UFUNCTION(BlueprintCallable, Category="Pick Up")
	void SetItemDefinition(UT_ItemDefinition* InItemDefinition);

	UFUNCTION(BlueprintCallable, Category="Pick Up")
	void SetQuantity(int32 InQuantity);

	UFUNCTION(BlueprintCallable, Category="Pick Up")
	bool ConsumeQuantity(AActor* Picker, int32 ConsumedQuantity);

	UFUNCTION(BlueprintPure, Category="Pick Up")
	bool CanBePickedUp(AActor* Picker) const;

	bool IsPhysicalCollisionComponent(const UPrimitiveComponent* Component) const;
	void SetFocused(bool bFocused);

	UFUNCTION(BlueprintCallable, Category="Pick Up")
	bool PickUp(AActor* Picker);

	UPROPERTY(BlueprintAssignable, Category="Pick Up")
	FTPickUpItemPickedUpSignature OnPickedUp;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Physics", meta=(ClampMin="0.1"))
	FVector PhysicsBoxExtent = FVector(15.f, 8.f, 5.f);
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components")
	TObjectPtr<UBoxComponent> PhysicsRoot;

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
