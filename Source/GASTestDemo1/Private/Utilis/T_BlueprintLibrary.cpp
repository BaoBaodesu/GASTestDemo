// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/T_BlueprintLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_EnemyCharacter.h"
#include "GameplayTags/TTags.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

EHitDirection UT_BlueprintLibrary::GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator)
{
	const float Dot = FVector::DotProduct(TargetForward, ToInstigator);
	
	if (Dot < -0.5f)
	{
		return EHitDirection::Back;
	}
	if (Dot < 0.5f)
	{
		const FVector Cross = FVector::CrossProduct(TargetForward, ToInstigator);
		if (Cross.Z < 0.0f)
		{
			return EHitDirection::Left;
		}
		return EHitDirection::Right;
	}
	return EHitDirection::Forward;
}

FName UT_BlueprintLibrary::GetHitDirectionName(const EHitDirection& HitDirection)
{
	switch (HitDirection)
	{
		case EHitDirection::Left: return FName("Left");
		case EHitDirection::Right: return FName("Right");
		case EHitDirection::Forward: return FName("Forward");
		case EHitDirection::Back: return FName("Back");
		default: return FName("None");
	}
}

ERollDirection UT_BlueprintLibrary::GetRollDirectionFromInput(const APawn* Pawn, const FVector2D& MovementInput)
{
	if (!IsValid(Pawn) || MovementInput.IsNearlyZero(0.1f))
	{
		return ERollDirection::Forward;
	}

	const FRotator ControlYawRotation(0.f, Pawn->GetControlRotation().Yaw, 0.f);

	const FVector WorldInputDirection =
		ControlYawRotation.Vector() * MovementInput.Y +
		FRotationMatrix(ControlYawRotation).GetUnitAxis(EAxis::Y) * MovementInput.X;

	const FVector LocalInputDirection =
		Pawn->GetActorTransform().InverseTransformVectorNoScale(WorldInputDirection).GetSafeNormal();

	const float Angle = FMath::RadiansToDegrees(
		FMath::Atan2(LocalInputDirection.Y, LocalInputDirection.X)
	);

	if (Angle >= -22.5f && Angle < 22.5f)
	{
		return ERollDirection::Forward;
	}
	if (Angle >= 22.5f && Angle < 67.5f)
	{
		return ERollDirection::ForwardRight;
	}
	if (Angle >= 67.5f && Angle < 112.5f)
	{
		return ERollDirection::Right;
	}
	if (Angle >= 112.5f && Angle < 157.5f)
	{
		return ERollDirection::BackRight;
	}
	if (Angle >= 157.5f || Angle < -157.5f)
	{
		return ERollDirection::Back;
	}
	if (Angle >= -157.5f && Angle < -112.5f)
	{
		return ERollDirection::BackLeft;
	}
	if (Angle >= -112.5f && Angle < -67.5f)
	{
		return ERollDirection::Left;
	}
	if (Angle >= -67.5f && Angle < -22.5f)
	{
		return ERollDirection::ForwardLeft;
	}

	return ERollDirection::Forward;
}

FName UT_BlueprintLibrary::GetRollDirectionName(const ERollDirection& RollDirection)
{
	switch (RollDirection)
	{
		case ERollDirection::Forward: return FName("Forward");
		case ERollDirection::ForwardRight: return FName("ForwardRight");
		case ERollDirection::Right: return FName("Right");
		case ERollDirection::BackRight: return FName("BackRight");
		case ERollDirection::Back: return FName("Back");
		case ERollDirection::BackLeft: return FName("BackLeft");
		case ERollDirection::Left: return FName("Left");
		case ERollDirection::ForwardLeft: return FName("ForwardLeft");
		default: return FName("None");
	}
}

/**
  * ?????????????????Tag??Actor???????????????????Actor??
  */
