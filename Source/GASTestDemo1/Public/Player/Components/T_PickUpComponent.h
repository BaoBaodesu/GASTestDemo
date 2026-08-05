// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Components/ActorComponent.h"
#include "GameObjects/T_PickUpItems.h"
#include "T_PickUpComponent.generated.h"

class AT_PickUpItems;
class UAnimInstance;
class UAnimMontage;
class USoundBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FTItemPickedUpSignature,
	FTPickUpItemData, ItemData
);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_PickUpComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UT_PickUpComponent();

	UFUNCTION(BlueprintCallable, Category="Pick Up")
	bool TryPickUp();

	UFUNCTION(BlueprintPure, Category="Pick Up")
	AT_PickUpItems* FindPickUpItem() const;

	UFUNCTION(BlueprintPure, Category="Pick Up")
	bool IsPickUpInProgress() const
	{
		return bPickUpInProgress;
	}

	UPROPERTY(BlueprintAssignable, Category="Pick Up")
	FTItemPickedUpSignature OnItemPickedUp;

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pick Up|Trace", meta=(ClampMin="1.0"))
	float TraceDistance = 250.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pick Up|Trace", meta=(ClampMin="0.0"))
	float TraceRadius = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pick Up|Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pick Up|Animation")
	TObjectPtr<UAnimMontage> PickUpMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pick Up|Animation")
	FName PickUpNotifyName = TEXT("PickUp");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pick Up|Sound")
	TObjectPtr<USoundBase> PickUpSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pick Up|Focus", meta=(ClampMin="1.0"))
	float FocusDistance = 350.f;

private:
	bool GetViewPoint(FVector& ViewLocation, FRotator& ViewRotation) const;

	bool CommitPendingItem();

	bool PickUpItem(AT_PickUpItems* Item);

	bool ValidatePickUpItem(const AT_PickUpItems* Item) const;

	void ResetPickUpState();
	void UpdateFocusedItem();
	void SetFocusedItem(AT_PickUpItems* Item);

	UAnimInstance* GetAnimInstance() const;

	UFUNCTION()
	void HandleMontageNotifyBegin(FName NotifyName, const FBranchingPointNotifyPayload& BranchingPointPayload);

	void HandlePickUpMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(Server, Reliable)
	void ServerPickUpItem(AT_PickUpItems* Item);

	UPROPERTY(Transient)
	TObjectPtr<AT_PickUpItems> PendingItem;

	UPROPERTY(Transient)
	TObjectPtr<AT_PickUpItems> FocusedItem;

	bool bPickUpInProgress = false;
};
