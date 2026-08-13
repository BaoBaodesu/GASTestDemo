#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "T_QuestWidget.generated.h"

class AT_PlayerCharacter;
class AT_QuestGameState;
class UT_InventoryComponent;
class UTextBlock;

UCLASS()
class GASTESTDEMO1_API UT_QuestWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeQuest(AT_PlayerCharacter* PlayerCharacter);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void RefreshQuestDisplay();

	void UnbindSources();
	void SetChallengeText(UTextBlock* TextBlock, const FString& Text) const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_MainObjective;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional01;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional02;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional03;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Optional04;

	UPROPERTY(Transient)
	TObjectPtr<UT_InventoryComponent> BoundInventory;

	UPROPERTY(Transient)
	TObjectPtr<AT_QuestGameState> BoundQuestGameState;
};
