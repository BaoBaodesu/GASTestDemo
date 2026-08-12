#include "Animations/T_GuardAnimInstance.h"

#include "AI/T_ShooterAIController.h"
#include "Characters/T_GuardCharacter.h"

UT_GuardAnimInstance::UT_GuardAnimInstance()
{
	bAlwaysAiming = false;
}

void UT_GuardAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	const AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(TryGetPawnOwner());
	bAlwaysAiming = IsValid(Guard) && AT_ShooterAIController::IsAlertAimingState(Guard->GetGuardAIState());
	Super::NativeUpdateAnimation(DeltaSeconds);
}
