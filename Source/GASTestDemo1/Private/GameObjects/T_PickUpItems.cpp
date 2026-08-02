
#include "GameObjects/T_PickUpItems.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"

AT_PickUpItems::AT_PickUpItems()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(SceneRoot);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	StaticItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticItemMesh"));
	StaticItemMesh->SetupAttachment(SceneRoot);
	StaticItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SkeletalItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalItemMesh"));
	SkeletalItemMesh->SetupAttachment(SceneRoot);
	SkeletalItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalItemMesh->SetVisibility(false);
}

void AT_PickUpItems::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshComponents();
}

bool AT_PickUpItems::CanBePickedUp(AActor* Picker) const
{
	return !bPickedUp && IsValid(Picker) && ItemData.IsValid();
}

bool AT_PickUpItems::PickUp(AActor* Picker)
{
	if (!HasAuthority() || !CanBePickedUp(Picker)) return false;

	bPickedUp = true;

	const FTPickUpItemData PickedItemData = ItemData;

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);

	OnPickedUp.Broadcast(Picker, PickedItemData);
	BP_OnPickedUp(Picker, PickedItemData);

	Destroy();

	return true;
}

void AT_PickUpItems::RefreshComponents()
{
	if (!IsValid(InteractionSphere) || !IsValid(StaticItemMesh) || !IsValid(SkeletalItemMesh)) return;

	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(InteractionTraceChannel, ECR_Block);

	const bool bHasSkeletalMesh = IsValid(SkeletalItemMesh->GetSkeletalMeshAsset());
	const bool bHasStaticMesh = IsValid(StaticItemMesh->GetStaticMesh());

	SkeletalItemMesh->SetVisibility(bHasSkeletalMesh);
	StaticItemMesh->SetVisibility(!bHasSkeletalMesh && bHasStaticMesh);
}
