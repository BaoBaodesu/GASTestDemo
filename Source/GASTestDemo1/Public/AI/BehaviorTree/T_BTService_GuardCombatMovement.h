#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "T_BTService_GuardCombatMovement.generated.h"

UCLASS()
class GASTESTDEMO1_API UT_BTService_GuardCombatMovement : public UBTService
{
	GENERATED_BODY()

public:
	UT_BTService_GuardCombatMovement();

	UPROPERTY(EditAnywhere, Category = "Guard|Combat Movement")
	FBlackboardKeySelector CombatMoveLocationKey;

	UPROPERTY(EditAnywhere, Category = "Guard|Combat Movement")
	FVector NavigationQueryExtent{100.f, 100.f, 200.f};

	static bool IsProjectedMoveUsable(bool bProjected, bool bPathValid);

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
