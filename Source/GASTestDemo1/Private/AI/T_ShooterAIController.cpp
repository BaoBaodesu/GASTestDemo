// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/T_ShooterAIController.h"

#include "AbilitySystemComponent.h"
#include "AI/T_GuardAlertSubsystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BrainComponent.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_GuardCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameObjects/T_Throwable.h"
#include "GameplayTags/TTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Quest/T_QuestGameState.h"

namespace GuardBBKeys
{
	const FName Enemy = TEXT("Enemy");
	const FName MoveLocation = TEXT("Move Location");
	const FName AIState = TEXT("AI State");
	const FName Awareness = TEXT("Awareness");
	const FName LastKnownLocation = TEXT("Last Known Location");
	const FName InvestigateLocation = TEXT("Investigate Location");
	const FName HomeLocation = TEXT("Home Location");
	const FName CombatMoveLocation = TEXT("Combat Move Location");
}

namespace
{
	constexpr float InvestigationMinAwareness = 30.f;
	constexpr float DistractionAwarenessBoost = 12.f;
	constexpr float DistractionAwarenessCap = 45.f;
	constexpr float RearContactMinAwareness = 20.f;
	constexpr float CloseRangeInstantDistance = 200.f;
	constexpr float CloseRangeFacingDot = 0.35f;
	constexpr float ContactFrontFacingDot = 0.15f;
	constexpr float CombatLostSightAwareness = 55.f;
	constexpr float CrouchAwarenessMultiplier = 0.28f;
	constexpr float SprintAwarenessMultiplier = 1.35f;
}

AT_ShooterAIController::AT_ShooterAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;

	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*PerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComponent->ConfigureSense(*SightConfig);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComponent->ConfigureSense(*HearingConfig);

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	PerceptionComponent->ConfigureSense(*DamageConfig);
	PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
}

void AT_ShooterAIController::ConfigurePerceptionSenses()
{
	if (!IsValid(PerceptionComponent)) return;

	if (IsValid(SightConfig))
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(SightMaxAge);
		PerceptionComponent->ConfigureSense(*SightConfig);
	}
	if (IsValid(HearingConfig))
	{
		HearingConfig->HearingRange = HearingRange;
		HearingConfig->SetMaxAge(HearingMaxAge);
		PerceptionComponent->ConfigureSense(*HearingConfig);
	}
	if (IsValid(DamageConfig))
	{
		DamageConfig->SetMaxAge(DamageMaxAge);
		PerceptionComponent->ConfigureSense(*DamageConfig);
	}
	PerceptionComponent->RequestStimuliListenerUpdate();
}

void AT_ShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ConfigurePerceptionSenses();

	if (IsValid(PerceptionComponent))
	{
		PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}

	if (StartGuardBehaviorTree())
	{
		if (UBlackboardComponent* BB = GetBlackboardComponent())
		{
			BB->SetValueAsVector(GuardBBKeys::HomeLocation, InPawn->GetActorLocation());
		}
		SetAwareness(0.f);
		SetAIState(ETGuardAIState::Patrol);
	}
}

void AT_ShooterAIController::OnUnPossess()
{
	if (IsValid(PerceptionComponent))
	{
		PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}
	PerceivedTargets.Reset();
	CurrentTarget = nullptr;
	Super::OnUnPossess();
}

void AT_ShooterAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (HasAuthority()) UpdateAwareness(DeltaSeconds);
}

AT_GuardCharacter* AT_ShooterAIController::GetGuardCharacter() const
{
	return Cast<AT_GuardCharacter>(GetPawn());
}

bool AT_ShooterAIController::IsValidPlayerTarget(AActor* Actor) const
{
	const AT_BaseCharacter* TargetCharacter = Cast<AT_BaseCharacter>(Actor);
	return IsValid(TargetCharacter) && TargetCharacter->IsAlive() && Actor->ActorHasTag(CrashTags::Player);
}

