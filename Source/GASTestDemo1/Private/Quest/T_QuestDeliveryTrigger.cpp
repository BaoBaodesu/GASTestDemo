#include "Quest/T_QuestDeliveryTrigger.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameObjects/T_PickUpItems.h"
#include "Quest/T_QuestGameState.h"
#include "UObject/ConstructorHelpers.h"

AT_QuestDeliveryTrigger::AT_QuestDeliveryTrigger()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DeliveryPanel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DeliveryPanel"));
	DeliveryPanel->SetupAttachment(SceneRoot);
	DeliveryPanel->SetCollisionProfileName(TEXT("BlockAll"));
	DeliveryPanel->SetRelativeScale3D(FVector(2.f, 2.f, 0.2f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PanelMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (PanelMesh.Succeeded()) DeliveryPanel->SetStaticMesh(PanelMesh.Object);

	DeliveryTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("DeliveryTrigger"));
	DeliveryTrigger->SetupAttachment(SceneRoot);
	DeliveryTrigger->SetRelativeLocation(FVector(0.f, 0.f, 35.f));
	DeliveryTrigger->SetBoxExtent(FVector(100.f, 100.f, 35.f));
	DeliveryTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DeliveryTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	DeliveryTrigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	DeliveryTrigger->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	DeliveryTrigger->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FClassFinder<AT_PickUpItems> RequiredPickupAsset(
		TEXT("/Game/GASTestDemo/GameObjects/PickUp/BP_PickFumo.BP_PickFumo_C"));
	RequiredPickupClass = RequiredPickupAsset.Class;
}

void AT_QuestDeliveryTrigger::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		DeliveryTrigger->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleTriggerBeginOverlap);
	}
}

void AT_QuestDeliveryTrigger::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bDelivered || !HasAuthority() || !IsValid(OtherActor)) return;

	AT_PickUpItems* Pickup = Cast<AT_PickUpItems>(OtherActor);
	AT_QuestGameState* QuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr;
	if (!IsValid(Pickup) || !IsValid(QuestGameState) || QuestGameState->GetQuestOutcome() != EQuestOutcome::InProgress || !QuestGameState->IsRequiredPickup(Pickup)) return;

	bDelivered = true;
	DeliveryTrigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	QuestGameState->CompleteMainQuest();
}
