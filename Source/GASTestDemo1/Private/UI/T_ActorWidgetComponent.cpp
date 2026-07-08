// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/T_ActorWidgetComponent.h"


// Sets default values for this component's properties
UT_ActorWidgetComponent::UT_ActorWidgetComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
void UT_ActorWidgetComponent::BeginPlay()
{
	Super::BeginPlay();

	SetWidgetSpace(EWidgetSpace::Screen);
	SetVisibility(!bHiddenOnBeginPlay);
	
}

void UT_ActorWidgetComponent::ShowWidget()
{
	SetVisibility(true);
}

void UT_ActorWidgetComponent::HideWidget()
{
	SetVisibility(false);
}

void UT_ActorWidgetComponent::SetWidgetVisible(bool bWidgetVisible)
{
	SetVisibility(bWidgetVisible);
}


void UT_ActorWidgetComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                            FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