bool AT_ShooterAIController::ShouldEnterSuspiciousFromSight(ETGuardAIState CurrentState)
{
	return CurrentState == ETGuardAIState::Patrol
		|| CurrentState == ETGuardAIState::Return;
}

float AT_ShooterAIController::GetFacingAwarenessMultiplier(const FVector& GuardForward, const FVector& ToTarget)
{
	const FVector Forward2D = GuardForward.GetSafeNormal2D();
	const FVector ToTarget2D = ToTarget.GetSafeNormal2D();
	if (Forward2D.IsNearlyZero() || ToTarget2D.IsNearlyZero()) return 1.f;

	const float FacingDot = FVector::DotProduct(Forward2D, ToTarget2D);
	if (FacingDot > 0.5f) return 1.f;
	if (FacingDot < 0.f) return 0.25f;
	return 0.55f;
}

bool AT_ShooterAIController::ShouldInstantDetectCloseRange(float Distance, float FacingDot)
{
	return Distance <= CloseRangeInstantDistance && FacingDot > CloseRangeFacingDot;
}

float AT_ShooterAIController::GetFacingDotToActor(AActor* Actor) const
{
	const APawn* GuardPawn = GetPawn();
	if (!IsValid(GuardPawn) || !IsValid(Actor)) return -1.f;

	const FVector ToTarget = Actor->GetActorLocation() - GuardPawn->GetActorLocation();
	const FVector Forward2D = GuardPawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector ToTarget2D = ToTarget.GetSafeNormal2D();
	if (Forward2D.IsNearlyZero() || ToTarget2D.IsNearlyZero()) return -1.f;
	return FVector::DotProduct(Forward2D, ToTarget2D);
}

void AT_ShooterAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!HasAuthority() || !IsValid(Actor)) return;

	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		HandleHearingStimulus(Actor, Stimulus);
		return;
	}
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
	{
		if (Stimulus.WasSuccessfullySensed()) ConfirmTargetFromDamage(Actor, Stimulus.StimulusLocation);
		return;
	}
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
	{
		HandleSightStimulus(Actor, Stimulus);
	}
}

void AT_ShooterAIController::HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (!IsValidPlayerTarget(Actor)) return;

	if (Stimulus.WasSuccessfullySensed())
	{
		UpdatePerceivedTarget(Actor, Stimulus.StimulusLocation);
		SelectAndSetTarget();
		if (Actor == CurrentTarget.Get())
		{
			SetVisualContact(true);
			TimeSinceVisualContact = 0.f;
			LastKnownTargetLocation = Actor->GetActorLocation();
			SyncLastKnownLocation();
			if (ShouldInstantReengageFromSight(AIState))
			{
				SetAwareness(100.f);
				SetAIState(ETGuardAIState::Combat);
			}
			else if (ShouldEnterSuspiciousFromSight(AIState))
			{
				SetAIState(ETGuardAIState::Suspicious);
			}
		}
		return;
	}

	RemovePerceivedTarget(Actor);
	if (Actor == CurrentTarget.Get())
	{
		SetVisualContact(false);
		TimeSinceVisualContact = 0.f;
		LastKnownTargetLocation = Stimulus.StimulusLocation;
		SyncLastKnownLocation();
		SelectAndSetTarget();
		if (CurrentTarget.IsValid() && CurrentTarget.Get() != Actor)
		{
			SetVisualContact(true);
			TimeSinceVisualContact = 0.f;
			LastKnownTargetLocation = CurrentTarget->GetActorLocation();
			SyncLastKnownLocation();
			if (ShouldInstantReengageFromSight(AIState))
			{
				SetAwareness(100.f);
				SetAIState(ETGuardAIState::Combat);
			}
			else if (ShouldEnterSuspiciousFromSight(AIState))
			{
				SetAIState(ETGuardAIState::Suspicious);
			}
		}
	}
}

