// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "Test2Component.generated.h"

class ACharacter;
class UAnimMontage;
class UCapsuleComponent;
class UCharacterMovementComponent;
class UMotionWarpingComponent;
class USkeletalMeshComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UTest2Component : public UActorComponent
{
	GENERATED_BODY()

public:
	UTest2Component();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void StartGrabTimer();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void StopGrabTimer();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void GrabTrace();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void BoundsCheck();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void AlignGrab();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void Transition();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void Detach();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void Shimmy(double Direction);

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void LedgeJump();

	UFUNCTION(BlueprintCallable, Category="Test2|Grab")
	void BarJump();

	bool IsGrabbed() const { return bGrabbed; }
	bool IsOnBar() const { return bOnBar; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool HasValidCharacter() const;
	void FinishBarJump();
	void OnLedgeClimbMontageEnded(UAnimMontage* Montage, bool bInterrupted);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Test2|Grab", meta=(ClampMin="0.001", AllowPrivateAccess="true"))
	float GrabTraceInterval{0.01f};

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Test2|Grab")
	TObjectPtr<UAnimMontage> LedgeClimbMontage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	bool bGrabbed{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	bool bCanMove{true};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	bool bOnBar{false};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	FName GrabType{NAME_None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	double GrabHeight{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	double DistanceToGrab{0.0};
	
	UPROPERTY(BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	double GrabOffset{0.0};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	FVector WallNormal2{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	FVector LowerCheckPoint{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	FVector TopImpactPoint{FVector::ZeroVector};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test2|Grab")
	FVector BarMoveDirection{FVector::ZeroVector};

	UPROPERTY(BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	FVector ClimbTargetLocation{FVector::ZeroVector};

	UPROPERTY(BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	FRotator ClimbTargetRotation{FRotator::ZeroRotator};

	UPROPERTY(Transient, BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	TObjectPtr<ACharacter> Character;

	UPROPERTY(Transient, BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCharacterMovementComponent> CharacterMovement;

	UPROPERTY(Transient, BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCapsuleComponent> CapsuleComponent;

	UPROPERTY(Transient, BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> MeshComponent;

	UPROPERTY(Transient, BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(BlueprintReadWrite, Category="Test2|State", meta=(AllowPrivateAccess="true"))
	FVector TopTraceLocation{FVector::ZeroVector};

	UPROPERTY(BlueprintReadWrite, Category="Test2|Timer", meta=(AllowPrivateAccess="true"))
	FTimerHandle GrabTraceTimerHandle;

	UPROPERTY(BlueprintReadWrite, Category="Test2|Timer", meta=(AllowPrivateAccess="true"))
	FTimerHandle TransitionTimerHandle;

	UPROPERTY(BlueprintReadWrite, Category="Test2|Timer", meta=(AllowPrivateAccess="true"))
	FTimerHandle DetachTimerHandle;

	UPROPERTY(BlueprintReadWrite, Category="Test2|Timer", meta=(AllowPrivateAccess="true"))
	FTimerHandle BarJumpTimerHandle;
};
