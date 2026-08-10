// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/T_ShooterAIController.h"

#include "AI/BehaviorTree/T_BTDecorator_GuardAlive.h"
#include "AI/BehaviorTree/T_BTDecorator_GuardBlackboard.h"
#include "AI/BehaviorTree/T_BTDecorator_GuardHasAmmo.h"
#include "AI/BehaviorTree/T_BTService_GuardFaceTarget.h"
#include "AI/BehaviorTree/T_BTService_GuardPatrol.h"
#include "AI/BehaviorTree/T_BTService_GuardUpdateSight.h"
#include "AI/BehaviorTree/T_BTTask_ActivateAbilityByTag.h"
#include "AI/BehaviorTree/T_BTTask_GuardAlertWait.h"
#include "AI/BehaviorTree/T_BTTask_GuardClearBlackboard.h"
#include "AI/BehaviorTree/T_BTTask_GuardMoveTo.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BrainComponent.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_GuardCharacter.h"
#include "GameplayTags/TTags.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Sight.h"

namespace GuardBBKeys
{
	const FName Enemy = TEXT("Enemy");
	const FName EnemySpotted = TEXT("Enemy Spotted");
	const FName MoveLocation = TEXT("Move Location");
	const FName NoiseLocation = TEXT("Noise Location");
}

namespace
{
	template <typename NodeType>
	NodeType* NewBTNode(UObject* Outer, const FName& Name)
	{
		return NewObject<NodeType>(Outer, Name, RF_Transient);
	}

	FBTCompositeChild& AddCompositeChild(UBTCompositeNode* Parent, UBTCompositeNode* ChildComposite)
	{
		FBTCompositeChild& Child = Parent->Children.AddDefaulted_GetRef();
		Child.ChildComposite = ChildComposite;
		Child.ChildTask = nullptr;
		return Child;
	}

	FBTCompositeChild& AddTaskChild(UBTCompositeNode* Parent, UBTTaskNode* ChildTask)
	{
		FBTCompositeChild& Child = Parent->Children.AddDefaulted_GetRef();
		Child.ChildComposite = nullptr;
		Child.ChildTask = ChildTask;
		return Child;
	}
}

AT_ShooterAIController::AT_ShooterAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));
	SetPerceptionComponent(*PerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = LoseSightRadius;
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightConfig->SetMaxAge(SightMaxAge);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComponent->ConfigureSense(*SightConfig);

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = HearingRange;
	HearingConfig->SetMaxAge(HearingMaxAge);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	PerceptionComponent->ConfigureSense(*HearingConfig);

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(DamageMaxAge);
	PerceptionComponent->ConfigureSense(*DamageConfig);

	PerceptionComponent->SetDominantSense(UAISense_Sight::StaticClass());
}

void AT_ShooterAIController::ConfigurePerceptionSenses()
{
	if (!IsValid(PerceptionComponent)) return;

	if (IsValid(SightConfig))
	{
		SightConfig->SightRadius = SightRadius;
		SightConfig->LoseSightRadius = LoseSightRadius;
		SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
		SightConfig->SetMaxAge(SightMaxAge);
		PerceptionComponent->ConfigureSense(*SightConfig);
	}

	if (IsValid(HearingConfig))
	{
		HearingConfig->HearingRange = HearingRange;
		HearingConfig->SetMaxAge(HearingMaxAge);
		PerceptionComponent->ConfigureSense(*HearingConfig);
	}

	if (IsValid(DamageConfig))
	{
		DamageConfig->SetMaxAge(DamageMaxAge);
		PerceptionComponent->ConfigureSense(*DamageConfig);
	}
}

void AT_ShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ConfigurePerceptionSenses();

	if (IsValid(PerceptionComponent))
	{
		PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
		PerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}

	StartGuardBehaviorTree();

	if (CurrentTarget.IsValid())
	{
		SetTarget(CurrentTarget.Get());
	}
}