void AT_ShooterAIController::HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus)
{
	if (!Stimulus.WasSuccessfullySensed()) return;
	if (UT_GuardAlertSubsystem::IsThrowableImpactNoise(Stimulus.Tag) || Actor->IsA<AT_Throwable>()) return;
	if (!IsValidPlayerTarget(Actor)) return;

	LastKnownTargetLocation = Stimulus.StimulusLocation;
	SyncLastKnownLocation();
	SetTarget(Actor);
	SetAwareness(ClampHearingAwareness(Awareness, Stimulus.Strength));

	if (AIState == ETGuardAIState::Search || AIState == ETGuardAIState::Investigate)
	{
		BeginInvestigation(Stimulus.StimulusLocation);
	}
	else if (AIState == ETGuardAIState::Patrol || AIState == ETGuardAIState::Return)
	{
		SetAIState(ETGuardAIState::Suspicious);
	}
}

void AT_ShooterAIController::UpdateGameplayFocus()
{
	// Return 的朝向由返回 MoveTo 控制，不能继续盯着玩家最后位置，否则会倒退回家。
	if (AIState == ETGuardAIState::Return || bAlertObservationActive) return;

	if (bHasVisualContact)
	{
		if (AActor* Target = CurrentTarget.Get())
		{
			if (IsValidPlayerTarget(Target))
			{
				SetFocus(Target, EAIFocusPriority::Gameplay);
				return;
			}
		}
	}

	if (DistractionFocusRemaining > 0.f)
	{
		SetFocalPoint(DistractionFocusLocation, EAIFocusPriority::Gameplay);
		return;
	}

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		if (BB->IsVectorValueSet(GuardBBKeys::LastKnownLocation))
		{
			SetFocalPoint(BB->GetValueAsVector(GuardBBKeys::LastKnownLocation), EAIFocusPriority::Gameplay);
			return;
		}
		if (BB->IsVectorValueSet(GuardBBKeys::InvestigateLocation))
		{
			SetFocalPoint(BB->GetValueAsVector(GuardBBKeys::InvestigateLocation), EAIFocusPriority::Gameplay);
			return;
		}
	}

	ClearFocus(EAIFocusPriority::Gameplay);
}

void AT_ShooterAIController::UpdateAwareness(float DeltaSeconds)
{
	AT_GuardCharacter* Guard = GetGuardCharacter();
	if (!IsValid(Guard) || !Guard->IsAlive()) return;
	if (DistractionFocusRemaining > 0.f)
	{
		DistractionFocusRemaining = FMath::Max(0.f, DistractionFocusRemaining - DeltaSeconds);
	}
	if (AIState == ETGuardAIState::Search)
	{
		SearchElapsedTime += DeltaSeconds;
		if (SearchElapsedTime >= SearchTimeout)
		{
			CompleteCurrentBehaviorState();
			return;
		}
	}

	AActor* Target = CurrentTarget.Get();
	if (bHasVisualContact && IsValidPlayerTarget(Target))
	{
		TimeSinceVisualContact = 0.f;
		LastKnownTargetLocation = Target->GetActorLocation();
		SyncLastKnownLocation();
		UpdateGameplayFocus();
		if (ShouldInstantReengageFromSight(AIState))
		{
			SetAwareness(100.f);
			SetAIState(ETGuardAIState::Combat);
			return;
		}

		const FVector ToTarget = Target->GetActorLocation() - Guard->GetActorLocation();
		const float Distance = ToTarget.Size();
		const float FacingDot = GetFacingDotToActor(Target);

		if (ShouldInstantDetectCloseRange(Distance, FacingDot))
		{
			SetAwareness(100.f);
		}
		else
		{
			float StanceMultiplier = 1.f;
			if (const AT_PlayerCharacter* Player = Cast<AT_PlayerCharacter>(Target))
			{
				if (Player->bIsCrouched) StanceMultiplier = CrouchAwarenessMultiplier;
				else if (Player->GetVelocity().Size2D() >= 600.f) StanceMultiplier = SprintAwarenessMultiplier;
			}
			const float FacingMultiplier = GetFacingAwarenessMultiplier(Guard->GetActorForwardVector(), ToTarget);
			SetAwareness(Awareness + GetSightAwarenessRate(Distance) * StanceMultiplier * FacingMultiplier * DeltaSeconds);
		}

		if (Awareness >= 100.f) SetAIState(ETGuardAIState::Combat);
		else SetAIState(ETGuardAIState::Suspicious);
		return;
	}

	UpdateGameplayFocus();

	TimeSinceVisualContact += DeltaSeconds;
	if (TimeSinceVisualContact <= SightLostGracePeriod) return;

	if (AIState == ETGuardAIState::Combat)
	{
		SetAwareness(CombatLostSightAwareness);
		SetAIState(ETGuardAIState::Search);
		return;
	}

	if (AIState == ETGuardAIState::Suspicious || AIState == ETGuardAIState::Search)
	{
		SetAwareness(Awareness - AwarenessDecayRate * DeltaSeconds);
		if (AIState == ETGuardAIState::Suspicious && Awareness <= 0.f)
		{
			SetAIState(ETGuardAIState::Return);
		}
	}
}

