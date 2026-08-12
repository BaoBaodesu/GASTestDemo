// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "T_ShooterAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UAISenseConfig_Hearing;
class UAISenseConfig_Damage;
class UBehaviorTree;
class UBlackboardComponent;
class AT_GuardCharacter;
struct FAIStimulus;

UENUM(BlueprintType)
enum class ETGuardAIState : uint8
{
	Patrol,
	Suspicious,
	Investigate,
	Combat,
	Search,
	Return
};

// Guard 行为树统一使用的黑板键
namespace GuardBBKeys
{
	extern GASTESTDEMO1_API const FName Enemy;
	extern GASTESTDEMO1_API const FName MoveLocation;
	extern GASTESTDEMO1_API const FName AIState;
	extern GASTESTDEMO1_API const FName Awareness;
	extern GASTESTDEMO1_API const FName LastKnownLocation;
	extern GASTESTDEMO1_API const FName InvestigateLocation;
	extern GASTESTDEMO1_API const FName HomeLocation;
	extern GASTESTDEMO1_API const FName CombatMoveLocation;
}

USTRUCT(BlueprintType)
struct GASTESTDEMO1_API FGuardPerceivedTarget
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY()
	FVector Location{ForceInit};
};

/**
 * 人形持枪敌人的原生 AI 控制器：服务器权威维护感知、警觉度和状态，行为树只负责调度动作。
 */
UCLASS()
class GASTESTDEMO1_API AT_ShooterAIController : public AAIController
{
	GENERATED_BODY()

public:
	AT_ShooterAIController(const FObjectInitializer& ObjectInitializer);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	AT_GuardCharacter* GetGuardCharacter() const;

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	ETGuardAIState GetAIState() const { return AIState; }

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	float GetAwareness() const { return Awareness; }

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	bool HasVisualContact() const { return bHasVisualContact; }

	FVector GetLastKnownTargetLocation() const { return LastKnownTargetLocation; }

	void SetAIState(ETGuardAIState NewState);
	void SetAwareness(float NewAwareness);
	void BeginInvestigation(const FVector& Location);
	void ReactToDistraction(const FVector& Location);
	void ConfirmTargetFromDamage(AActor* InstigatorActor, const FVector& Location);
	void ConfirmTargetFromContact(AActor* PlayerActor);
	void CompleteCurrentBehaviorState();
	void SyncLastKnownLocation();
	void SetAlertObservationActive(bool bActive) { bAlertObservationActive = bActive; }

	void OnGuardDied();
	void RestartGuardAI();
	void ClearTargetState();

	static AActor* SelectNearestValidTarget(const TArray<FGuardPerceivedTarget>& Candidates, const FVector& Origin);
	static void ClearGuardBlackboard(UBlackboardComponent* BlackboardComp);
	static float GetSightAwarenessRate(float Distance);
	static float ClampHearingAwareness(float CurrentAwareness, float StimulusStrength);
	static float GetFacingAwarenessMultiplier(const FVector& GuardForward, const FVector& ToTarget);
	static bool ShouldInstantDetectCloseRange(float Distance, float FacingDot);
	static bool ShouldEnterSuspiciousFromSight(ETGuardAIState CurrentState);
	static bool ShouldInstantReengageFromSight(ETGuardAIState CurrentState)
	{
		return CurrentState == ETGuardAIState::Combat
			|| CurrentState == ETGuardAIState::Search
			|| CurrentState == ETGuardAIState::Investigate;
	}
	static bool IsAlertAimingState(ETGuardAIState CurrentState)
	{
		return CurrentState == ETGuardAIState::Suspicious
			|| CurrentState == ETGuardAIState::Investigate
			|| CurrentState == ETGuardAIState::Combat
			|| CurrentState == ETGuardAIState::Search;
	}
	static bool IsCombatEntry(ETGuardAIState PreviousState, ETGuardAIState NewState)
	{
		return PreviousState != ETGuardAIState::Combat && NewState == ETGuardAIState::Combat;
	}

	bool ShouldConfirmContact(AActor* PlayerActor) const;

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float SightRadius{1800.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float LoseSightRadius{2100.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float PeripheralVisionAngleDegrees{50.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float HearingRange{1800.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float SightMaxAge{5.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float HearingMaxAge{3.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float DamageMaxAge{5.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Awareness")
	float SightLostGracePeriod{1.2f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Awareness")
	float AwarenessDecayRate{16.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Behavior", meta = (ClampMin = "0.0"))
	float SearchTimeout{30.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

private:
	void ConfigurePerceptionSenses();

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void UpdateAwareness(float DeltaSeconds);
	void UpdateGameplayFocus();
	void HandleSightStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void HandleHearingStimulus(AActor* Actor, const FAIStimulus& Stimulus);
	void UpdatePerceivedTarget(AActor* Actor, const FVector& Location);
	void RemovePerceivedTarget(AActor* Actor);
	void SelectAndSetTarget();
	void SetTarget(AActor* Target);
	void SetVisualContact(bool bNewVisualContact);
	void CancelCombatAbilities();
	bool StartGuardBehaviorTree();
	void RefreshPerceivedTargetsFromPerception();
	bool IsValidPlayerTarget(AActor* Actor) const;
	float GetFacingDotToActor(AActor* Actor) const;
	void HandleRearContactNudge(AActor* PlayerActor);

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	TArray<FGuardPerceivedTarget> PerceivedTargets;
	TWeakObjectPtr<AActor> CurrentTarget;
	FVector LastKnownTargetLocation{ForceInit};
	ETGuardAIState AIState{ETGuardAIState::Patrol};
	float Awareness{0.f};
	float TimeSinceVisualContact{0.f};
	float SearchElapsedTime{0.f};
	float DistractionFocusRemaining{0.f};
	FVector DistractionFocusLocation{ForceInit};
	bool bHasVisualContact{false};
	bool bAlertObservationActive{false};
};
