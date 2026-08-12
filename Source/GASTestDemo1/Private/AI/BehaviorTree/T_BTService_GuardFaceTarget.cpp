// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BehaviorTree/T_BTService_GuardFaceTarget.h"

#include "AI/T_ShooterAIController.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTreeComponent.h"

UT_BTService_GuardFaceTarget::UT_BTService_GuardFaceTarget()
{
	NodeName = TEXT("Guard Face Target");
	Interval = 0.1f;
	RandomDeviation = 0.02f;
}

void UT_BTService_GuardFaceTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	AT_ShooterAIController* ShooterController = Cast<AT_ShooterAIController>(AIController);
	if (!IsValid(ShooterController)) return;

	AActor* Target = ShooterController->GetCurrentTarget();
	if (IsValid(Target) && ShooterController->HasVisualContact())
	{
		AIController->SetFocus(Target, EAIFocusPriority::Gameplay);
	}
	else
	{
		UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
		if (IsValid(BlackboardComp) && BlackboardComp->IsVectorValueSet(GuardBBKeys::LastKnownLocation))
		{
			AIController->SetFocalPoint(BlackboardComp->GetValueAsVector(GuardBBKeys::LastKnownLocation), EAIFocusPriority::Gameplay);
		}
		else if (IsValid(BlackboardComp) && BlackboardComp->IsVectorValueSet(GuardBBKeys::InvestigateLocation))
		{
			AIController->SetFocalPoint(BlackboardComp->GetValueAsVector(GuardBBKeys::InvestigateLocation), EAIFocusPriority::Gameplay);
		}
		else
		{
			AIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
	}
}
