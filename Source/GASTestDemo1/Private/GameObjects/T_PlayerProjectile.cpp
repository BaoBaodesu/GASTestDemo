#include "GameObjects/T_PlayerProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

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
	float InDamage, UNiagaraSystem* InTrailSystem, UNiagaraSystem* InImpactSystem,
	USoundBase* InImpactSound, USoundAttenuation* InImpactSoundAttenuation, AActor* WeaponActor)
{
	DamageEffectClass = InDamageEffectClass;
	Damage = InDamage;
	TrailSystem = InTrailSystem;
	ImpactSystem = InImpactSystem;
	ImpactSound = InImpactSound;
	ImpactSoundAttenuation = InImpactSoundAttenuation;
	OnRep_TrailSystem();

	if (IsValid(GetOwner())) CollisionComponent->IgnoreActorWhenMoving(GetOwner(), true);
	if (IsValid(GetInstigator())) CollisionComponent->IgnoreActorWhenMoving(GetInstigator(), true);
	if (IsValid(WeaponActor)) CollisionComponent->IgnoreActorWhenMoving(WeaponActor, true);
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
	if (!HasAuthority() || bImpactHandled || !IsValid(OtherActor) || OtherActor == GetOwner() || OtherActor == GetInstigator()) return;
	bImpactHandled = true;
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileMovement->StopMovementImmediately();
	MulticastPlayImpactEffects(Hit.ImpactPoint, Hit.ImpactNormal);

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (IsValid(SourceASC) && IsValid(TargetASC) && IsValid(DamageEffectClass) && Damage > 0.f)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		ContextHandle.AddHitResult(Hit, true);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(TTags::SetByCaller::Projectile, Damage);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
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
