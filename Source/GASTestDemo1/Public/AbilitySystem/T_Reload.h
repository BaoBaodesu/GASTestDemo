// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "T_AbilitySystemComponent.h"
#include "T_Reload.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_Reload : public UT_AbilitySystemComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UT_Reload();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