FClosestActorWithTagResult UT_BlueprintLibrary::FindClosestActorWithTag(UObject* WorldContextObject,
	const FVector& Origin, const FName& Tag, float SearchRange)
{
	
	TArray<AActor*> ActorsWithTag;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject, Tag, ActorsWithTag);
	
	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;
	
	// ??????????? Tag ?? Actor
	for (AActor* Actor : ActorsWithTag)
	{
		if (!IsValid(Actor)) continue;
		AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(Actor);
		if (!IsValid(BaseCharacter) || !BaseCharacter->IsAlive()) continue;
		
		// ???? Origin ?? Actor ?????
		const float Distance = FVector::Dist(Origin, Actor->GetActorLocation());
		
		AT_EnemyCharacter* SearchingCharacter = Cast<AT_EnemyCharacter>(WorldContextObject);
		if (IsValid(SearchingCharacter))
		{
			if (Distance > SearchingCharacter->SearchRange) continue;
		}
		// ?????????????????
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = Actor;
		}
	}
	
	FClosestActorWithTagResult Result;
	Result.Actor = ClosestActor;
	Result.Distance = ClosestDistance;
	
	return Result;
}

bool UT_BlueprintLibrary::IsInvincible(AActor* Actor)
{
	if (!IsValid(Actor)) return false;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Actor);
	if (!IsValid(ASC)) return false;

	return ASC->HasMatchingGameplayTag(TTags::State::Action::Invincible);
}

void UT_BlueprintLibrary::SendDamageEventToPlayer(AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect,
	FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag &EventTagOverride, UObject* OptionalParticleSystem)
{
	AT_BaseCharacter* PlayerCharacter = Cast<AT_BaseCharacter>(Target);
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsAlive()) return;
	if (IsInvincible(PlayerCharacter)) return;

	FGameplayTag EventTag;
	if (!EventTagOverride.MatchesTagExact(TTags::None))
	{
		EventTag = EventTagOverride;
	}
	else
	{
		UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet());
		if (!IsValid(AttributeSet)) return;

		const bool bLethal = AttributeSet->GetHealth() - Damage <= 0.0f;
		EventTag = bLethal ? TTags::Events::Player::Death : TTags::Events::Player::HitReact;
	}

	Payload.OptionalObject = OptionalParticleSystem;

	UAbilitySystemComponent* TargetASC = PlayerCharacter->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return;

	TargetASC->HandleGameplayEvent(EventTag, &Payload);

	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect, 1.0f, ContextHandle);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DataTag, -Damage);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}


void UT_BlueprintLibrary::SendDamageEventToPlayers(TArray<AActor*> Targets,
	const TSubclassOf<UGameplayEffect>& DamageEffect, FGameplayEventData& Payload, const FGameplayTag& DataTag,
	float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem)
{
	for (AActor* Target : Targets)
	{
		SendDamageEventToPlayer(Target, DamageEffect, Payload, DataTag, Damage, EventTagOverride, OptionalParticleSystem);
	}
}

TArray<AActor*> UT_BlueprintLibrary::HitBoxOverlapTest(AActor* AvatarActor, float HitBoxRadius,
                                                       float HitBoxForwardOffset, float HitBoxElevationOffset, bool bDrawDebugs)
{
	if (!IsValid(AvatarActor)) return TArray<AActor*>();
	// ???????
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);

	// ???????????? Avatar Actor??
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(ActorsToIgnore);
	QueryParams.AddIgnoredActor(AvatarActor);

	// ???? Pawn
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	// ??????????
	TArray<FOverlapResult> OverlapResults;
	// ????????????
	FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);

	// ?????????
	const FVector Forward = AvatarActor->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0.f, 0.f, HitBoxElevationOffset);
	
	UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return TArray<AActor*>();
	// ?????????????
	World->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity, ECC_Visibility, Sphere, QueryParams, ResponseParams);

	// ???????????
	TArray<AActor*> ActorsHit;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(Result.GetActor());
		if (!IsValid(BaseCharacter)) continue;
		if (!BaseCharacter->IsAlive()) continue;
		// ???????????
		ActorsHit.AddUnique(BaseCharacter);		
	}

	if (bDrawDebugs)
	{
		DrawHitBoxOverlapDebugs(AvatarActor,OverlapResults, HitBoxLocation, HitBoxRadius);
	}
	
	return ActorsHit;
}

