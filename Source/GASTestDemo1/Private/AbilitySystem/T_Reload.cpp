// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/T_Reload.h"


// Sets default values for this component's properties
UT_Reload::UT_Reload()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UT_Reload::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UT_Reload::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

