#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "T_QuestDeliveryTrigger.generated.h"

class UBoxComponent;
class USceneComponent;
class UStaticMeshComponent;
class AT_PickUpItems;
class UPrimitiveComponent;

UCLASS()
class GASTESTDEMO1_API AT_QuestDeliveryTrigger : public AActor
{
	GENERATED_BODY()

public:
	AT_QuestDeliveryTrigger();

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> DeliveryPanel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> DeliveryTrigger;

	UPROPERTY(EditDefaultsOnly, Category = "Quest")
	TSubclassOf<AT_PickUpItems> RequiredPickupClass;

	bool bDelivered = false;
};
