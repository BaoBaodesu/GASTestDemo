#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T_GameMenuWidget.generated.h"

class AT_QuestGameState;
class UButton;
class UTextBlock;
class UWidget;

UENUM()
enum class ETGameMenuMode : uint8
{
	Pause,
	Victory,
	Failure
};

UCLASS()
class GASTESTDEMO1_API UT_GameMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetMenuMode(ETGameMenuMode NewMode, AT_QuestGameState* QuestGameState);
	void SetNoMoreLevelsMessage();
	ETGameMenuMode GetMenuMode() const { return MenuMode; }

protected:
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleRestartClicked();

	UFUNCTION()
	void HandleQuitClicked();

	void SetChallengeText(UTextBlock* TextBlock, bool bCompleted, const FString& Text) const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ChallengeQuest;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_QuestTitle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Title;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional01;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional02;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional03;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional04;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Continue;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Restart;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Quit;

	FText InitialQuestTitle;
	ETGameMenuMode MenuMode = ETGameMenuMode::Pause;
};
