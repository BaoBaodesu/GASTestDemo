// Fill out your copyright notice in the Description page of Project Settings.


#include "GameObjects/T_Projectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayTags/TTags.h"
#include "Utils/T_BlueprintLibrary.h"


// Sets default values
AT_Projectile::AT_Projectile()
{

	PrimaryActorTick.bCanEverTick = false;
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	
	bReplicates = true;
}

void AT_Projectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(OtherActor);
	if (!IsValid(PlayerCharacter)) return;
	if (!PlayerCharacter->IsAlive()) return;
	UAbilitySystemComponent* AbilitySystemComponent = PlayerCharacter->GetAbilitySystemComponent();

	if (!IsValid(AbilitySystemComponent) || !HasAuthority()) return;
	 
	FGameplayEventData Payload;
	Payload.Instigator = GetOwner();
	Payload.Target = PlayerCharacter;
	
	UT_BlueprintLibrary::SendDamageEventToPlayer(PlayerCharacter, DamageEffect, Payload, TTags::SetByCaller::Projectile, Damage);

	UE_LOG(LogTemp, Log, TEXT("Damage: %f"), Damage);

	SpawnImpactEffects();
	
	Destroy();
}