void AT_ShooterAIController::OnUnPossess()
{
	if (IsValid(PerceptionComponent))
	{
		PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}
	PerceivedTargets.Reset();
	CurrentTarget = nullptr;

	Super::OnUnPossess();
}

void AT_ShooterAIController::BeginPlay()
{
	Super::BeginPlay();
}

AT_GuardCharacter* AT_ShooterAIController::GetGuardCharacter() const
{
	return Cast<AT_GuardCharacter>(GetPawn());
}

void AT_ShooterAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!IsValid(Actor)) return;

	// 噪声只写 Noise Location，不直接视为发现敌人
	if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
	{
		if (Stimulus.WasSuccessfullySensed())
		{
			UBlackboardComponent* BB = GetBlackboardComponent();
			if (IsValid(BB)) BB->SetValueAsVector(GuardBBKeys::NoiseLocation, Stimulus.StimulusLocation);
		}
		return;
	}

	if (!Stimulus.WasSuccessfullySensed())
	{
		// 只有视觉丢失才清除目标；听觉刺激过期不影响当前目标
		if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
		{
			RemovePerceivedTarget(Actor);
			if (Actor == CurrentTarget.Get())
			{
				UBlackboardComponent* BB = GetBlackboardComponent();
				if (IsValid(BB))
				{
					BB->SetValueAsBool(GuardBBKeys::EnemySpotted, false);
					BB->SetValueAsVector(GuardBBKeys::MoveLocation, LastKnownTargetLocation);
				}
			}
			SelectAndSetTarget();
		}
		return;
	}

	// 伤害刺激汇报的 Actor 是伤害来源（玩家）
	UpdatePerceivedTarget(Actor, Stimulus.StimulusLocation);
	SelectAndSetTarget();
}

void AT_ShooterAIController::UpdatePerceivedTarget(AActor* Actor, const FVector& Location)
{
	for (FGuardPerceivedTarget& Entry : PerceivedTargets)
	{
		if (Entry.Actor == Actor)
		{
			Entry.Location = Location;
			return;
		}
	}

	FGuardPerceivedTarget NewEntry;
	NewEntry.Actor = Actor;
	NewEntry.Location = Location;
	PerceivedTargets.Add(NewEntry);
}

void AT_ShooterAIController::RemovePerceivedTarget(AActor* Actor)
{
	PerceivedTargets.RemoveAll([Actor](const FGuardPerceivedTarget& Entry)
	{
		return !Entry.Actor.IsValid() || Entry.Actor == Actor;
	});
}

void AT_ShooterAIController::SelectAndSetTarget()
{
	const FVector Origin = IsValid(GetPawn()) ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	AActor* Selected = SelectNearestValidTarget(PerceivedTargets, Origin);
	SetTarget(Selected);
}

void AT_ShooterAIController::SetTarget(AActor* Target)
{
	CurrentTarget = Target;

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!IsValid(BB)) return;

	if (IsValid(Target))
	{
		LastKnownTargetLocation = Target->GetActorLocation();
		BB->SetValueAsObject(GuardBBKeys::Enemy, Target);
		BB->SetValueAsBool(GuardBBKeys::EnemySpotted, true);
		BB->SetValueAsVector(GuardBBKeys::MoveLocation, LastKnownTargetLocation);
	}
	else
	{
		BB->SetValueAsObject(GuardBBKeys::Enemy, nullptr);
		BB->SetValueAsBool(GuardBBKeys::EnemySpotted, false);
	}
}

AActor* AT_ShooterAIController::SelectNearestValidTarget(const TArray<FGuardPerceivedTarget>& Candidates, const FVector& Origin)
{
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	AActor* Best = nullptr;

	for (const FGuardPerceivedTarget& Candidate : Candidates)
	{
		AActor* Actor = Candidate.Actor.Get();
		if (!IsValid(Actor)) continue;

		const AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(Actor);
		if (!IsValid(BaseCharacter) || !BaseCharacter->IsAlive()) continue;
		if (!Actor->ActorHasTag(CrashTags::Player)) continue;

		const float DistanceSquared = FVector::DistSquared(Origin, Candidate.Location);
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			Best = Actor;
		}
	}

	return Best;
}

