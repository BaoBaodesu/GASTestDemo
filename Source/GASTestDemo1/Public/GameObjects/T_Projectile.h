// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "T_Projectile.generated.h"

class UProjectileMovementComponent;
class UGameplayEffect;
UCLASS()
class GASTESTDEMO1_API AT_Projectile : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AT_Projectile();
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash | Damage", meta = (ExposeOnSpawn, ClampMin = "0.0"))
	float Damage { -20.0f };
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Crash|Projectile")
	void SpawnImpactEffects();
	
private:
	UPROPERTY(VisibleAnywhere, Category = "Crash|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
	
	UPROPERTY(EditAnywhere, Category = "Crash|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;
};
