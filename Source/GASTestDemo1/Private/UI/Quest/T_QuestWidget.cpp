#include "UI/Quest/T_QuestWidget.h"

#include "Characters/T_PlayerCharacter.h"
#include "Components/TextBlock.h"
#include "Inventory/T_InventoryComponent.h"
#include "Inventory/T_ItemDefinition.h"
#include "Quest/T_QuestGameState.h"

void UT_QuestWidget::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeQuest(Cast<AT_PlayerCharacter>(GetOwningPlayerPawn()));
}

void UT_QuestWidget::NativeDestruct()
{
	UnbindSources();
	Super::NativeDestruct();
}

void UT_QuestWidget::InitializeQuest(AT_PlayerCharacter* PlayerCharacter)
{
	UnbindSources();
	BoundInventory = IsValid(PlayerCharacter) ? PlayerCharacter->GetInventoryComponent() : nullptr;
	BoundQuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr;

	if (IsValid(BoundInventory)) BoundInventory->OnInventoryChanged.AddUniqueDynamic(this, &ThisClass::RefreshQuestDisplay);
	if (IsValid(BoundQuestGameState)) BoundQuestGameState->OnQuestStateChanged.AddUniqueDynamic(this, &ThisClass::RefreshQuestDisplay);
	RefreshQuestDisplay();
}

void UT_QuestWidget::RefreshQuestDisplay()
{
	if (!IsValid(BoundQuestGameState))
	{
		BoundQuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr;
		if (IsValid(BoundQuestGameState)) BoundQuestGameState->OnQuestStateChanged.AddUniqueDynamic(this, &ThisClass::RefreshQuestDisplay);
	}

	const UT_ItemDefinition* QuestItem = IsValid(BoundQuestGameState) ? BoundQuestGameState->GetQuestItemDefinition() : nullptr;
	const bool bHasQuestItem = IsValid(BoundInventory) && IsValid(QuestItem)
		&& BoundInventory->ContainsItem(QuestItem->ItemId);
	if (IsValid(Text_MainObjective))
	{
		Text_MainObjective->SetText(FText::FromString(bHasQuestItem ? TEXT("将关键道具送往指定地点") : TEXT("寻找关键道具")));
	}

	if (!IsValid(BoundQuestGameState)) return;
	const FQuestChallengeResult Result = BoundQuestGameState->GetChallengeResult();
	const bool bVictory = BoundQuestGameState->GetQuestOutcome() == EQuestOutcome::Victory;
	SetChallengeText(Text_Optional01, Result.bNeverDetected
		? (bVictory ? TEXT("√ 未被敌人发现") : TEXT("未被敌人发现"))
		: TEXT("X 未被敌人发现"));

	const int32 DefeatedCount = BoundQuestGameState->GetDefeatedEnemyCount();
	const int32 TotalCount = BoundQuestGameState->GetTotalEnemyCount();
	const FString EnemyProgress = FString::Printf(TEXT("消灭全部敌人 %d / %d"), DefeatedCount, TotalCount);
	SetChallengeText(Text_Optional02, Result.bEliminatedAllEnemies ? TEXT("√ ") + EnemyProgress : EnemyProgress);
	SetChallengeText(Text_Optional03, Result.bNoPistolKill
		? (bVictory ? TEXT("√ 不使用手枪击杀敌人") : TEXT("不使用手枪击杀敌人"))
		: TEXT("X 不使用手枪击杀敌人"));
	SetChallengeText(Text_Optional04, Result.bNoHealthItemUsed
		? (bVictory ? TEXT("√ 不使用医疗包") : TEXT("不使用医疗包"))
		: TEXT("X 不使用医疗包"));
}

void UT_QuestWidget::UnbindSources()
{
	if (IsValid(BoundInventory)) BoundInventory->OnInventoryChanged.RemoveDynamic(this, &ThisClass::RefreshQuestDisplay);
	if (IsValid(BoundQuestGameState)) BoundQuestGameState->OnQuestStateChanged.RemoveDynamic(this, &ThisClass::RefreshQuestDisplay);
	BoundInventory = nullptr;
	BoundQuestGameState = nullptr;
}

void UT_QuestWidget::SetChallengeText(UTextBlock* TextBlock, const FString& Text) const
{
	if (IsValid(TextBlock)) TextBlock->SetText(FText::FromString(Text));
}
