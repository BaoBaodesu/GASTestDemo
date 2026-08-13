
#include "GameObjects/T_PickUpItems.h"

#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/CollisionProfile.h"
#include "Inventory/T_ItemDefinition.h"
#include "PhysicsEngine/BodyInstance.h"


AT_PickUpItems::AT_PickUpItems()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	SetReplicateMovement(true);

	PhysicsRoot = CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsRoot"));
	SetRootComponent(PhysicsRoot);
	PhysicsRoot->InitBoxExtent(PhysicsBoxExtent);
	PhysicsRoot->SetMobility(EComponentMobility::Movable);
	PhysicsRoot->SetCollisionProfileName(UCollisionProfile::PhysicsActor_ProfileName);
	PhysicsRoot->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PhysicsRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	PhysicsRoot->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	PhysicsRoot->SetSimulatePhysics(true);
	PhysicsRoot->SetEnableGravity(true);
	PhysicsRoot->SetLinearDamping(4.f);
	PhysicsRoot->SetAngularDamping(12.f);
	// 任务交付面板通过重叠识别世界中的实体掉落物；该组件没有重叠回调，不会改变拾取逻辑。
	PhysicsRoot->SetGenerateOverlapEvents(true);
	PhysicsRoot->SetCanEverAffectNavigation(false);
	PhysicsRoot->BodyInstance.bLockXRotation = true;
	PhysicsRoot->BodyInstance.bLockYRotation = true;
	PhysicsRoot->BodyInstance.bLockZRotation = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(PhysicsRoot);
	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(InteractionTraceChannel, ECR_Block);
	InteractionSphere->SetGenerateOverlapEvents(false);
	InteractionSphere->SetCanEverAffectNavigation(false);

	StaticItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticItemMesh"));
	StaticItemMesh->SetupAttachment(PhysicsRoot);
	StaticItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticItemMesh->SetSimulatePhysics(false);
	StaticItemMesh->SetCanEverAffectNavigation(false);

	SkeletalItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalItemMesh"));
	SkeletalItemMesh->SetupAttachment(PhysicsRoot);
	SkeletalItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalItemMesh->SetSimulatePhysics(false);
	SkeletalItemMesh->SetCanEverAffectNavigation(false);
	SkeletalItemMesh->SetVisibility(false);
}

void AT_PickUpItems::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshComponents();
}

void AT_PickUpItems::BeginPlay()
{
	Super::BeginPlay();
	ApplyDefaultQuantityFromDefinition();
	SetFocused(false);
}

bool AT_PickUpItems::CanBePickedUp(AActor* Picker) const
{
	return !bPickedUp && IsValid(Picker) && ItemData.IsValid();
}

bool AT_PickUpItems::IsPhysicalCollisionComponent(const UPrimitiveComponent* Component) const
{
	return Component == PhysicsRoot;
}

void AT_PickUpItems::SetFocused(bool bFocused)
{
	TArray<UWidgetComponent*> WidgetComponents;
	GetComponents(WidgetComponents);
	for (UWidgetComponent* WidgetComponent : WidgetComponents)
	{
		if (!IsValid(WidgetComponent)) continue;
		WidgetComponent->SetVisibility(bFocused, true);
		WidgetComponent->SetHiddenInGame(!bFocused);
	}
}

bool AT_PickUpItems::PickUp(AActor* Picker)
{
	if (!HasAuthority() || !CanBePickedUp(Picker)) return false;
	return ConsumeQuantity(Picker, ItemData.Quantity);
}

void AT_PickUpItems::SetItemDefinition(UT_ItemDefinition* InItemDefinition)
{
	ItemData.ItemDefinition = InItemDefinition;
	ApplyDefaultQuantityFromDefinition();
	RefreshComponents();
}

void AT_PickUpItems::SetQuantity(int32 InQuantity)
{
	ItemData.Quantity = FMath::Max(1, InQuantity);
}

void AT_PickUpItems::ApplyDefaultQuantityFromDefinition()
{
	if (!IsValid(ItemData.ItemDefinition)) return;
	ItemData.Quantity = FMath::Max(1, ItemData.ItemDefinition->DefaultQuantity);
}

bool AT_PickUpItems::ConsumeQuantity(AActor* Picker, int32 ConsumedQuantity)
{
	if (!CanBePickedUp(Picker) || ConsumedQuantity <= 0 || ConsumedQuantity > ItemData.Quantity) return false;

	FTPickUpItemData PickedItemData = ItemData;
	PickedItemData.Quantity = ConsumedQuantity;
	ItemData.Quantity -= ConsumedQuantity;

	OnPickedUp.Broadcast(Picker, PickedItemData);
	BP_OnPickedUp(Picker, PickedItemData);

	if (ItemData.Quantity > 0) return true;

	bPickedUp = true;

	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	Destroy();
	return true;
}

void AT_PickUpItems::RefreshComponents()
{
	if (!IsValid(PhysicsRoot) || !IsValid(InteractionSphere) || !IsValid(StaticItemMesh) || !IsValid(SkeletalItemMesh)) return;

	PhysicsRoot->SetBoxExtent(PhysicsBoxExtent);
	PhysicsRoot->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	PhysicsRoot->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	PhysicsRoot->SetCollisionResponseToChannel(InteractionTraceChannel, ECR_Block);

	InteractionSphere->SetSphereRadius(InteractionRadius);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(InteractionTraceChannel, ECR_Block);

	StaticItemMesh->SetStaticMesh(IsValid(ItemData.ItemDefinition) ? ItemData.ItemDefinition->StaticMesh : nullptr);
	SkeletalItemMesh->SetSkeletalMesh(IsValid(ItemData.ItemDefinition) ? ItemData.ItemDefinition->SkeletalMesh : nullptr);

	const bool bHasStaticMesh = IsValid(StaticItemMesh->GetStaticMesh());
	const bool bHasSkeletalMesh = !bHasStaticMesh && IsValid(SkeletalItemMesh->GetSkeletalMeshAsset());

	StaticItemMesh->SetVisibility(bHasStaticMesh);
	SkeletalItemMesh->SetVisibility(bHasSkeletalMesh);
}
