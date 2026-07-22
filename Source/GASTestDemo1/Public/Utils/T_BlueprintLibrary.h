// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "T_BlueprintLibrary.generated.h"

struct FGameplayTag;
class UGameplayEffect;
class APawn;

UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Left,
	Right,
	Forward,
	Back
};

UENUM(BlueprintType)
enum class ERollDirection : uint8
{
	Forward,
	ForwardRight,
	ForwardLeft,
	Right,
	BackRight,
	Back,
	BackLeft,
	Left,
	None
};


USTRUCT(BlueprintType)
struct FClosestActorWithTagResult
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(BlueprintReadWrite)
	float Distance{0.f};
};

UCLASS()
class GASTESTDEMO1_API UT_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	// 判断攻击来自目标的哪个方向
	UFUNCTION(BlueprintPure)
	static EHitDirection GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator);

	// 将受击方向枚举转换为名字
	UFUNCTION(BlueprintPure)
	static FName GetHitDirectionName(const EHitDirection& HitDirection);
	
	// 根据角色本地空间中的移动输入判断翻滚方向
	UFUNCTION(BlueprintPure)
	static ERollDirection GetRollDirectionFromInput(const APawn* Pawn, const FVector2D& MovementInput);

	// 将翻滚方向枚举转换为 Montage Section Name
	UFUNCTION(BlueprintPure)
	static FName GetRollDirectionName(const ERollDirection& RollDirection);
	
	// 查找距离 Origin 最近的、带有指定 Tag 的 Actor
	UFUNCTION(BlueprintCallable)
	static FClosestActorWithTagResult FindClosestActorWithTag(UObject* WorldContextObject, const FVector& Origin, const FName& Tag, float SearchRange);
	
	// 给玩家发送伤害事件，并传入伤害效果、事件数据、Tag 和伤害数值
	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayer(AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect, UPARAM(ref) FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag &EventTagOverride, UObject* OptionalParticleSystem = nullptr);
	
	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayers(TArray<AActor*> Targets, const TSubclassOf<UGameplayEffect>& DamageEffect, UPARAM(ref) FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem = nullptr);
	
	// 在角色前方生成一个球形检测范围，返回检测到的角色
	UFUNCTION(BlueprintCallable, Category = "Crash|Abilities")
	static TArray<AActor*> HitBoxOverlapTest(AActor* AvatarActor, float HitBoxRadius, float HitBoxForwardOffset = 0.f, float HitBoxElevationOffset = 0.f, bool bDrawDebugs = false);

	// 绘制 HitBoxOverlapTest 的调试球体和命中目标位置
	static void DrawHitBoxOverlapDebugs(const UObject* WorldContextObject, const TArray<FOverlapResult>& OverlapResults, const FVector& HitBoxLocation, float HitBoxRadius);

	UFUNCTION(BlueprintCallable, Category = "Crash|Abilities")
	static TArray<AActor*> ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors, float InnerRadius, float OuterRadius, float LaunchForceMagnitude, float RotationAngle = 45.f, bool bDrawDebugs = false);
};