float AT_ShooterAIController::GetSightAwarenessRate(float Distance)
{
	if (Distance <= 600.f) return 28.f;
	if (Distance <= 1200.f) return 14.f;
	return Distance <= 1800.f ? 6.f : 0.f;
}

float AT_ShooterAIController::ClampHearingAwareness(float CurrentAwareness, float StimulusStrength)
{
	return FMath::Clamp(CurrentAwareness + 25.f * FMath::Max(StimulusStrength, 0.f), 0.f, 80.f);
}

void AT_ShooterAIController::SetAIState(ETGuardAIState NewState)
{
	if (!HasAuthority()) return;
	if (AIState == NewState) return;
	const ETGuardAIState PreviousState = AIState;
	AIState = NewState;
	if (PreviousState != ETGuardAIState::Search && NewState == ETGuardAIState::Search) SearchElapsedTime = 0.f;
	else if (NewState != ETGuardAIState::Search) SearchElapsedTime = 0.f;

	const bool bCombatEntry = IsCombatEntry(PreviousState, NewState);
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsEnum(GuardBBKeys::AIState, static_cast<uint8>(AIState));
	}
	if (PreviousState == ETGuardAIState::Combat && NewState != ETGuardAIState::Combat)
	{
		CancelCombatAbilities();
		ClearFocus(EAIFocusPriority::Gameplay);
		StopMovement();
	}
	if (AT_GuardCharacter* Guard = GetGuardCharacter())
	{
		Guard->SetGuardAIPresentation(Awareness, AIState, bHasVisualContact);
		if (PreviousState == ETGuardAIState::Combat) Guard->SetCombatStrafeEnabled(false);
		if (PreviousState == ETGuardAIState::Return) Guard->SetReturnMovementEnabled(false);
		if (NewState == ETGuardAIState::Combat) Guard->SetCombatStrafeEnabled(true);
		if (NewState == ETGuardAIState::Return) Guard->SetReturnMovementEnabled(true);
		if (NewState == ETGuardAIState::Patrol)
		{
			Guard->SnapPatrolIndexToNearest(Guard->GetActorLocation());
		}
	}
	if (NewState == ETGuardAIState::Return)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
	}
	if (AT_QuestGameState* QuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr)
	{
		QuestGameState->NotifyGuardStateChanged(PreviousState, NewState);
	}
}

void AT_ShooterAIController::SetAwareness(float NewAwareness)
{
	if (!HasAuthority()) return;
	const float ClampedAwareness = FMath::Clamp(NewAwareness, 0.f, 100.f);
	if (FMath::IsNearlyEqual(Awareness, ClampedAwareness)) return;
	Awareness = ClampedAwareness;
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsFloat(GuardBBKeys::Awareness, Awareness);
	}
	if (AT_GuardCharacter* Guard = GetGuardCharacter())
	{
		Guard->SetGuardAIPresentation(Awareness, AIState, bHasVisualContact);
	}
}

