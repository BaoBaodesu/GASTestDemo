#pragma once

#include "CoreMinimal.h"
#include "AI/T_ShooterAIController.h"
#include "GameFramework/GameStateBase.h"
#include "UObject/ObjectKey.h"
#include "T_QuestGameState.generated.h"

class AT_EnemyCharacter;
class AT_BaseCharacter;
class AT_PickUpItems;
class UT_ItemDefinition;

UENUM(BlueprintType)
enum class EQuestOutcome : uint8
{
	InProgress,
	Victory,
	Failure
};

USTRUCT(BlueprintType)
struct GASTESTDEMO1_API FQuestChallengeResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bNeverDetected = true;

	UPROPERTY(BlueprintReadOnly)
	bool bEliminatedAllEnemies = false;

	UPROPERTY(BlueprintReadOnly)
	bool bNoPistolKill = true;

	UPROPERTY(BlueprintReadOnly)
	bool bNoHealthItemUsed = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FTQuestStateChangedSignature);

UCLASS()
class GASTESTDEMO1_API AT_QuestGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AT_QuestGameState();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void CompleteMainQuest();

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void FailQuest();

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void RegisterEnemy(AT_EnemyCharacter* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void NotifyEnemyDeath(AT_EnemyCharacter* Enemy);

	UFUNCTION(BlueprintCallable, Category = "Quest|Challenge")
	void NotifyForbiddenPistolKill();

	UFUNCTION(BlueprintCallable, Category = "Quest|Challenge")
	void NotifyForbiddenHealthItemUsed();

	UFUNCTION(BlueprintCallable, Category = "Quest|Challenge")
	void NotifyGuardStateChanged(ETGuardAIState PreviousState, ETGuardAIState NewState);

	UFUNCTION(BlueprintCallable, Category = "Quest")
	void NotifyPlayerDeath();

	UFUNCTION(BlueprintPure, Category = "Quest")
	EQuestOutcome GetQuestOutcome() const { return QuestOutcome; }

	UFUNCTION(BlueprintPure, Category = "Quest")
	int32 GetTotalEnemyCount() const { return TotalEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Quest")
	int32 GetDefeatedEnemyCount() const { return DefeatedEnemyCount; }

	UFUNCTION(BlueprintPure, Category = "Quest")
	int32 GetPlayerDeathCount() const { return PlayerDeathCount; }

	UFUNCTION(BlueprintPure, Category = "Quest")
	FQuestChallengeResult GetChallengeResult() const;

	UFUNCTION(BlueprintPure, Category = "Quest")
	UT_ItemDefinition* GetQuestItemDefinition() const { return QuestItemDefinition; }

	bool IsRequiredPickup(const AT_PickUpItems* Pickup) const;
	bool IsForbiddenPistolItem(const UT_ItemDefinition* ItemDefinition) const;
	bool IsForbiddenHealthItem(const UT_ItemDefinition* ItemDefinition) const;

	UPROPERTY(BlueprintAssignable, Category = "Quest")
	FTQuestStateChangedSignature OnQuestStateChanged;

private:
	UFUNCTION()
	void HandleRegisteredEnemyDeath(AT_BaseCharacter* DeadCharacter);

	UFUNCTION()
	void OnRep_QuestState();

	void BroadcastChanged();
	void RefreshEliminateAllChallenge();

	UPROPERTY(EditDefaultsOnly, Category = "Quest|Items")
	TObjectPtr<UT_ItemDefinition> QuestItemDefinition;

	UPROPERTY(EditDefaultsOnly, Category = "Quest|Items")
	TSubclassOf<AT_PickUpItems> RequiredPickupClass;

	UPROPERTY(EditDefaultsOnly, Category = "Quest|Items")
	TSubclassOf<AT_PickUpItems> ForbiddenPistolPickupClass;

	UPROPERTY(EditDefaultsOnly, Category = "Quest|Items")
	TSubclassOf<AT_PickUpItems> ForbiddenHealthPickupClass;

	UPROPERTY(ReplicatedUsing = OnRep_QuestState)
	EQuestOutcome QuestOutcome = EQuestOutcome::InProgress;

	UPROPERTY(ReplicatedUsing = OnRep_QuestState)
	FQuestChallengeResult LiveChallengeResult;

	UPROPERTY(ReplicatedUsing = OnRep_QuestState)
	FQuestChallengeResult FrozenChallengeResult;

	UPROPERTY(ReplicatedUsing = OnRep_QuestState)
	int32 TotalEnemyCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_QuestState)
	int32 DefeatedEnemyCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_QuestState)
	int32 PlayerDeathCount = 0;

	TSet<FObjectKey> RegisteredEnemies;
	TSet<FObjectKey> DefeatedEnemies;
};
