#include "GameObjects/T_PlayerProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "Characters/T_GuardCharacter.h"
#include "Characters/T_EnemyCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "Kismet/GameplayStatics.h"
#include "Utils/T_BlueprintLibrary.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Quest/T_QuestGameState.h"

AT_PlayerProjectile::AT_PlayerProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);
	InitialLifeSpan = 5.f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	SetRootComponent(CollisionComponent);
	CollisionComponent->InitSphereRadius(5.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Camera, ECR_Overlap);
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ThisClass::OnProjectileHit);

	TrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailComponent"));
	TrailComponent->SetupAttachment(CollisionComponent);
	TrailComponent->SetAutoActivate(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 12000.f;
	ProjectileMovement->MaxSpeed = 12000.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
}

void AT_PlayerProjectile::InitializeProjectile(TSubclassOf<UGameplayEffect> InDamageEffectClass,
	float InDamage, bool bInHeadshot, UNiagaraSystem* InTrailSystem, UNiagaraSystem* InImpactSystem,
	USoundBase* InImpactSound, USoundAttenuation* InImpactSoundAttenuation, AActor* WeaponActor,
	bool bInForbiddenPistolShot)
{
	DamageEffectClass = InDamageEffectClass;
	Damage = InDamage;
	bHeadshot = bInHeadshot;
	bForbiddenPistolShot = bInForbiddenPistolShot;
	TrailSystem = InTrailSystem;
	ImpactSystem = InImpactSystem;
	ImpactSound = InImpactSound;
	ImpactSoundAttenuation = InImpactSoundAttenuation;
	OnRep_TrailSystem();

	if (IsValid(GetOwner())) CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
	if (IsValid(GetInstigator())) CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	if (IsValid(WeaponActor)) CollisionComponent->IgnoreActorWhenMoving(WeaponActor, true);
}

void AT_PlayerProjectile::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!IsValid(World)) return;

	// The player may already be inside an invincibility window when this projectile spawns
	for (TActorIterator<AT_PlayerCharacter> PlayerIt(World); PlayerIt; ++PlayerIt)
	{
		if (UT_BlueprintLibrary::IsInvincible(*PlayerIt))
		{
			SetMoveIgnoredActor(*PlayerIt, true);
		}
	}
}

void AT_PlayerProjectile::SetMoveIgnoredActor(AActor* Actor, bool bIgnore)
{
	if (!IsValid(Actor) || !IsValid(CollisionComponent)) return;
	CollisionComponent->IgnoreActorWhenMoving(Actor, bIgnore);
}

void AT_PlayerProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AT_PlayerProjectile, TrailSystem);
	DOREPLIFETIME(AT_PlayerProjectile, ImpactSystem);
	DOREPLIFETIME(AT_PlayerProjectile, ImpactSound);
	DOREPLIFETIME(AT_PlayerProjectile, ImpactSoundAttenuation);
}

void AT_PlayerProjectile::OnRep_TrailSystem()
{
	if (!IsValid(TrailComponent) || !IsValid(TrailSystem)) return;
	TrailComponent->SetAsset(TrailSystem);
	TrailComponent->Activate(true);
}

