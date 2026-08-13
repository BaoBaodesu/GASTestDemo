#include "Quest/T_QuestGameMode.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameplayTags/TTags.h"
#include "Player/T_PlayerController.h"
#include "Quest/T_QuestGameState.h"
#include "TimerManager.h"

AT_QuestGameMode::AT_QuestGameMode()
{
	GameStateClass = AT_QuestGameState::StaticClass();
}

void AT_QuestGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	AT_PlayerCharacter* PlayerCharacter = IsValid(NewPlayer) ? Cast<AT_PlayerCharacter>(NewPlayer->GetPawn()) : nullptr;
	if (!IsValid(PlayerCharacter)) return;
	BindPlayerDeath(PlayerCharacter);
	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(NewPlayer))
	{
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);
	}
}

void AT_QuestGameMode::HandlePlayerDeath(AT_BaseCharacter* DeadCharacter)
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(DeadCharacter);
	AT_QuestGameState* QuestGameState = GetGameState<AT_QuestGameState>();
	if (!IsValid(PlayerCharacter) || !IsValid(QuestGameState) || QuestGameState->GetQuestOutcome() != EQuestOutcome::InProgress) return;

	QuestGameState->NotifyPlayerDeath();
	if (QuestGameState->GetPlayerDeathCount() >= 2)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ThisClass::FinalizeQuestFailure, TWeakObjectPtr<AT_PlayerCharacter>(PlayerCharacter)));
		return;
	}

	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(PlayerCharacter->GetController()))
	{
		PlayerController->ClientShowLastChance();
	}

	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &ThisClass::RespawnForLastChance, TWeakObjectPtr<AT_PlayerCharacter>(PlayerCharacter)));
}

void AT_QuestGameMode::FinalizeQuestFailure(TWeakObjectPtr<AT_PlayerCharacter> PlayerCharacter)
{
	AT_PlayerCharacter* Character = PlayerCharacter.Get();
	if (IsValid(Character))
	{
		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
		{
			const FGameplayTagContainer DeathAbilityTags(TTags::TAbilities::Death.GetTag());
			ASC->CancelAbilities(&DeathAbilityTags);
		}
	}
	if (AT_QuestGameState* QuestGameState = GetGameState<AT_QuestGameState>()) QuestGameState->FailQuest();
}

void AT_QuestGameMode::RespawnForLastChance(TWeakObjectPtr<AT_PlayerCharacter> PlayerCharacter)
{
	AT_PlayerCharacter* Character = PlayerCharacter.Get();
	AController* Controller = IsValid(Character) ? Character->GetController() : nullptr;
	if (!IsValid(Character) || !IsValid(Controller)) return;

	UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent();
	if (IsValid(ASC))
	{
		const FGameplayTagContainer DeathAbilityTags(TTags::TAbilities::Death.GetTag());
		ASC->CancelAbilities(&DeathAbilityTags);

		FGameplayTagContainer DeadEffectTags;
		DeadEffectTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("TTags.Status.Dead")));
		ASC->RemoveActiveEffectsWithGrantedTags(DeadEffectTags);
	}

	if (USkeletalMeshComponent* Mesh = Character->GetMesh())
	{
		if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance()) AnimInstance->StopAllMontages(0.f);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	Character->SetActorHiddenInGame(false);
	Character->SetActorEnableCollision(true);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Character->ResetAttributes();
	Character->HandleRespawn();

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->SetComponentTickEnabled(true);
		Movement->SetMovementMode(MOVE_Walking);
		Movement->StopMovementImmediately();
	}

	if (AActor* PlayerStart = FindPlayerStart(Controller))
	{
		Character->TeleportTo(PlayerStart->GetActorLocation(), PlayerStart->GetActorRotation(), false, true);
		Controller->SetControlRotation(PlayerStart->GetActorRotation());
	}

	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(Controller))
	{
		Character->EnableInput(PlayerController);
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->SetIgnoreLookInput(false);
	}
}

void AT_QuestGameMode::BindPlayerDeath(AT_PlayerCharacter* PlayerCharacter)
{
	if (IsValid(PlayerCharacter))
	{
		PlayerCharacter->OnCharacterDied.AddUniqueDynamic(this, &ThisClass::HandlePlayerDeath);
	}
}
