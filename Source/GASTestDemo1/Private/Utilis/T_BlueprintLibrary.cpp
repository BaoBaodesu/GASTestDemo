// Fill out your copyright notice in the Description page of Project Settings.


#include "Utilis/T_BlueprintLibrary.h"

#include "InterchangeTranslatorBase.h"
#include "Characters/T_BaseCharacter.h"
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

/**
  * 在场景中找出“带指定Tag的Actor里，距离某个点最近的那个Actor”
  */
FClosestActorWithTagResult UT_BlueprintLibrary::FindClosestActorWithTag(const UObject* WorldContextObject,
	const FVector& Origin, const FName& Tag)
{
	TArray<AActor*> ActorsWithTag;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject, Tag, ActorsWithTag);
	
	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;
	
	// 遍历所有符合 Tag 的 Actor
	for (AActor* Actor : ActorsWithTag)
	{
		if (!IsValid(Actor)) continue;
		AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(Actor);
		if (!IsValid(BaseCharacter) || !BaseCharacter->IsAlive()) continue;
		
		// 计算 Origin 到 Actor 的距离
		const float Distance = FVector::Dist(Origin, Actor->GetActorLocation());
		// 找到更近的目标则更新
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