void AT_ShooterAIController::BeginInvestigation(const FVector& Location)
{
	AT_GuardCharacter* Guard = GetGuardCharacter();
	if (!HasAuthority() || !IsValid(Guard) || !Guard->IsAlive() || AIState == ETGuardAIState::Combat) return;

	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(GuardBBKeys::InvestigateLocation, Location);
	}
	LastKnownTargetLocation = Location;
	SyncLastKnownLocation();
	SetAwareness(FMath::Max(Awareness, InvestigationMinAwareness));
	SetFocalPoint(Location, EAIFocusPriority::Gameplay);
	SetAIState(ETGuardAIState::Investigate);
}

void AT_ShooterAIController::ReactToDistraction(const FVector& Location)
{
	AT_GuardCharacter* Guard = GetGuardCharacter();
	if (!HasAuthority() || !IsValid(Guard) || !Guard->IsAlive() || AIState == ETGuardAIState::Combat) return;

	SetAwareness(FMath::Min(Awareness + DistractionAwarenessBoost, DistractionAwarenessCap));
	DistractionFocusLocation = Location;
	DistractionFocusRemaining = 2.f;
	SetFocalPoint(Location, EAIFocusPriority::Gameplay);
}

bool AT_ShooterAIController::ShouldConfirmContact(AActor* PlayerActor) const
{
	if (!IsValidPlayerTarget(PlayerActor)) return false;

	if (AIState == ETGuardAIState::Combat
		|| AIState == ETGuardAIState::Suspicious
		|| AIState == ETGuardAIState::Search)
	{
		return true;
	}

	return GetFacingDotToActor(PlayerActor) > ContactFrontFacingDot;
}

void AT_ShooterAIController::HandleRearContactNudge(AActor* PlayerActor)
{
	if (!HasAuthority() || !IsValidPlayerTarget(PlayerActor)) return;
	if (AIState == ETGuardAIState::Combat) return;

	SetTarget(PlayerActor);
	LastKnownTargetLocation = PlayerActor->GetActorLocation();
	SyncLastKnownLocation();
	SetAwareness(FMath::Max(Awareness, RearContactMinAwareness));
	if (AIState == ETGuardAIState::Patrol || AIState == ETGuardAIState::Return)
	{
		SetAIState(ETGuardAIState::Suspicious);
	}
}

void AT_ShooterAIController::ConfirmTargetFromDamage(AActor* InstigatorActor, const FVector& Location)
{
	if (!HasAuthority() || !IsValidPlayerTarget(InstigatorActor)) return;
	const bool bKeepVisualContact = CurrentTarget.Get() == InstigatorActor && bHasVisualContact;
	SetTarget(InstigatorActor);
	LastKnownTargetLocation = Location.IsNearlyZero() ? InstigatorActor->GetActorLocation() : Location;
	SyncLastKnownLocation();
	SetVisualContact(bKeepVisualContact);
	TimeSinceVisualContact = 0.f;
	SetAwareness(100.f);
	SetAIState(ETGuardAIState::Combat);
}

void AT_ShooterAIController::ConfirmTargetFromContact(AActor* PlayerActor)
{
	if (!HasAuthority() || !IsValidPlayerTarget(PlayerActor)) return;

	if (ShouldConfirmContact(PlayerActor))
	{
		ConfirmTargetFromDamage(PlayerActor, PlayerActor->GetActorLocation());
		return;
	}

	HandleRearContactNudge(PlayerActor);
}

