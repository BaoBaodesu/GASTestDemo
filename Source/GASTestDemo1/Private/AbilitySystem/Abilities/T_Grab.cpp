// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/T_Grab.h"


// Sets default values for this component's properties
UT_Grab::UT_Grab()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UT_Grab::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UT_Grab::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

