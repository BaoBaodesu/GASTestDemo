#include "Quest/T_QuestGameMode.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
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

namespace
{
	constexpr float RespawnDelayAfterDeathAnimation = 0.5f;
}

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
	if (!IsValid(PlayerCharacter) || !IsValid(QuestGameState) || QuestGameState->GetQuestOutcome() != EQuestOutcome::InProgress)
	{
		return;
	}

	QuestGameState->NotifyPlayerDeath();
	const int32 DeathCount = QuestGameState->GetPlayerDeathCount();
	UAnimMontage* DeathMontage = PlayerCharacter->GetAbilitySystemComponent()
		? PlayerCharacter->GetAbilitySystemComponent()->GetCurrentMontage()
		: nullptr;
	UAnimInstance* AnimInstance = PlayerCharacter->GetMesh() ? PlayerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (DeathCount >= 2)
	{
		GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &ThisClass::FinalizeQuestFailure, TWeakObjectPtr<AT_PlayerCharacter>(PlayerCharacter)));
		return;
	}

	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(PlayerCharacter->GetController()))
	{
		PlayerController->ClientShowLastChance();
	}

	PendingRespawnPlayer = PlayerCharacter;
	PendingDeathTransform = PlayerCharacter->GetActorTransform();
	bLastChanceRespawnArmed = true;
	FreezePlayerForDeathPresentation(PlayerCharacter);

	if (!IsValid(DeathMontage) || !IsValid(AnimInstance))
	{
		ArmLastChanceRespawn(PlayerCharacter, RespawnDelayAfterDeathAnimation);
		return;
	}

	PendingDeathMontage = DeathMontage;
	AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &ThisClass::HandlePlayerDeathMontageEnded);
	ArmLastChanceRespawn(PlayerCharacter, DeathMontage->GetPlayLength() + RespawnDelayAfterDeathAnimation);
}

void AT_QuestGameMode::HandlePlayerDeathMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	(void)bInterrupted;
	if (Montage != PendingDeathMontage.Get()) return;

	AT_PlayerCharacter* PlayerCharacter = PendingRespawnPlayer.Get();
	UnbindPendingDeathMontage(PlayerCharacter);
	PendingDeathMontage.Reset();
	if (!IsValid(PlayerCharacter) || !bLastChanceRespawnArmed) return;

	FreezePlayerForDeathPresentation(PlayerCharacter);
	if (USkeletalMeshComponent* Mesh = PlayerCharacter->GetMesh()) Mesh->bPauseAnims = true;
	ArmLastChanceRespawn(PlayerCharacter, RespawnDelayAfterDeathAnimation);
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
	if (!bLastChanceRespawnArmed) return;
	bLastChanceRespawnArmed = false;
	GetWorldTimerManager().ClearTimer(LastChanceRespawnTimerHandle);

	AT_PlayerCharacter* Character = PlayerCharacter.Get();
	if (!IsValid(Character)) Character = PendingRespawnPlayer.Get();
	UnbindPendingDeathMontage(Character);
	PendingRespawnPlayer.Reset();
	PendingDeathMontage.Reset();

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
		Mesh->bPauseAnims = false;
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

	AActor* PlayerStart = FindPlayerStart(Controller);
	if (IsValid(PlayerStart))
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

void AT_QuestGameMode::FreezePlayerForDeathPresentation(AT_PlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerCharacter)) return;

	PlayerCharacter->SetAlive(false);
	PlayerCharacter->SetActorEnableCollision(false);
	if (UCapsuleComponent* Capsule = PlayerCharacter->GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	PlayerCharacter->SetActorTransform(PendingDeathTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (UCharacterMovementComponent* Movement = PlayerCharacter->GetCharacterMovement())
	{
		Movement->DisableMovement();
		Movement->SetComponentTickEnabled(false);
	}
	if (AT_PlayerController* PlayerController = Cast<AT_PlayerController>(PlayerCharacter->GetController()))
	{
		PlayerCharacter->DisableInput(PlayerController);
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
	}
}

void AT_QuestGameMode::UnbindPendingDeathMontage(AT_PlayerCharacter* PlayerCharacter)
{
	if (IsValid(PlayerCharacter) && IsValid(PlayerCharacter->GetMesh()) && IsValid(PlayerCharacter->GetMesh()->GetAnimInstance()))
	{
		PlayerCharacter->GetMesh()->GetAnimInstance()->OnMontageEnded.RemoveDynamic(this, &ThisClass::HandlePlayerDeathMontageEnded);
	}
}

void AT_QuestGameMode::ArmLastChanceRespawn(AT_PlayerCharacter* PlayerCharacter, float DelaySeconds)
{
	GetWorldTimerManager().SetTimer(
		LastChanceRespawnTimerHandle,
		FTimerDelegate::CreateUObject(this, &ThisClass::RespawnForLastChance, TWeakObjectPtr<AT_PlayerCharacter>(PlayerCharacter)),
		FMath::Max(DelaySeconds, 0.05f),
		false);
}