void AT_ShooterAIController::CompleteCurrentBehaviorState()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	switch (AIState)
	{
	case ETGuardAIState::Investigate:
		if (IsValid(BB)) BB->ClearValue(GuardBBKeys::InvestigateLocation);
		SetAIState(ETGuardAIState::Return);
		break;
	case ETGuardAIState::Search:
		if (IsValid(BB)) BB->ClearValue(GuardBBKeys::CombatMoveLocation);
		SetAIState(ETGuardAIState::Return);
		break;
	case ETGuardAIState::Return:
		if (IsValid(BB))
		{
			BB->ClearValue(GuardBBKeys::MoveLocation);
			BB->ClearValue(GuardBBKeys::Enemy);
			BB->ClearValue(GuardBBKeys::LastKnownLocation);
			BB->ClearValue(GuardBBKeys::InvestigateLocation);
			BB->ClearValue(GuardBBKeys::CombatMoveLocation);
		}
		CurrentTarget = nullptr;
		PerceivedTargets.Reset();
		SetAwareness(0.f);
		SetAIState(ETGuardAIState::Patrol);
		RefreshPerceivedTargetsFromPerception();
		break;
	default:
		break;
	}
}

void AT_ShooterAIController::SyncLastKnownLocation()
{
	if (UBlackboardComponent* BB = GetBlackboardComponent())
	{
		BB->SetValueAsVector(GuardBBKeys::LastKnownLocation, LastKnownTargetLocation);
	}
}

void AT_ShooterAIController::UpdatePerceivedTarget(AActor* Actor, const FVector& Location)
{
	for (FGuardPerceivedTarget& Entry : PerceivedTargets)
	{
		if (Entry.Actor == Actor)
		{
			Entry.Location = Location;
			return;
		}
	}
	FGuardPerceivedTarget& Entry = PerceivedTargets.AddDefaulted_GetRef();
	Entry.Actor = Actor;
	Entry.Location = Location;
}

void AT_ShooterAIController::RemovePerceivedTarget(AActor* Actor)
{
	PerceivedTargets.RemoveAll([Actor](const FGuardPerceivedTarget& Entry)
	{
		return !Entry.Actor.IsValid() || Entry.Actor == Actor;
	});
}

void AT_ShooterAIController::SelectAndSetTarget()
{
	const FVector Origin = IsValid(GetPawn()) ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	SetTarget(SelectNearestValidTarget(PerceivedTargets, Origin));
}

void AT_ShooterAIController::SetTarget(AActor* Target)
{
	if (IsValid(Target))
	{
		CurrentTarget = Target;
		if (UBlackboardComponent* BB = GetBlackboardComponent()) BB->SetValueAsObject(GuardBBKeys::Enemy, Target);
	}
}

void AT_ShooterAIController::SetVisualContact(bool bNewVisualContact)
{
	if (bHasVisualContact == bNewVisualContact) return;
	bHasVisualContact = bNewVisualContact;
	if (AT_GuardCharacter* Guard = GetGuardCharacter())
	{
		Guard->SetGuardAIPresentation(Awareness, AIState, bHasVisualContact);
	}
}

AActor* AT_ShooterAIController::SelectNearestValidTarget(const TArray<FGuardPerceivedTarget>& Candidates, const FVector& Origin)
{
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	AActor* Best = nullptr;
	for (const FGuardPerceivedTarget& Candidate : Candidates)
	{
		AActor* Actor = Candidate.Actor.Get();
		const AT_BaseCharacter* TargetCharacter = Cast<AT_BaseCharacter>(Actor);
		if (!IsValid(TargetCharacter) || !TargetCharacter->IsAlive() || !Actor->ActorHasTag(CrashTags::Player)) continue;
		const float DistanceSquared = FVector::DistSquared(Origin, Candidate.Location);
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			Best = Actor;
		}
	}
	return Best;
}

void AT_ShooterAIController::CancelCombatAbilities()
{
	AT_GuardCharacter* Guard = GetGuardCharacter();
	UAbilitySystemComponent* ASC = IsValid(Guard) ? Guard->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(ASC)) return;

	FGameplayTagContainer AbilityTagsToCancel;
	AbilityTagsToCancel.AddTag(TTags::TAbilities::Enemy::Aim.GetTag());
	AbilityTagsToCancel.AddTag(TTags::TAbilities::Enemy::Shoot.GetTag());
	AbilityTagsToCancel.AddTag(TTags::TAbilities::Enemy::Reload.GetTag());
	ASC->CancelAbilities(&AbilityTagsToCancel);
}

