#pragma once

#include "CoreMinimal.h"
#include "AI/T_ShooterAIController.h"
#include "Blueprint/UserWidget.h"
#include "T_AIAwarenessWidget.generated.h"

class UImage;
class UProgressBar;
class UTexture2D;

UCLASS()
class GASTESTDEMO1_API UT_AIAwarenessWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UT_AIAwarenessWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Guard|Awareness")
	void UpdateAwareness(float InAwareness, ETGuardAIState InAIState, bool bHasVisualContact);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> Image_Awareness;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> ProgressBar_Awareness;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Awareness")
	TObjectPtr<UTexture2D> SuspicionIconTexture;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|Awareness")
	TObjectPtr<UTexture2D> AlertIconTexture;
};
