#include "UI/Quest/T_GameMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Player/T_PlayerController.h"
#include "Quest/T_QuestGameState.h"

void UT_GameMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (IsValid(Text_QuestTitle) && InitialQuestTitle.IsEmpty()) InitialQuestTitle = Text_QuestTitle->GetText();
	if (IsValid(Button_Continue)) Button_Continue->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleContinueClicked);
	if (IsValid(Button_Restart)) Button_Restart->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleRestartClicked);
	if (IsValid(Button_Quit)) Button_Quit->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
}

void UT_GameMenuWidget::SetMenuMode(ETGameMenuMode NewMode, AT_QuestGameState* QuestGameState)
{
	MenuMode = NewMode;
	const bool bVictory = MenuMode == ETGameMenuMode::Victory;
	const bool bFailure = MenuMode == ETGameMenuMode::Failure;
	if (IsValid(ChallengeQuest)) ChallengeQuest->SetVisibility(bVictory ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (IsValid(Text_QuestTitle)) Text_QuestTitle->SetVisibility(MenuMode == ETGameMenuMode::Pause ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	if (IsValid(Button_Continue)) Button_Continue->SetVisibility(bFailure ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	if (IsValid(Text_Title))
	{
		Text_Title->SetText(FText::FromString(bVictory ? TEXT("游戏胜利") : bFailure ? TEXT("游戏失败") : TEXT("游戏菜单")));
	}
	if (IsValid(Text_QuestTitle))
	{
		Text_QuestTitle->SetText(bFailure ? FText::FromString(TEXT("游戏失败")) : InitialQuestTitle);
	}
	if (!bVictory || !IsValid(QuestGameState)) return;

	const FQuestChallengeResult Result = QuestGameState->GetChallengeResult();
	SetChallengeText(Text_Optional01, Result.bNeverDetected, TEXT("未被敌人发现"));
	SetChallengeText(Text_Optional02, Result.bEliminatedAllEnemies,
		FString::Printf(TEXT("消灭全部敌人 %d / %d"), QuestGameState->GetDefeatedEnemyCount(), QuestGameState->GetTotalEnemyCount()));
	SetChallengeText(Text_Optional03, Result.bNoPistolKill, TEXT("不使用手枪击杀敌人"));
	SetChallengeText(Text_Optional04, Result.bNoHealthItemUsed, TEXT("不使用医疗包"));
}

void UT_GameMenuWidget::SetNoMoreLevelsMessage()
{
	if (IsValid(Text_Title)) Text_Title->SetText(FText::FromString(TEXT("没有继续的关卡了")));
}

void UT_GameMenuWidget::HandleContinueClicked()
{
	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetOwningPlayer())) PlayerController->HandleGameMenuContinue();
}

void UT_GameMenuWidget::HandleRestartClicked()
{
	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetOwningPlayer())) PlayerController->RestartQuestLevel();
}

void UT_GameMenuWidget::HandleQuitClicked()
{
	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(GetOwningPlayer())) PlayerController->QuitQuestGame();
}

void UT_GameMenuWidget::SetChallengeText(UTextBlock* TextBlock, bool bCompleted, const FString& Text) const
{
	if (IsValid(TextBlock)) TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s %s"), bCompleted ? TEXT("√") : TEXT("X"), *Text)));
}