void AT_ShooterAIController::ClearTargetState()
{
	CancelCombatAbilities();
	CurrentTarget = nullptr;
	LastKnownTargetLocation = FVector::ZeroVector;
	PerceivedTargets.Reset();
	SetVisualContact(false);
	TimeSinceVisualContact = 0.f;
	DistractionFocusRemaining = 0.f;
	DistractionFocusLocation = FVector::ZeroVector;
	bAlertObservationActive = false;
	ClearGuardBlackboard(GetBlackboardComponent());
	SetAwareness(0.f);
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AT_ShooterAIController::ClearGuardBlackboard(UBlackboardComponent* BlackboardComp)
{
	if (!IsValid(BlackboardComp)) return;
	BlackboardComp->ClearValue(GuardBBKeys::Enemy);
	BlackboardComp->ClearValue(GuardBBKeys::MoveLocation);
	BlackboardComp->ClearValue(GuardBBKeys::Awareness);
	BlackboardComp->ClearValue(GuardBBKeys::LastKnownLocation);
	BlackboardComp->ClearValue(GuardBBKeys::InvestigateLocation);
	BlackboardComp->ClearValue(GuardBBKeys::CombatMoveLocation);
}

void AT_ShooterAIController::OnGuardDied()
{
	StopMovement();
	if (AT_GuardCharacter* Guard = GetGuardCharacter()) Guard->SetCombatStrafeEnabled(false);
	if (UBrainComponent* Brain = GetBrainComponent()) Brain->StopLogic(TEXT("GuardDead"));
	if (IsValid(PerceptionComponent)) PerceptionComponent->Deactivate();
	ClearTargetState();
}

void AT_ShooterAIController::RestartGuardAI()
{
	StopMovement();
	ClearTargetState();
	if (IsValid(PerceptionComponent)) PerceptionComponent->Activate(true);
	if (AT_GuardCharacter* Guard = GetGuardCharacter()) Guard->ClearStaleBlockingTags();
	if (StartGuardBehaviorTree())
	{
		if (UBlackboardComponent* BB = GetBlackboardComponent()) BB->SetValueAsVector(GuardBBKeys::HomeLocation, GetPawn()->GetActorLocation());
		SetAIState(ETGuardAIState::Patrol);
	}
	RefreshPerceivedTargetsFromPerception();
}

void AT_ShooterAIController::RefreshPerceivedTargetsFromPerception()
{
	if (!IsValid(PerceptionComponent)) return;
	TArray<AActor*> PerceivedActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	for (AActor* Actor : PerceivedActors)
	{
		if (IsValidPlayerTarget(Actor)) UpdatePerceivedTarget(Actor, Actor->GetActorLocation());
	}
	SelectAndSetTarget();
	if (CurrentTarget.IsValid())
	{
		SetVisualContact(true);
		TimeSinceVisualContact = 0.f;
		LastKnownTargetLocation = CurrentTarget->GetActorLocation();
		SyncLastKnownLocation();
		SetAIState(ETGuardAIState::Suspicious);
	}
}

bool AT_ShooterAIController::StartGuardBehaviorTree()
{
	if (!IsValid(BehaviorTreeAsset) || !IsValid(BehaviorTreeAsset->BlackboardAsset))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: BehaviorTreeAsset 或其 BlackboardAsset 无效，Guard AI 已停止。"), *GetName());
		if (UBrainComponent* Brain = GetBrainComponent()) Brain->StopLogic(TEXT("InvalidGuardBehaviorTree"));
		return false;
	}

	const bool bStarted = RunBehaviorTree(BehaviorTreeAsset);
	if (!bStarted)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: RunBehaviorTree(%s) 失败，Guard AI 已停止。"), *GetName(), *BehaviorTreeAsset->GetName());
	}
	return bStarted;
}
