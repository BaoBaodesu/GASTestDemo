#include "Quest/T_QuestGameState.h"

#include "Characters/T_BaseCharacter.h"
#include "Characters/T_EnemyCharacter.h"
#include "EngineUtils.h"
#include "GameObjects/T_PickUpItems.h"
#include "Inventory/T_ItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AT_QuestGameState::AT_QuestGameState()
{
	static ConstructorHelpers::FObjectFinder<UT_ItemDefinition> QuestItemAsset(
		TEXT("/Game/GASTestDemo/GameObjects/PickUp/DA_Item_Fumo.DA_Item_Fumo"));
	static ConstructorHelpers::FClassFinder<AT_PickUpItems> RequiredPickupAsset(
		TEXT("/Game/GASTestDemo/GameObjects/PickUp/BP_PickFumo.BP_PickFumo_C"));
	static ConstructorHelpers::FClassFinder<AT_PickUpItems> PistolPickupAsset(
		TEXT("/Game/GASTestDemo/GameObjects/PickUp/BP_PickPistol.BP_PickPistol_C"));
	static ConstructorHelpers::FClassFinder<AT_PickUpItems> HealthPickupAsset(
		TEXT("/Game/GASTestDemo/GameObjects/PickUp/BP_HealthUP.BP_HealthUP_C"));

	QuestItemDefinition = QuestItemAsset.Object;
	RequiredPickupClass = RequiredPickupAsset.Class;
	ForbiddenPistolPickupClass = PistolPickupAsset.Class;
	ForbiddenHealthPickupClass = HealthPickupAsset.Class;
}

void AT_QuestGameState::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;

	for (TActorIterator<AT_EnemyCharacter> It(GetWorld()); It; ++It)
	{
		RegisterEnemy(*It);
	}
	RefreshEliminateAllChallenge();
}

void AT_QuestGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass, QuestOutcome);
	DOREPLIFETIME(ThisClass, LiveChallengeResult);
	DOREPLIFETIME(ThisClass, FrozenChallengeResult);
	DOREPLIFETIME(ThisClass, TotalEnemyCount);
	DOREPLIFETIME(ThisClass, DefeatedEnemyCount);
	DOREPLIFETIME(ThisClass, PlayerDeathCount);
}

void AT_QuestGameState::CompleteMainQuest()
{
	if (!HasAuthority() || QuestOutcome != EQuestOutcome::InProgress) return;
	RefreshEliminateAllChallenge();
	FrozenChallengeResult = LiveChallengeResult;
	QuestOutcome = EQuestOutcome::Victory;
	BroadcastChanged();
}

void AT_QuestGameState::FailQuest()
{
	if (!HasAuthority() || QuestOutcome != EQuestOutcome::InProgress) return;
	QuestOutcome = EQuestOutcome::Failure;
	BroadcastChanged();
}

void AT_QuestGameState::RegisterEnemy(AT_EnemyCharacter* Enemy)
{
	if (!HasAuthority() || !IsValid(Enemy)) return;
	const FObjectKey EnemyKey(Enemy);
	if (RegisteredEnemies.Contains(EnemyKey)) return;

	RegisteredEnemies.Add(EnemyKey);
	++TotalEnemyCount;
	Enemy->OnCharacterDied.AddUniqueDynamic(this, &ThisClass::HandleRegisteredEnemyDeath);
	if (!Enemy->IsAlive()) NotifyEnemyDeath(Enemy);
	else
	{
		RefreshEliminateAllChallenge();
		BroadcastChanged();
	}
}

void AT_QuestGameState::NotifyEnemyDeath(AT_EnemyCharacter* Enemy)
{
	if (!HasAuthority() || !IsValid(Enemy)) return;
	const FObjectKey EnemyKey(Enemy);
	if (!RegisteredEnemies.Contains(EnemyKey)) RegisterEnemy(Enemy);
	if (DefeatedEnemies.Contains(EnemyKey)) return;

	DefeatedEnemies.Add(EnemyKey);
	++DefeatedEnemyCount;
	RefreshEliminateAllChallenge();
	BroadcastChanged();
}

void AT_QuestGameState::NotifyForbiddenPistolKill()
{
	if (!HasAuthority() || QuestOutcome != EQuestOutcome::InProgress || !LiveChallengeResult.bNoPistolKill) return;
	LiveChallengeResult.bNoPistolKill = false;
	BroadcastChanged();
}

void AT_QuestGameState::NotifyForbiddenHealthItemUsed()
{
	if (!HasAuthority() || QuestOutcome != EQuestOutcome::InProgress || !LiveChallengeResult.bNoHealthItemUsed) return;
	LiveChallengeResult.bNoHealthItemUsed = false;
	BroadcastChanged();
}

void AT_QuestGameState::NotifyGuardStateChanged(ETGuardAIState PreviousState, ETGuardAIState NewState)
{
	if (!HasAuthority() || QuestOutcome != EQuestOutcome::InProgress || !LiveChallengeResult.bNeverDetected) return;
	if (PreviousState == ETGuardAIState::Combat || NewState != ETGuardAIState::Combat) return;
	LiveChallengeResult.bNeverDetected = false;
	BroadcastChanged();
}

void AT_QuestGameState::NotifyPlayerDeath()
{
	if (!HasAuthority() || QuestOutcome != EQuestOutcome::InProgress) return;
	++PlayerDeathCount;
	BroadcastChanged();
}

FQuestChallengeResult AT_QuestGameState::GetChallengeResult() const
{
	return QuestOutcome == EQuestOutcome::Victory ? FrozenChallengeResult : LiveChallengeResult;
}

bool AT_QuestGameState::IsRequiredPickup(const AT_PickUpItems* Pickup) const
{
	return IsValid(Pickup)
		&& IsValid(QuestItemDefinition)
		&& Pickup->GetItemDefinition() == QuestItemDefinition;
}

bool AT_QuestGameState::IsForbiddenPistolItem(const UT_ItemDefinition* ItemDefinition) const
{
	const AT_PickUpItems* ForbiddenPickup = ForbiddenPistolPickupClass
		? ForbiddenPistolPickupClass->GetDefaultObject<AT_PickUpItems>()
		: nullptr;
	return IsValid(ItemDefinition)
		&& IsValid(ForbiddenPickup)
		&& ForbiddenPickup->GetItemDefinition() == ItemDefinition;
}

bool AT_QuestGameState::IsForbiddenHealthItem(const UT_ItemDefinition* ItemDefinition) const
{
	const AT_PickUpItems* ForbiddenPickup = ForbiddenHealthPickupClass
		? ForbiddenHealthPickupClass->GetDefaultObject<AT_PickUpItems>()
		: nullptr;
	return IsValid(ItemDefinition)
		&& IsValid(ForbiddenPickup)
		&& ForbiddenPickup->GetItemDefinition() == ItemDefinition;
}

void AT_QuestGameState::HandleRegisteredEnemyDeath(AT_BaseCharacter* DeadCharacter)
{
	NotifyEnemyDeath(Cast<AT_EnemyCharacter>(DeadCharacter));
}

void AT_QuestGameState::OnRep_QuestState()
{
	OnQuestStateChanged.Broadcast();
}

void AT_QuestGameState::BroadcastChanged()
{
	OnQuestStateChanged.Broadcast();
	ForceNetUpdate();
}

void AT_QuestGameState::RefreshEliminateAllChallenge()
{
	LiveChallengeResult.bEliminatedAllEnemies = TotalEnemyCount > 0 && DefeatedEnemyCount == TotalEnemyCount;
}
