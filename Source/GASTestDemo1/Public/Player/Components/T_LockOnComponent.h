// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "T_LockOnComponent.generated.h"

class ACharacter;
class APlayerController;
class UCharacterMovementComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_LockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UT_LockOnComponent();

	UFUNCTION(BlueprintCallable)
	void ToggleLockOn();

	UFUNCTION(BlueprintCallable)
	void StartLockOn();

	UFUNCTION(BlueprintCallable)
	void StopLockOn();

	UFUNCTION(BlueprintCallable)
	void SwitchTarget(float Direction);

	UFUNCTION(BlueprintPure)
	AActor* GetCurrentTargetActor() const { return CurrentTargetActor; }

	UFUNCTION(BlueprintPure)
	bool IsLockingOn() const { return IsValid(CurrentTargetActor); }
	
protected:

	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LockOn")
	float LockOnRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LockOn")
	float BreakDistance = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LockOn")
	float CameraInterpSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LockOn")
	float MinForwardDot = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LockOn")
	TSubclassOf<AActor> TargetClassFilter;

	UPROPERTY(BlueprintReadOnly)
	AActor* CurrentTargetActor = nullptr;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	
private:
	UPROPERTY()
	ACharacter* OwnerCharacter = nullptr;

	UPROPERTY()
	APlayerController* PlayerController = nullptr;

	UPROPERTY()
	UCharacterMovementComponent* MovementComponent = nullptr;

	AActor* FindBestTarget() const;
	bool IsValidTarget(AActor* Target) const;
	bool HasLineOfSight(AActor* Target) const;
	void UpdateLockOnRotation(float DeltaTime);
	void SetLockOnMovementMode(bool bEnable);
};