void AT_ShooterAIController::ClearTargetState()
{
	CurrentTarget = nullptr;
	LastKnownTargetLocation = FVector::ZeroVector;
	PerceivedTargets.Reset();

	ClearGuardBlackboard(GetBlackboardComponent());
	ClearFocus(EAIFocusPriority::Gameplay);
}

void AT_ShooterAIController::ClearGuardBlackboard(UBlackboardComponent* BlackboardComp)
{
	if (!IsValid(BlackboardComp)) return;

	BlackboardComp->ClearValue(GuardBBKeys::Enemy);
	BlackboardComp->ClearValue(GuardBBKeys::EnemySpotted);
	BlackboardComp->ClearValue(GuardBBKeys::MoveLocation);
	BlackboardComp->ClearValue(GuardBBKeys::NoiseLocation);
}

void AT_ShooterAIController::OnGuardDied()
{
	StopMovement();

	UBrainComponent* Brain = GetBrainComponent();
	if (IsValid(Brain)) Brain->StopLogic(TEXT("GuardDead"));

	if (IsValid(PerceptionComponent)) PerceptionComponent->Deactivate();

	ClearFocus(EAIFocusPriority::Gameplay);
	ClearTargetState();
}

void AT_ShooterAIController::RestartGuardAI()
{
	StopMovement();

	// 清理死亡遗留的目标状态与黑板键，重新进入巡逻/索敌
	ClearTargetState();

	// 重新激活感知（死亡时被 Deactivate）
	if (IsValid(PerceptionComponent)) PerceptionComponent->Activate(true);

	if (AT_GuardCharacter* Guard = GetGuardCharacter())
	{
		Guard->ClearStaleBlockingTags();
	}

	StartGuardBehaviorTree();

	// 复活时玩家可能一直在视野内，不会再次触发感知回调，需要主动回填
	RefreshPerceivedTargetsFromPerception();
}

void AT_ShooterAIController::RefreshPerceivedTargetsFromPerception()
{
	if (!IsValid(PerceptionComponent)) return;

	TArray<AActor*> PerceivedActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);
	for (AActor* Actor : PerceivedActors)
	{
		if (!IsValid(Actor)) continue;
		UpdatePerceivedTarget(Actor, Actor->GetActorLocation());
	}

	TArray<AActor*> DamagedByActors;
	PerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Damage::StaticClass(), DamagedByActors);
	for (AActor* Actor : DamagedByActors)
	{
		if (!IsValid(Actor)) continue;
		UpdatePerceivedTarget(Actor, Actor->GetActorLocation());
	}

	SelectAndSetTarget();
}

bool AT_ShooterAIController::StartGuardBehaviorTree()
{
	RuntimeBehaviorTree = CreateRuntimeGuardBehaviorTree();
	UBehaviorTree* TreeToRun = IsValid(RuntimeBehaviorTree) ? RuntimeBehaviorTree.Get() : BehaviorTreeAsset.Get();
	if (!IsValid(TreeToRun))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: 无法启动 Guard 行为树（运行时树与 BehaviorTreeAsset 均无效）。"), *GetName());
		return false;
	}

	const bool bStarted = RunBehaviorTree(TreeToRun);
	BehaviorTreeComponent = Cast<UBehaviorTreeComponent>(GetBrainComponent());
	BlackboardComponent = GetBlackboardComponent();

	if (!bStarted)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: RunBehaviorTree(%s) 失败。"), *GetName(), *TreeToRun->GetName());
	}
	return bStarted;
}

