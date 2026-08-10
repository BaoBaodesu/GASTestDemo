// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTService_GuardUpdateSight.h"

#include "AI/T_ShooterAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UT_BTService_GuardUpdateSight::UT_BTService_GuardUpdateSight()
{
	NodeName = TEXT("Guard Update Sight");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UT_BTService_GuardUpdateSight::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(AIController);
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!IsValid(ShooterController) || !IsValid(BlackboardComp)) return;

	AActor* Target = ShooterController->GetCurrentTarget();
	if (!IsValid(Target) || !IsValid(AIController))
	{
		BlackboardComp->SetValueAsBool(GuardBBKeys::EnemySpotted, false);
		return;
	}

	if (AIController->LineOfSightTo(Target, FVector::ZeroVector, true))
	{
		BlackboardComp->SetValueAsObject(GuardBBKeys::Enemy, Target);
		BlackboardComp->SetValueAsBool(GuardBBKeys::EnemySpotted, true);
		BlackboardComp->SetValueAsVector(GuardBBKeys::MoveLocation, Target->GetActorLocation());
	}
	// 视线被短暂遮挡时不立即清除 Enemy Spotted：
	// 由感知系统的"丢失目标"刺激（HandleTargetPerceptionUpdated 失败分支）负责清除，
	// 避免单次 trace 误判导致 Combat 分支被反复中断。
}
