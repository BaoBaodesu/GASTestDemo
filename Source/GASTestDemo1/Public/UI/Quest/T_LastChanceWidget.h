#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T_LastChanceWidget.generated.h"

UCLASS()
class GASTESTDEMO1_API UT_LastChanceWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
};
