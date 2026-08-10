#include "Notifies/AnimNotifyState_ComboHit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/T_PrimaryComboAbility.h"
#include "Characters/T_BaseCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "KismetTraceUtils.h"

void UAnimNotifyState_ComboHit::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!IsValid(MeshComp) || !IsValid(MeshComp->GetOwner()) || SocketNames.IsEmpty()) return;
	if (!MeshComp->GetOwner()->HasAuthority()) return;

	FComboHitWindowState& WindowState = ActiveWindows.FindOrAdd(MeshComp);
	WindowState.ActorsAlreadyHit.Reset();
	CollectSocketLocations(MeshComp, WindowState.PrevSocketLocations);
}

void UAnimNotifyState_ComboHit::NotifyTick(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float FrameDeltaTime,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!IsValid(MeshComp) || !IsValid(MeshComp->GetOwner()) || SocketNames.IsEmpty()) return;
	if (!MeshComp->GetOwner()->HasAuthority()) return;

	FComboHitWindowState* WindowState = ActiveWindows.Find(MeshComp);
	if (!WindowState) return;

	TArray<FVector> CurrentLocations;
	CollectSocketLocations(MeshComp, CurrentLocations);
	if (CurrentLocations.Num() != WindowState->PrevSocketLocations.Num())
	{
		WindowState->PrevSocketLocations = CurrentLocations;
		return;
	}

	for (int32 Index = 0; Index < CurrentLocations.Num(); ++Index)
	{
		ProcessHitsForSocket(MeshComp, WindowState->PrevSocketLocations[Index], CurrentLocations[Index], *WindowState);
	}

	WindowState->PrevSocketLocations = MoveTemp(CurrentLocations);
}

void UAnimNotifyState_ComboHit::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (IsValid(MeshComp))
	{
		ActiveWindows.Remove(MeshComp);
	}
}

FVector UAnimNotifyState_ComboHit::GetSocketWorldLocation(const USkeletalMeshComponent* MeshComp, FName SocketName) const
{
	if (!IsValid(MeshComp) || SocketName.IsNone()) return FVector::ZeroVector;

	const FTransform SocketTransform = MeshComp->DoesSocketExist(SocketName)
		? MeshComp->GetSocketTransform(SocketName, RTS_World)
		: MeshComp->GetComponentTransform();

	return SocketTransform.TransformPosition(SocketLocationOffset);
}

void UAnimNotifyState_ComboHit::CollectSocketLocations(const USkeletalMeshComponent* MeshComp, TArray<FVector>& OutLocations) const
{
	OutLocations.Reset(SocketNames.Num());
	for (const FName SocketName : SocketNames)
	{
		if (SocketName.IsNone()) continue;
		OutLocations.Add(GetSocketWorldLocation(MeshComp, SocketName));
	}
}

UT_PrimaryComboAbility* UAnimNotifyState_ComboHit::FindActiveComboAbility(AActor* Owner) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!IsValid(ASC)) return nullptr;

	if (UT_PrimaryComboAbility* AnimatingCombo = Cast<UT_PrimaryComboAbility>(ASC->GetAnimatingAbility()))
	{
		return AnimatingCombo;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive()) continue;
		if (UT_PrimaryComboAbility* ComboAbility = Cast<UT_PrimaryComboAbility>(Spec.GetPrimaryInstance()))
		{
			return ComboAbility;
		}
	}

	return nullptr;
}

void UAnimNotifyState_ComboHit::ProcessHitsForSocket(
	USkeletalMeshComponent* MeshComp,
	const FVector& Start,
	const FVector& End,
	FComboHitWindowState& WindowState)
{
	AActor* Owner = MeshComp->GetOwner();
	UWorld* World = MeshComp->GetWorld();
	if (!IsValid(Owner) || !IsValid(World)) return;

	// 静止帧也做一次极短 Sweep，避免贴身静止命中漏检
	FVector SweepEnd = End;
	if (FVector::DistSquared(Start, End) < KINDA_SMALL_NUMBER)
	{
		SweepEnd = Start + FVector(0.f, 0.f, 1.f);
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ComboHitNotify), false, Owner);
	Params.AddIgnoredActor(Owner);

	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits,
		Start,
		SweepEnd,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(SphereRadius),
		Params,
		ResponseParams);

	if (bDrawDebug)
	{
		DrawDebugSphereTraceMulti(
			World,
			Start,
			SweepEnd,
			SphereRadius,
			EDrawDebugTrace::ForOneFrame,
			bHit,
			Hits,
			FColor::Green,
			FColor::Red,
			0.f);
	}

	UT_PrimaryComboAbility* ComboAbility = FindActiveComboAbility(Owner);
	if (!IsValid(ComboAbility)) return;

	for (const FHitResult& Hit : Hits)
	{
		AT_BaseCharacter* TargetCharacter = Cast<AT_BaseCharacter>(Hit.GetActor());
		if (!IsValid(TargetCharacter) || !TargetCharacter->IsAlive()) continue;

		const bool bAlreadyHit = WindowState.ActorsAlreadyHit.ContainsByPredicate(
			[TargetCharacter](const TWeakObjectPtr<AActor>& Entry)
			{
				return Entry.Get() == TargetCharacter;
			});
		if (bAlreadyHit) continue;

		if (ComboAbility->ApplyComboHitTarget(TargetCharacter, &Hit))
		{
			WindowState.ActorsAlreadyHit.Add(TargetCharacter);
		}
	}
}