void UT_BlueprintLibrary::DrawHitBoxOverlapDebugs(const UObject* WorldContextObject,
	const TArray<FOverlapResult>& OverlapResults, const FVector& HitBoxLocation, float HitBoxRadius)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return;
	
	// ??????????
	DrawDebugSphere(World, HitBoxLocation, HitBoxRadius, 16, FColor::Red, false, 3.0f);
	
	// ??????????????
	for (const FOverlapResult& Result : OverlapResults)
	{
		if (IsValid(Result.GetActor()))
		{
			FVector DebugLocation = Result.GetActor()->GetActorLocation();
			DebugLocation.Z += 100.f;
			DrawDebugSphere(World, DebugLocation, 30.f, 10, FColor::Green, false, 3.0);
		}
	}
}

// ????????????? HitActors ??????????
TArray<AActor*> UT_BlueprintLibrary::ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors,
	float InnerRadius, float OuterRadius, float LaunchForceMagnitude, float RotationAngle, bool bDrawDebugs)
{
	for (AActor* HitActor : HitActors)
	{
		// ????? Character ????????
		ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
		if (!IsValid(HitCharacter) || !IsValid(AvatarActor)) return TArray<AActor*>();
		
		// ????????????????
		const FVector HitCharacterLocation = HitCharacter->GetActorLocation();
		const FVector AvatarLocation = AvatarActor->GetActorLocation();
		
		// ????????????????????
		const FVector ToHitActor = HitCharacterLocation - AvatarLocation;
		
		// ???????????????????
		const float Distance = FVector::Dist(AvatarLocation, HitCharacterLocation);
		
		// ????????
		float LaunchForce = 0.0f;
		// ???????????????
		if (Distance > OuterRadius) continue;
		
		// ?????????????????
		if (Distance <= InnerRadius)
		{
			LaunchForce = LaunchForceMagnitude;
		}
		else
		{
			// ??????????????????????
			const FVector2D FalloffRange(InnerRadius, OuterRadius);
			const FVector2D LaunchForceRange(LaunchForceMagnitude, 0.0f);
			LaunchForce = FMath::GetMappedRangeValueClamped(FalloffRange, LaunchForceRange, Distance);
		}
		// ???????????
		if (bDrawDebugs)GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("LaunchForce: %f"), LaunchForce));
		
		// ??????????
		FVector KnockbackForce = ToHitActor.GetSafeNormal();
		// ????? Z?????????????????????
		KnockbackForce.Z = 0.0f;
		
		// ???????????????????????
		const FVector Right = KnockbackForce.RotateAngleAxis(90.0f, FVector::UpVector);
		
		// ???? RotationAngle ????????????????????????
		KnockbackForce = KnockbackForce.RotateAngleAxis(-RotationAngle, Right) * LaunchForce;
		
		// ????????????
		if (bDrawDebugs)
		{
			UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor, EGetWorldErrorMode::LogAndReturnNull);
			DrawDebugDirectionalArrow(World, HitCharacterLocation, HitCharacterLocation + KnockbackForce, 100.f, FColor::Green, false, 3.f);
		}
		
		AT_EnemyCharacter* EnemyCharacter = Cast<AT_EnemyCharacter>(HitCharacter);
		if (IsValid(EnemyCharacter))
		{
			EnemyCharacter->StopMovementUntilLanded();
		}
		
		// ??????????
		HitCharacter->LaunchCharacter(KnockbackForce, true, true);
	}	
	return HitActors;
}
