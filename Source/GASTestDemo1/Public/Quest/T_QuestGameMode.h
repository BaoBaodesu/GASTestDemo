#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "T_QuestGameMode.generated.h"

class AT_BaseCharacter;
class AT_PlayerCharacter;
class AController;

UCLASS()
class GASTESTDEMO1_API AT_QuestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AT_QuestGameMode();

	virtual void RestartPlayer(AController* NewPlayer) override;

private:
	UFUNCTION()
	void HandlePlayerDeath(AT_BaseCharacter* DeadCharacter);

	void RespawnForLastChance(TWeakObjectPtr<AT_PlayerCharacter> PlayerCharacter);
	void FinalizeQuestFailure(TWeakObjectPtr<AT_PlayerCharacter> PlayerCharacter);
	void BindPlayerDeath(AT_PlayerCharacter* PlayerCharacter);
};
