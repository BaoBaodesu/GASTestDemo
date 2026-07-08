// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "T_ActorWidgetComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GASTESTDEMO1_API UT_ActorWidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UT_ActorWidgetComponent();
	
	UFUNCTION(BlueprintCallable)
	void ShowWidget();

	UFUNCTION(BlueprintCallable)
	void HideWidget();

	UFUNCTION(BlueprintCallable)
	void SetWidgetVisible(bool bWidgetVisible);
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crash|UI")
	bool bHiddenOnBeginPlay = true;
};
