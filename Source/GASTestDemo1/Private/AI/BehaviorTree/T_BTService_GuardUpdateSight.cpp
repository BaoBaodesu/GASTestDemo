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

	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(OwnerComp.GetAIOwner());
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!IsValid(ShooterController) || !IsValid(BlackboardComp)) return;

	AActor* Target = ShooterController->GetCurrentTarget();
	if (IsValid(Target))
	{
		BlackboardComp->SetValueAsObject(GuardBBKeys::Enemy, Target);
		if (ShooterController->HasVisualContact()) ShooterController->SyncLastKnownLocation();
	}
}