void AT_PlayerProjectile::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bImpactHandled) return;
	if (IsValid(OtherActor) && (OtherActor == GetOwner() || OtherActor == GetInstigator())) return;
	// Fallback when the move-ignore list has not taken effect yet: no damage and no impact FX.
	// The blocking hit already stopped ProjectileMovement, so destroy it instead of leaving it frozen.
	if (IsValid(OtherActor) && OtherActor->IsA<AT_PlayerCharacter>() && UT_BlueprintLibrary::IsInvincible(OtherActor))
	{
		Destroy();
		return;
	}
	bImpactHandled = true;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	MulticastPlayImpactEffects(Hit.ImpactPoint, Hit.ImpactNormal);
	AActor* NoiseInstigator = GetInstigator() ? GetInstigator() : GetOwner();
	if (IsValid(NoiseInstigator) && NoiseInstigator->IsA<AT_PlayerCharacter>())
	{
		UAISense_Hearing::ReportNoiseEvent(
			this,
			Hit.ImpactPoint,
			0.5f,
			NoiseInstigator,
			800.f,
			TEXT("GuardNoise.BulletImpact"));
	}
	// BSP/盒体笔刷的碰撞可能只有 UModelComponent，没有可用的 Actor；命中特效播放后直接结束。
	if (!IsValid(OtherActor))
	{
		Destroy();
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	const UT_AttributeSet* TargetAttributes = IsValid(TargetASC) ? Cast<const UT_AttributeSet>(TargetASC->GetAttributeSet(UT_AttributeSet::StaticClass())) : nullptr;
	const float HealthBefore = IsValid(TargetAttributes) ? TargetAttributes->GetHealth() : 0.f;

	bool bHitConfirmed = false;
	float DamageMagnitude = FMath::Abs(Damage);
	if (bHeadshot && HeadshotDamageMultiplier > 0.f)
	{
		DamageMagnitude *= HeadshotDamageMultiplier;
	}
	if (IsValid(SourceASC) && IsValid(TargetASC) && IsValid(DamageEffectClass) && DamageMagnitude > 0.f)
	{
		AT_PlayerCharacter* HitPlayer = Cast<AT_PlayerCharacter>(OtherActor);
		if (IsValid(HitPlayer) && !HitPlayer->IsAlive())
		{
			Destroy();
			return;
		}

		const bool bLethal = IsValid(TargetAttributes) && (HealthBefore - DamageMagnitude <= 0.f);
		FGameplayEventData EventPayload;
		EventPayload.Instigator = GetInstigator() ? GetInstigator() : GetOwner();
		EventPayload.Target = OtherActor;
		EventPayload.ContextHandle = TargetASC->MakeEffectContext();
		EventPayload.ContextHandle.AddHitResult(Hit, true);

		if (IsValid(HitPlayer))
		{
			const FGameplayTag EventTag = bLethal ? TTags::Events::Player::Death : TTags::Events::Player::HitReact;
			TargetASC->HandleGameplayEvent(EventTag, &EventPayload);
		}
		else if (AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(OtherActor))
		{
			if (!bLethal)
			{
				// HitReact BP (ActivateOnGiven+Wait) is skipped by ASC; play presentation in C++
				TargetASC->HandleGameplayEvent(TTags::Events::Enemy::HitReact, &EventPayload);
				Guard->PlayHitReactPresentation(GetInstigator() ? GetInstigator() : GetOwner());
			}
		}
		else if (OtherActor->IsA<AT_EnemyCharacter>() && !bLethal)
		{
			TargetASC->HandleGameplayEvent(TTags::Events::Enemy::HitReact, &EventPayload);
		}

		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		ContextHandle.AddHitResult(Hit, true);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(TTags::SetByCaller::Projectile, -DamageMagnitude);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			const float HealthAfter = IsValid(TargetAttributes) ? TargetAttributes->GetHealth() : HealthBefore;
			bHitConfirmed = IsValid(TargetAttributes) && HealthAfter < HealthBefore;
			if (bForbiddenPistolShot && HealthBefore > 0.f && HealthAfter <= 0.f && OtherActor->IsA<AT_EnemyCharacter>())
			{
				if (AT_QuestGameState* QuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr)
				{
					QuestGameState->NotifyForbiddenPistolKill();
				}
			}
		}
	}

	if (bHitConfirmed && OtherActor->IsA<APawn>())
	{
		if (AT_GuardCharacter* Guard = Cast<AT_GuardCharacter>(OtherActor))
		{
			UAISense_Damage::ReportDamageEvent(
				this,
				Guard,
				GetInstigator() ? GetInstigator() : GetOwner(),
				DamageMagnitude,
				Hit.ImpactPoint,
				Hit.ImpactPoint);
		}
		if (AT_PlayerCharacter* Shooter = Cast<AT_PlayerCharacter>(GetOwner()))
		{
			Shooter->ClientNotifyHitConfirmed();
		}
	}

	Destroy();
}

void AT_PlayerProjectile::MulticastPlayImpactEffects_Implementation(FVector ImpactPoint, FVector ImpactNormal)
{
	if (IsValid(TrailComponent)) TrailComponent->DeactivateImmediate();
	if (IsValid(ImpactSystem)) UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactSystem, ImpactPoint, ImpactNormal.Rotation(), FVector::OneVector, true, true, ENCPoolMethod::AutoRelease, true);
	if (IsValid(ImpactSound)) UGameplayStatics::SpawnSoundAtLocation(this, ImpactSound, ImpactPoint, FRotator::ZeroRotator, 1.f, 1.f, 0.f, ImpactSoundAttenuation);
}