UBehaviorTree* AT_ShooterAIController::CreateRuntimeGuardBehaviorTree()
{
	UBlackboardData* BlackboardData = nullptr;
	if (IsValid(BehaviorTreeAsset) && IsValid(BehaviorTreeAsset->BlackboardAsset))
	{
		BlackboardData = BehaviorTreeAsset->BlackboardAsset;
	}
	if (!IsValid(BlackboardData))
	{
		BlackboardData = LoadObject<UBlackboardData>(nullptr, TEXT("/Game/GASTestDemo/AI/ShooterController/BB_Shooter.BB_Shooter"));
	}
	if (!IsValid(BlackboardData))
	{
		UE_LOG(LogTemp, Error, TEXT("%s: 无法加载 BB_Shooter，跳过运行时行为树创建。"), *GetName());
		return nullptr;
	}

	UBehaviorTree* Tree = NewObject<UBehaviorTree>(
		this,
		MakeUniqueObjectName(this, UBehaviorTree::StaticClass(), TEXT("RuntimeGuardBT")));
	Tree->BlackboardAsset = BlackboardData;

	UBTComposite_Selector* Root = NewBTNode<UBTComposite_Selector>(Tree, TEXT("Root"));
	Tree->RootNode = Root;

	// ----- Combat -----
	UBTComposite_Sequence* Combat = NewBTNode<UBTComposite_Sequence>(Tree, TEXT("Combat"));
	FBTCompositeChild& CombatChild = AddCompositeChild(Root, Combat);
	{
		UT_BTDecorator_GuardAlive* Alive = NewBTNode<UT_BTDecorator_GuardAlive>(Tree, TEXT("CombatAlive"));
		CombatChild.Decorators.Add(Alive);

		UT_BTDecorator_GuardBlackboard* Spotted = NewBTNode<UT_BTDecorator_GuardBlackboard>(Tree, TEXT("CombatEnemySpotted"));
		Spotted->SetBlackboardKeyName(GuardBBKeys::EnemySpotted);
		Spotted->Condition = EGuardBlackboardCondition::IsTrue;
		Spotted->SetFlowAbortMode(EBTFlowAbortMode::LowerPriority);
		CombatChild.Decorators.Add(Spotted);
	}

	UT_BTService_GuardUpdateSight* UpdateSight = NewBTNode<UT_BTService_GuardUpdateSight>(Tree, TEXT("UpdateSight"));
	Combat->Services.Add(UpdateSight);
	UT_BTService_GuardFaceTarget* FaceTarget = NewBTNode<UT_BTService_GuardFaceTarget>(Tree, TEXT("FaceTarget"));
	Combat->Services.Add(FaceTarget);

	UT_BTTask_ActivateAbilityByTag* AimTask = NewBTNode<UT_BTTask_ActivateAbilityByTag>(Tree, TEXT("Aim"));
	AimTask->AbilityTag = TTags::TAbilities::Enemy::Aim.GetTag();
	AimTask->bWaitForCompletion = false;
	AddTaskChild(Combat, AimTask);

	UBTComposite_Selector* AttackOrReload = NewBTNode<UBTComposite_Selector>(Tree, TEXT("AttackOrReload"));
	AddCompositeChild(Combat, AttackOrReload);

	UT_BTTask_ActivateAbilityByTag* ShootTask = NewBTNode<UT_BTTask_ActivateAbilityByTag>(Tree, TEXT("Shoot"));
	ShootTask->AbilityTag = TTags::TAbilities::Enemy::Shoot.GetTag();
	ShootTask->bWaitForCompletion = true;
	FBTCompositeChild& ShootChild = AddTaskChild(AttackOrReload, ShootTask);
	{
		UT_BTDecorator_GuardHasAmmo* HasAmmo = NewBTNode<UT_BTDecorator_GuardHasAmmo>(Tree, TEXT("HasLoadedAmmo"));
		HasAmmo->AmmoCheck = EGuardAmmoCheck::HasLoadedAmmo;
		ShootChild.Decorators.Add(HasAmmo);
	}

	UT_BTTask_ActivateAbilityByTag* ReloadTask = NewBTNode<UT_BTTask_ActivateAbilityByTag>(Tree, TEXT("Reload"));
	ReloadTask->AbilityTag = TTags::TAbilities::Enemy::Reload.GetTag();
	ReloadTask->bWaitForCompletion = true;
	FBTCompositeChild& ReloadChild = AddTaskChild(AttackOrReload, ReloadTask);
	{
		UT_BTDecorator_GuardHasAmmo* NeedsReload = NewBTNode<UT_BTDecorator_GuardHasAmmo>(Tree, TEXT("NeedsReload"));
		NeedsReload->AmmoCheck = EGuardAmmoCheck::MagazineEmpty;
		ReloadChild.Decorators.Add(NeedsReload);

		UT_BTDecorator_GuardHasAmmo* HasReserve = NewBTNode<UT_BTDecorator_GuardHasAmmo>(Tree, TEXT("HasReserveAmmo"));
		HasReserve->AmmoCheck = EGuardAmmoCheck::HasReserveAmmo;
		ReloadChild.Decorators.Add(HasReserve);
	}

	// ----- Investigate last known location -----
	UBTComposite_Sequence* Investigate = NewBTNode<UBTComposite_Sequence>(Tree, TEXT("Investigate"));
	FBTCompositeChild& InvestigateChild = AddCompositeChild(Root, Investigate);
	{
		UT_BTDecorator_GuardAlive* Alive = NewBTNode<UT_BTDecorator_GuardAlive>(Tree, TEXT("InvestigateAlive"));
		InvestigateChild.Decorators.Add(Alive);

		UT_BTDecorator_GuardBlackboard* NotSpotted = NewBTNode<UT_BTDecorator_GuardBlackboard>(Tree, TEXT("InvestigateNotSpotted"));
		NotSpotted->SetBlackboardKeyName(GuardBBKeys::EnemySpotted);
		NotSpotted->Condition = EGuardBlackboardCondition::IsFalse;
		InvestigateChild.Decorators.Add(NotSpotted);

		UT_BTDecorator_GuardBlackboard* HasMove = NewBTNode<UT_BTDecorator_GuardBlackboard>(Tree, TEXT("InvestigateHasMove"));
		HasMove->SetBlackboardKeyName(GuardBBKeys::MoveLocation);
		HasMove->Condition = EGuardBlackboardCondition::IsSet;
		InvestigateChild.Decorators.Add(HasMove);
	}

	UT_BTTask_GuardMoveTo* InvestigateMove = NewBTNode<UT_BTTask_GuardMoveTo>(Tree, TEXT("InvestigateMove"));
	InvestigateMove->MoveToKeyName = GuardBBKeys::MoveLocation;
	AddTaskChild(Investigate, InvestigateMove);

	UT_BTTask_GuardAlertWait* AlertWait = NewBTNode<UT_BTTask_GuardAlertWait>(Tree, TEXT("AlertWait"));
	AddTaskChild(Investigate, AlertWait);

	UT_BTTask_GuardClearBlackboard* ClearBB = NewBTNode<UT_BTTask_GuardClearBlackboard>(Tree, TEXT("ClearBB"));
	AddTaskChild(Investigate, ClearBB);

	// ----- Patrol -----
	UBTComposite_Sequence* Patrol = NewBTNode<UBTComposite_Sequence>(Tree, TEXT("Patrol"));
	FBTCompositeChild& PatrolChild = AddCompositeChild(Root, Patrol);
	{
		UT_BTDecorator_GuardAlive* Alive = NewBTNode<UT_BTDecorator_GuardAlive>(Tree, TEXT("PatrolAlive"));
		PatrolChild.Decorators.Add(Alive);
	}

	UT_BTService_GuardPatrol* PatrolService = NewBTNode<UT_BTService_GuardPatrol>(Tree, TEXT("PatrolPoints"));
	PatrolService->MoveLocationKey.SelectedKeyName = GuardBBKeys::MoveLocation;
	Patrol->Services.Add(PatrolService);

	UT_BTTask_GuardMoveTo* PatrolMove = NewBTNode<UT_BTTask_GuardMoveTo>(Tree, TEXT("PatrolMove"));
	PatrolMove->MoveToKeyName = GuardBBKeys::MoveLocation;
	AddTaskChild(Patrol, PatrolMove);

	return Tree;
}
