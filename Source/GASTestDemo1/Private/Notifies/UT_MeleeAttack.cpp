// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/UT_MeleeAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "DrawDebugHelpers.h"
#include "KismetTraceUtils.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameplayTags/TTags.h"
#include "Kismet/KismetMathLibrary.h"


void UT_MeleeAttack::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime,
                                const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	
	if (!IsValid(MeshComp)) return;
	if (!IsValid(MeshComp->GetOwner())) return;
	
	// 执行球形检测
	TArray<FHitResult> Hits = PerformSphereTrace(MeshComp);
	
	// 把检测到的目标发送给拥有者 Actor
	SendEventsToActors(Hits, MeshComp);
}

TArray<FHitResult> UT_MeleeAttack::PerformSphereTrace(USkeletalMeshComponent* MeshComp) const
{
	TArray<FHitResult> OutHits;
	
	const FTransform SocketTransform = MeshComp->GetSocketTransform(SocketName);
	const FVector Start = SocketTransform.GetLocation();
	// 根据 Socket 的朝向，计算向前延伸的方向和距离
	const FVector ExtendedSocketDirection = UKismetMathLibrary::GetForwardVector(SocketTransform.GetRotation().Rotator()) * SocketExtensionOffset;
	// Trace 终点：从 Start 按 Socket 方向偏移
	const FVector End = Start - ExtendedSocketDirection;
	
	FCollisionQueryParams Params;
	// 忽略自身，避免打到自己
	Params.AddIgnoredActor(MeshComp->GetOwner());
	
	FCollisionResponseParams ResponseParams;
	// 默认忽略所有碰撞通道
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	// 只检测 Pawn 通道
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn, ECR_Block);
	
	UWorld* World = GEngine->GetWorldFromContextObject(MeshComp, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return OutHits;
	
	// 执行球形 Sweep 检测
	bool const bHit = World->SweepMultiByChannel(
		OutHits,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(SphereTraceRadius),
		Params,
		ResponseParams);

	if (bDrawDebug)
	{
		DrawDebugSphereTraceMulti(
			World,
			Start,
			End,
			SphereTraceRadius,
			EDrawDebugTrace::ForDuration,
			bHit,
			OutHits,
			FColor::Green,
			FColor::Red,
			5.f);
	}

	return OutHits;
}

void UT_MeleeAttack::SendEventsToActors(const TArray<FHitResult>& Hits, USkeletalMeshComponent* MeshComp) const
{
	for (const FHitResult& Hit : Hits)
	{
		// 只处理玩家角色
		AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(Hit.GetActor());
		if (!IsValid(PlayerCharacter)) continue;
		if (!PlayerCharacter->IsAlive()) continue;
		
		// 获取玩家身上的 AbilitySystemComponent
		UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent();
		if (!IsValid(ASC)) continue;

		FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
		ContextHandle.AddHitResult(Hit);

		// 创建 GameplayEvent 需要携带的数据
		FGameplayEventData Payload;
		Payload.Target = PlayerCharacter;
		Payload.ContextHandle = ContextHandle;
		Payload.Instigator = MeshComp->GetOwner();
		
		// 给攻击者发送近战命中事件
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), TTags::Events::Enemy::MeleeTraceHit, Payload);
	}
}
