// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/T_EnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "ContextualAnimSceneActorComponent.h"
#include "Engine/World.h"
#include "GameplayTags/TTags.h"
#include "Quest/T_QuestGameState.h"
#include "MotionWarpingComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UI/T_ActorWidgetComponent.h"
#include "Runtime/AIModule/Classes/AIController.h"


AT_EnemyCharacter::AT_EnemyCharacter()
{

	PrimaryActorTick.bCanEverTick = false;
	
	AbilitySystemComponent = CreateDefaultSubobject<UT_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	AttributeSet = CreateDefaultSubobject<UT_AttributeSet>("AttributeSet");
	
	LockOnWidget = CreateDefaultSubobject<UT_ActorWidgetComponent>(TEXT("LockOnWidget"));
	LockOnWidget->SetupAttachment(GetRootComponent());

	ContextualAnimSceneActorComponent = CreateDefaultSubobject<UContextualAnimSceneActorComponent>(TEXT("ContextualAnimSceneActorComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

void AT_EnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bIsBeingLaunched);
}

UAbilitySystemComponent* AT_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* AT_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

void AT_EnemyCharacter::StopMovementUntilLanded()
{
	bIsBeingLaunched = true;
	AAIController* AIController = GetController<AAIController>();
	if (!IsValid(AIController)) return;
	
	AIController->StopMovement();
	if (!LandedDelegate.IsAlreadyBound(this, &ThisClass::EnableMovementOnLanded))
	{
		LandedDelegate.AddDynamic(this, &ThisClass::EnableMovementOnLanded);
	}
}

void AT_EnemyCharacter::EnableMovementOnLanded(const FHitResult& Hit)
{
	bIsBeingLaunched = false;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, TTags::Events::Enemy::EndAttack, FGameplayEventData());
	LandedDelegate.RemoveAll(this);
}


void AT_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);

	if (!HasAuthority()) return;
	if (AT_QuestGameState* QuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr)
	{
		QuestGameState->RegisterEnemy(this);
	}
	
	GiveStartupAbilities();
	InitializeAttributes();
	
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	
	UT_AttributeSet* T_AttributeSet = Cast<UT_AttributeSet>(GetAttributeSet());
	if (!IsValid(T_AttributeSet)) return;
	
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(T_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void AT_EnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SkipDeathDestroyTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AT_EnemyCharacter::RequestSkipDeathPresentation(float InDestroyDelay)
{
	bSkipDeathPresentation = true;
	SkipDeathDestroyDelay = InDestroyDelay;
}

void AT_EnemyCharacter::ScheduleDeathDestroy(float DelaySeconds)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		Destroy();
		return;
	}

	World->GetTimerManager().SetTimer(
		SkipDeathDestroyTimerHandle,
		this,
		&AActor::K2_DestroyActor,
		FMath::Max(DelaySeconds, 0.f),
		false);
}

void AT_EnemyCharacter::HandleDeath()
{
	Super::HandleDeath();
	
	if (AAIController* AIController = GetController<AAIController>())
	{
		AIController->StopMovement();
	}

	if (!bSkipDeathPresentation) return;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->CancelAllAbilities();
		ASC->BlockAbilitiesWithTags(FGameplayTagContainer(TTags::TAbilities::Death.GetTag()));
	}

	ScheduleDeathDestroy(SkipDeathDestroyDelay);
}

