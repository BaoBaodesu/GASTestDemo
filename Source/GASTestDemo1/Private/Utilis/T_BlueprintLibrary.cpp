// Fill out your copyright notice in the Description page of Project Settings.


#include "Utils/T_BlueprintLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "InterchangeTranslatorBase.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Characters/T_BaseCharacter.h"
#include "GameplayTags/TTags.h"
#include "Engine/OverlapResult.h"
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

void UT_BlueprintLibrary::SendDamageEventToPlayer(AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect,
	FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, UObject* OptionalParticleSystem)
{
	AT_BaseCharacter* PlayerCharacter = Cast<AT_BaseCharacter>(Target);
	if (!IsValid(PlayerCharacter)) return;
	if (!PlayerCharacter->IsAlive()) return;
	
	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet());
	if (!IsValid(AttributeSet)) return;
	
	const bool bLethal = AttributeSet->GetHealth() - Damage <= 0.0f;
	const FGameplayTag EvenTag = bLethal ? TTags::Events::Player::Death : TTags::Events::Player::HitReact;
	
	Payload.OptionalObject = OptionalParticleSystem;
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PlayerCharacter, EvenTag, Payload);
	
	UAbilitySystemComponent* TargetASC = PlayerCharacter->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return;
	
	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect, 1.0f, ContextHandle);
	
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, DataTag, -Damage);
	
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	
}


TArray<AActor*> UT_BlueprintLibrary::HitBoxOverlapTest(AActor* AvatarActor, float HitBoxRadius,
	float HitBoxForwardOffset, float HitBoxElevationOffset, bool bDrawDebugs)
{
	if (!IsValid(AvatarActor)) return TArray<AActor*>();
	// 忽略自己
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);

	// 确保重叠检测忽略 Avatar Actor。
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActors(ActorsToIgnore);
	QueryParams.AddIgnoredActor(AvatarActor);

	// 只检测 Pawn
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);

	// 保存重叠结果
	TArray<FOverlapResult> OverlapResults;
	// 创建球形检测体
	FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);

	// 计算检测位置
	const FVector Forward = AvatarActor->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0.f, 0.f, HitBoxElevationOffset);
	
	UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return TArray<AActor*>();
	// 执行球形重叠检测
	World->OverlapMultiByChannel(OverlapResults, HitBoxLocation, FQuat::Identity, ECC_Visibility, Sphere, QueryParams, ResponseParams);

	// 保存命中的角色
	TArray<AActor*> ActorsHit;
	for (const FOverlapResult& Result : OverlapResults)
	{
		AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(Result.GetActor());
		if (!IsValid(BaseCharacter)) continue;
		if (!BaseCharacter->IsAlive()) continue;
		// 避免重复添加
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
	
	// 绘制检测球体
	DrawDebugSphere(World, HitBoxLocation, HitBoxRadius, 16, FColor::Red, false, 3.0f);
	
	// 绘制命中目标位置
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

// 对命中的角色数组 HitActors 进行击退处理
TArray<AActor*> UT_BlueprintLibrary::ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors,
	float InnerRadius, float OuterRadius, float LaunchForceMagnitude, float RotationAngle, bool bDrawDebugs)
{
	for (AActor* HitActor : HitActors)
	{
		// 只处理 Character 类型的目标
		ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
		if (!IsValid(HitCharacter) || !IsValid(AvatarActor)) return TArray<AActor*>();
		
		// 获取被击中角色的位置
		const FVector HitCharacterLocation = HitCharacter->GetActorLocation();
		const FVector AvatarLocation = AvatarActor->GetActorLocation();
		
		// 计算从攻击者指向目标的方向
		const FVector ToHitActor = HitCharacterLocation - AvatarLocation;
		
		// 计算攻击者和目标之间的距离
		const float Distance = FVector::Dist(AvatarLocation, HitCharacterLocation);
		
		// 击退力度
		float LaunchForce = 0.0f;
		// 超出外半径，不击退
		if (Distance > OuterRadius) continue;
		
		// 内半径内，使用最大击退力
		if (Distance <= InnerRadius)
		{
			LaunchForce = LaunchForceMagnitude;
		}
		else
		{
			// 内半径到外半径之间，击退力逐渐衰减
			const FVector2D FalloffRange(InnerRadius, OuterRadius);
			const FVector2D LaunchForceRange(LaunchForceMagnitude, 0.0f);
			LaunchForce = FMath::GetMappedRangeValueClamped(FalloffRange, LaunchForceRange, Distance);
		}
		// 打印击退力度
		if (bDrawDebugs)GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, FString::Printf(TEXT("LaunchForce: %f"), LaunchForce));
		
		// 计算击退方向
		FVector KnockbackForce = ToHitActor.GetSafeNormal();
		// 先清掉 Z，保证基础方向是水平击退
		KnockbackForce.Z = 0.0f;
		
		// 计算右方向，用来调整击退角度
		const FVector Right = KnockbackForce.RotateAngleAxis(90.0f, FVector::UpVector);
		
		// 根据 RotationAngle 调整击退方向，并乘以击退力度
		KnockbackForce = KnockbackForce.RotateAngleAxis(-RotationAngle, Right) * LaunchForce;
		
		// 绘制击退方向箭头
		if (bDrawDebugs)
		{
			UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor, EGetWorldErrorMode::LogAndReturnNull);
			DrawDebugDirectionalArrow(World, HitCharacterLocation, HitCharacterLocation + KnockbackForce, 100.f, FColor::Green, false, 3.f);
		}
		
		// 对角色施加击退
		HitCharacter->LaunchCharacter(KnockbackForce, true, true);
	}	
	return HitActors;
}
