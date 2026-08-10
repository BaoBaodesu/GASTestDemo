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
class UBehaviorTreeComponent;
class UBehaviorTree;
class UBlackboardComponent;
class AT_GuardCharacter;
struct FAIStimulus;

// Guard 行为树统一使用的黑板键
namespace GuardBBKeys
{
	extern GASTESTDEMO1_API const FName Enemy;
	extern GASTESTDEMO1_API const FName EnemySpotted;
	extern GASTESTDEMO1_API const FName MoveLocation;
	extern GASTESTDEMO1_API const FName NoiseLocation;
}

// 感知到的候选目标（Actor + 感知位置），用于最近目标选择
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
 * 人形持枪敌人的原生 AI 控制器：原生创建感知，只选择存活且带 Player 标签的最近目标。
 */
UCLASS()
class GASTESTDEMO1_API AT_ShooterAIController : public AAIController
{
	GENERATED_BODY()

public:

	AT_ShooterAIController(const FObjectInitializer& ObjectInitializer);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	AT_GuardCharacter* GetGuardCharacter() const;

	// 当前目标（可能为空）
	UFUNCTION(BlueprintPure, Category = "Guard|AI")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	// 最后已知目标位置
	FVector GetLastKnownTargetLocation() const { return LastKnownTargetLocation; }

	// 死亡时停止移动、感知、行为树并清理目标状态
	void OnGuardDied();

	// 复活时重新激活感知并重启行为树
	void RestartGuardAI();

	// 清理 Guard 目标状态与黑板键
	void ClearTargetState();

	// 从候选目标中选择最近的、仍存活且带 Player 标签的目标（供测试与感知更新复用）
	static AActor* SelectNearestValidTarget(const TArray<FGuardPerceivedTarget>& Candidates, const FVector& Origin);

	// 清理 Guard 行为树统一使用的黑板键
	static void ClearGuardBlackboard(UBlackboardComponent* BlackboardComp);

	// 感知参数（蓝图仅配置这些数值）
	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float SightRadius{3000.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float LoseSightRadius{3500.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float PeripheralVisionAngleDegrees{110.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float HearingRange{1500.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float SightMaxAge{5.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float HearingMaxAge{3.f};

	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Perception")
	float DamageMaxAge{5.f};

	// 行为树资源（蓝图仅配置此项）
	UPROPERTY(EditDefaultsOnly, Category = "Guard|AI|Behavior")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

protected:

	virtual void BeginPlay() override;

private:

	void ConfigurePerceptionSenses();

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void UpdatePerceivedTarget(AActor* Actor, const FVector& Location);
	void RemovePerceivedTarget(AActor* Actor);
	void SelectAndSetTarget();
	void SetTarget(AActor* Target);

	// BT_Guard1 资产节点引用断裂时，用原生节点运行时拼装可用行为树
	UBehaviorTree* CreateRuntimeGuardBehaviorTree();
	bool StartGuardBehaviorTree();
	void RefreshPerceivedTargetsFromPerception();

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	UPROPERTY()
	TObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent;

	UPROPERTY()
	TObjectPtr<UBlackboardComponent> BlackboardComponent;

	UPROPERTY()
	TObjectPtr<UBehaviorTree> RuntimeBehaviorTree;

	TArray<FGuardPerceivedTarget> PerceivedTargets;

	TWeakObjectPtr<AActor> CurrentTarget;
	FVector LastKnownTargetLocation{ForceInit};
};
