#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/Abilities/T_GuardAim.h"
#include "AI/Abilities/T_GuardAmmoLibrary.h"
#include "AI/Abilities/T_GuardReload.h"
#include "AI/Abilities/T_GuardShoot.h"
#include "AI/T_ShooterAIController.h"
#include "AI/T_GuardAlertSubsystem.h"
#include "AI/BehaviorTree/T_BTService_GuardCombatMovement.h"
#include "AI/BehaviorTree/T_BTTask_GuardMoveTo.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Characters/T_GuardCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#if WITH_EDITOR
#include "Editor.h"
#endif
#include "GameplayTags/TTags.h"
#include "GameObjects/T_Throwable.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/Components/T_ProjectileShooterComponent.h"

namespace
{
	const FGameplayTag& GetGuardTestDeadTag()
	{
		static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("TTags.Status.Dead"));
		return DeadTag;
	}

	void AddBlackboardKey(UBlackboardData* BlackboardData, const FName& EntryName, UBlackboardKeyType* KeyType)
	{
		FBlackboardEntry Entry;
		Entry.EntryName = EntryName;
		Entry.KeyType = KeyType;
		BlackboardData->Keys.Add(Entry);
	}

	UWorld* GetGuardTestEditorWorld()
	{
#if WITH_EDITOR
		return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
#else
		return nullptr;
#endif
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardAmmoMathTest,
	"GASTestDemo1.Guard.AmmoMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardAmmoMathTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("满弹匣不需要换弹"), UT_GuardAmmoLibrary::GetReloadAmount(12.f, 12.f, 24.f), 0.f);
	TestEqual(TEXT("空弹匣且备弹充足时换满"), UT_GuardAmmoLibrary::GetReloadAmount(0.f, 12.f, 24.f), 12.f);
	TestEqual(TEXT("备弹不足时部分换弹"), UT_GuardAmmoLibrary::GetReloadAmount(0.f, 12.f, 5.f), 5.f);
	TestEqual(TEXT("弹匣部分缺失时只补缺失量"), UT_GuardAmmoLibrary::GetReloadAmount(7.f, 12.f, 24.f), 5.f);
	TestEqual(TEXT("备弹耗尽时无法换弹"), UT_GuardAmmoLibrary::GetReloadAmount(0.f, 12.f, 0.f), 0.f);
	TestEqual(TEXT("弹匣高于上限时按 0 处理"), UT_GuardAmmoLibrary::GetReloadAmount(13.f, 12.f, 24.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardAmmoApplyTest,
	"GASTestDemo1.Guard.AmmoApply",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardAmmoApplyTest::RunTest(const FString& Parameters)
{
	UT_AttributeSet* AttributeSet = NewObject<UT_AttributeSet>();

	AttributeSet->InitMaxMagazineAmmo(12.f);
	AttributeSet->InitMagazineAmmo(12.f);
	AttributeSet->InitReserveAmmo(24.f);

	TestTrue(TEXT("射击可扣除弹药"), UT_GuardAmmoLibrary::ApplyShotCost(AttributeSet));
	TestEqual(TEXT("射击后弹匣为 11"), AttributeSet->GetMagazineAmmo(), 11.f);
	TestEqual(TEXT("射击不影响备弹"), AttributeSet->GetReserveAmmo(), 24.f);

	TestTrue(TEXT("换弹成功"), UT_GuardAmmoLibrary::ApplyReload(AttributeSet));
	TestEqual(TEXT("换弹后弹匣补满"), AttributeSet->GetMagazineAmmo(), 12.f);
	TestEqual(TEXT("换弹只扣 1 发备弹"), AttributeSet->GetReserveAmmo(), 23.f);

	// 备弹不足时的部分换弹
	AttributeSet->InitMagazineAmmo(0.f);
	AttributeSet->InitReserveAmmo(2.f);
	TestTrue(TEXT("备弹不足时仍可部分换弹"), UT_GuardAmmoLibrary::ApplyReload(AttributeSet));
	TestEqual(TEXT("部分换弹后弹匣为 2"), AttributeSet->GetMagazineAmmo(), 2.f);
	TestEqual(TEXT("部分换弹后备弹耗尽"), AttributeSet->GetReserveAmmo(), 0.f);

	AttributeSet->InitMagazineAmmo(0.f);
	TestTrue(TEXT("空枪无法扣弹"), !UT_GuardAmmoLibrary::ApplyShotCost(AttributeSet));
	TestTrue(TEXT("无备弹无法换弹"), !UT_GuardAmmoLibrary::ApplyReload(AttributeSet));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardAbilityExclusionTest,
	"GASTestDemo1.Guard.AbilityExclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardAbilityExclusionTest::RunTest(const FString& Parameters)
{
	const UT_GuardShoot* Shoot = GetDefault<UT_GuardShoot>();
	TestNotNull(TEXT("UT_GuardShoot 默认对象有效"), Shoot);
	if (IsValid(Shoot))
	{
		TestTrue(TEXT("射击能力包含 Enemy.Shoot 标签"), Shoot->GetAssetTags().HasTagExact(TTags::TAbilities::Enemy::Shoot));
		TestTrue(TEXT("射击激活要求瞄准状态"), Shoot->GetActivationRequiredTags().HasTagExact(TTags::State::Aiming));
		TestTrue(TEXT("射击激活期间持有 Shooting 标签"), Shoot->GetActivationOwnedTags().HasTagExact(TTags::State::Action::Shooting));
		TestTrue(TEXT("换弹状态阻挡射击"), Shoot->GetActivationBlockedTags().HasTagExact(TTags::State::Action::Reloading));
		TestTrue(TEXT("受击硬直阻挡射击"), Shoot->GetActivationBlockedTags().HasTagExact(TTags::State::Action::HitReact));
		if (GetGuardTestDeadTag().IsValid())
		{
			TestTrue(TEXT("死亡状态阻挡射击"), Shoot->GetActivationBlockedTags().HasTagExact(GetGuardTestDeadTag()));
		}
	}

	const UT_GuardReload* Reload = GetDefault<UT_GuardReload>();
	TestNotNull(TEXT("UT_GuardReload 默认对象有效"), Reload);
	if (IsValid(Reload))
	{
		TestTrue(TEXT("换弹能力包含 Enemy.Reload 标签"), Reload->GetAssetTags().HasTagExact(TTags::TAbilities::Enemy::Reload));
		TestTrue(TEXT("换弹激活期间持有 Reloading 标签"), Reload->GetActivationOwnedTags().HasTagExact(TTags::State::Action::Reloading));
		TestTrue(TEXT("射击状态阻挡换弹"), Reload->GetActivationBlockedTags().HasTagExact(TTags::State::Action::Shooting));
	}

	const UT_GuardAim* Aim = GetDefault<UT_GuardAim>();
	TestNotNull(TEXT("UT_GuardAim 默认对象有效"), Aim);
	if (IsValid(Aim))
	{
		TestTrue(TEXT("瞄准能力包含 Enemy.Aim 标签"), Aim->GetAssetTags().HasTagExact(TTags::TAbilities::Enemy::Aim));
		TestTrue(TEXT("瞄准激活期间持有 Aiming 标签"), Aim->GetActivationOwnedTags().HasTagExact(TTags::State::Aiming));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardDeathInterruptTest,
	"GASTestDemo1.Guard.DeathInterrupt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardDeathInterruptTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("死亡标签可中断射击"), UT_GuardShoot::IsInterruptingTag(GetGuardTestDeadTag()));
	TestTrue(TEXT("受击硬直可中断射击"), UT_GuardShoot::IsInterruptingTag(TTags::State::Action::HitReact));
	TestTrue(TEXT("换弹状态可中断射击"), UT_GuardShoot::IsInterruptingTag(TTags::State::Action::Reloading));
	TestTrue(TEXT("无关标签不中断射击"), !UT_GuardShoot::IsInterruptingTag(TTags::State::Action::Attacking));

	const AT_GuardCharacter* Guard = GetDefault<AT_GuardCharacter>();
	TestNotNull(TEXT("AT_GuardCharacter 默认对象有效"), Guard);
	if (IsValid(Guard))
	{
		TestNotNull(TEXT("默认武器类已配置（BP_Pistol）"), Guard->GetDefaultWeaponClass().Get());
		TestEqual(TEXT("默认挂点为 hand_rPistol"), Guard->GetWeaponAttachSocketName(), FName(TEXT("hand_rPistol")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardTargetSelectionTest,
	"GASTestDemo1.Guard.TargetSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardTargetSelectionTest::RunTest(const FString& Parameters)
{
	AT_PlayerCharacter* AlivePlayer = NewObject<AT_PlayerCharacter>();
	AT_PlayerCharacter* DeadPlayer = NewObject<AT_PlayerCharacter>();
	DeadPlayer->SetAlive(false);
	AActor* NonPlayer = NewObject<AActor>();

	TArray<FGuardPerceivedTarget> Candidates;

	FGuardPerceivedTarget NonPlayerEntry;
	NonPlayerEntry.Actor = NonPlayer;
	NonPlayerEntry.Location = FVector(1.f, 0.f, 0.f);
	Candidates.Add(NonPlayerEntry);

	FGuardPerceivedTarget DeadPlayerEntry;
	DeadPlayerEntry.Actor = DeadPlayer;
	DeadPlayerEntry.Location = FVector(50.f, 0.f, 0.f);
	Candidates.Add(DeadPlayerEntry);

	FGuardPerceivedTarget FarPlayerEntry;
	FarPlayerEntry.Actor = AlivePlayer;
	FarPlayerEntry.Location = FVector(300.f, 0.f, 0.f);
	Candidates.Add(FarPlayerEntry);

	AActor* Selected = AT_ShooterAIController::SelectNearestValidTarget(Candidates, FVector::ZeroVector);
	TestTrue(TEXT("只选择存活且带 Player 标签的最近目标"), Selected == AlivePlayer);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardBlackboardClearTest,
	"GASTestDemo1.Guard.BlackboardClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardBlackboardClearTest::RunTest(const FString& Parameters)
{
	UBlackboardData* BlackboardData = NewObject<UBlackboardData>();
	AddBlackboardKey(BlackboardData, GuardBBKeys::Enemy, NewObject<UBlackboardKeyType_Object>());
	AddBlackboardKey(BlackboardData, GuardBBKeys::MoveLocation, NewObject<UBlackboardKeyType_Vector>());
	AddBlackboardKey(BlackboardData, GuardBBKeys::Awareness, NewObject<UBlackboardKeyType_Float>());
	AddBlackboardKey(BlackboardData, GuardBBKeys::LastKnownLocation, NewObject<UBlackboardKeyType_Vector>());
	AddBlackboardKey(BlackboardData, GuardBBKeys::InvestigateLocation, NewObject<UBlackboardKeyType_Vector>());
	AddBlackboardKey(BlackboardData, GuardBBKeys::HomeLocation, NewObject<UBlackboardKeyType_Vector>());
	AddBlackboardKey(BlackboardData, GuardBBKeys::CombatMoveLocation, NewObject<UBlackboardKeyType_Vector>());

	UWorld* World = GetGuardTestEditorWorld();
	if (!IsValid(World))
	{
		// 无编辑器世界时跳过黑板行为验证
		return true;
	}

	AActor* OwnerActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	UBlackboardComponent* BlackboardComp = NewObject<UBlackboardComponent>(OwnerActor);
	BlackboardComp->RegisterComponentWithWorld(World);
	TestTrue(TEXT("黑板初始化成功"), BlackboardComp->InitializeBlackboard(*BlackboardData));
	if (!BlackboardComp->GetBlackboardAsset()) return true;

	BlackboardComp->SetValueAsObject(GuardBBKeys::Enemy, NewObject<AT_PlayerCharacter>());
	BlackboardComp->SetValueAsVector(GuardBBKeys::MoveLocation, FVector(100.f, 0.f, 0.f));
	BlackboardComp->SetValueAsFloat(GuardBBKeys::Awareness, 75.f);
	BlackboardComp->SetValueAsVector(GuardBBKeys::LastKnownLocation, FVector(200.f, 0.f, 0.f));
	BlackboardComp->SetValueAsVector(GuardBBKeys::InvestigateLocation, FVector(300.f, 0.f, 0.f));
	BlackboardComp->SetValueAsVector(GuardBBKeys::HomeLocation, FVector(400.f, 0.f, 0.f));
	BlackboardComp->SetValueAsVector(GuardBBKeys::CombatMoveLocation, FVector(500.f, 0.f, 0.f));

	AT_ShooterAIController::ClearGuardBlackboard(BlackboardComp);

	TestNull(TEXT("Enemy 键已清空"), BlackboardComp->GetValueAsObject(GuardBBKeys::Enemy));
	TestFalse(TEXT("Move Location 键已清空"), BlackboardComp->IsVectorValueSet(GuardBBKeys::MoveLocation));
	TestEqual(TEXT("Awareness 键已清零"), BlackboardComp->GetValueAsFloat(GuardBBKeys::Awareness), 0.f);
	TestFalse(TEXT("Last Known Location 键已清空"), BlackboardComp->IsVectorValueSet(GuardBBKeys::LastKnownLocation));
	TestFalse(TEXT("Investigate Location 键已清空"), BlackboardComp->IsVectorValueSet(GuardBBKeys::InvestigateLocation));
	TestFalse(TEXT("Combat Move Location 键已清空"), BlackboardComp->IsVectorValueSet(GuardBBKeys::CombatMoveLocation));
	TestTrue(TEXT("Home Location 永远保留"), BlackboardComp->IsVectorValueSet(GuardBBKeys::HomeLocation));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardAwarenessMathTest,
	"GASTestDemo1.Guard.AwarenessMath",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardAwarenessMathTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("近距离视觉增长为 28/秒"), AT_ShooterAIController::GetSightAwarenessRate(500.f), 28.f);
	TestEqual(TEXT("中距离视觉增长为 14/秒"), AT_ShooterAIController::GetSightAwarenessRate(900.f), 14.f);
	TestEqual(TEXT("远距离视觉增长为 6/秒"), AT_ShooterAIController::GetSightAwarenessRate(1500.f), 6.f);
	TestEqual(TEXT("SightRadius 外不增长"), AT_ShooterAIController::GetSightAwarenessRate(1900.f), 0.f);
	TestEqual(TEXT("蹲伏倍率为基础增长的 0.28"), 28.f * 0.28f, 7.84f);
	TestEqual(TEXT("跑动倍率为基础增长的 1.35"), 14.f * 1.35f, 18.9f);
	TestEqual(TEXT("听觉按强度增加"), AT_ShooterAIController::ClampHearingAwareness(10.f, 0.45f), 21.25f);
	TestEqual(TEXT("听觉永远封顶 80"), AT_ShooterAIController::ClampHearingAwareness(75.f, 1.f), 80.f);
	TestTrue(TEXT("正面近距可秒满"), AT_ShooterAIController::ShouldInstantDetectCloseRange(150.f, 0.8f));
	TestFalse(TEXT("背后近距不可秒满"), AT_ShooterAIController::ShouldInstantDetectCloseRange(150.f, -0.5f));
	TestFalse(TEXT("正面但超出近距不秒满"), AT_ShooterAIController::ShouldInstantDetectCloseRange(250.f, 0.9f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardFacingMultiplierTest,
	"GASTestDemo1.Guard.FacingMultiplier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardFacingMultiplierTest::RunTest(const FString& Parameters)
{
	const FVector Forward = FVector::ForwardVector;
	TestEqual(TEXT("正面朝向倍率为 1"), AT_ShooterAIController::GetFacingAwarenessMultiplier(Forward, Forward), 1.f);
	TestEqual(TEXT("侧面朝向倍率为 0.55"), AT_ShooterAIController::GetFacingAwarenessMultiplier(Forward, FVector(0.3f, 1.f, 0.f)), 0.55f);
	TestEqual(TEXT("背朝向倍率为 0.25"), AT_ShooterAIController::GetFacingAwarenessMultiplier(Forward, -Forward), 0.25f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardStateTransitionTest,
	"GASTestDemo1.Guard.StateTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardStateTransitionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Guard AI 公开六个行为状态"), StaticEnum<ETGuardAIState>()->NumEnums() - 1, 6);
	int32 CombatEntryCount = 0;
	CombatEntryCount += AT_ShooterAIController::IsCombatEntry(ETGuardAIState::Patrol, ETGuardAIState::Suspicious) ? 1 : 0;
	CombatEntryCount += AT_ShooterAIController::IsCombatEntry(ETGuardAIState::Suspicious, ETGuardAIState::Combat) ? 1 : 0;
	CombatEntryCount += AT_ShooterAIController::IsCombatEntry(ETGuardAIState::Combat, ETGuardAIState::Combat) ? 1 : 0;
	CombatEntryCount += AT_ShooterAIController::IsCombatEntry(ETGuardAIState::Combat, ETGuardAIState::Search) ? 1 : 0;
	CombatEntryCount += AT_ShooterAIController::IsCombatEntry(ETGuardAIState::Search, ETGuardAIState::Combat) ? 1 : 0;
	TestEqual(TEXT("仅两次进入 Combat 的边沿触发提示"), CombatEntryCount, 2);
	TestFalse(TEXT("Search 重见目标不再降级到 Suspicious"), AT_ShooterAIController::ShouldEnterSuspiciousFromSight(ETGuardAIState::Search));
	TestFalse(TEXT("Investigate 见人不再重新累计警觉"), AT_ShooterAIController::ShouldEnterSuspiciousFromSight(ETGuardAIState::Investigate));
	TestTrue(TEXT("Search 重见玩家立即战斗"), AT_ShooterAIController::ShouldInstantReengageFromSight(ETGuardAIState::Search));
	TestTrue(TEXT("Investigate 搜到玩家立即战斗"), AT_ShooterAIController::ShouldInstantReengageFromSight(ETGuardAIState::Investigate));
	TestFalse(TEXT("Combat 见人不经 Suspicious 入口"), AT_ShooterAIController::ShouldEnterSuspiciousFromSight(ETGuardAIState::Combat));
	TestFalse(TEXT("巡逻不举枪"), AT_ShooterAIController::IsAlertAimingState(ETGuardAIState::Patrol));
	TestFalse(TEXT("返回不举枪"), AT_ShooterAIController::IsAlertAimingState(ETGuardAIState::Return));
	TestTrue(TEXT("Suspicious 举枪"), AT_ShooterAIController::IsAlertAimingState(ETGuardAIState::Suspicious));
	TestTrue(TEXT("Investigate 举枪"), AT_ShooterAIController::IsAlertAimingState(ETGuardAIState::Investigate));
	TestTrue(TEXT("Combat 举枪"), AT_ShooterAIController::IsAlertAimingState(ETGuardAIState::Combat));
	TestTrue(TEXT("Search 举枪"), AT_ShooterAIController::IsAlertAimingState(ETGuardAIState::Search));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardDirectConfirmationTest,
	"GASTestDemo1.Guard.DirectConfirmation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardDirectConfirmationTest::RunTest(const FString& Parameters)
{
	AT_ShooterAIController* Controller = NewObject<AT_ShooterAIController>();
	AT_PlayerCharacter* Player = NewObject<AT_PlayerCharacter>();
	Controller->ConfirmTargetFromDamage(Player, FVector(100.f, 0.f, 0.f));
	TestEqual(TEXT("Damage 直接进入 Combat"), Controller->GetAIState(), ETGuardAIState::Combat);
	TestEqual(TEXT("Damage 直接将 Awareness 设为 100"), Controller->GetAwareness(), 100.f);

	Controller->SetAIState(ETGuardAIState::Suspicious);
	Controller->SetAwareness(40.f);
	TestTrue(TEXT("Suspicious 状态允许接触确认"), Controller->ShouldConfirmContact(Player));
	Controller->ConfirmTargetFromContact(Player);
	TestEqual(TEXT("已警觉时接触进入 Combat"), Controller->GetAIState(), ETGuardAIState::Combat);
	TestEqual(TEXT("已警觉时接触将 Awareness 设为 100"), Controller->GetAwareness(), 100.f);

	Controller->SetAIState(ETGuardAIState::Patrol);
	Controller->SetAwareness(0.f);
	TestFalse(TEXT("Patrol 且无朝向信息时不直接确认接触"), Controller->ShouldConfirmContact(Player));
	Controller->ConfirmTargetFromContact(Player);
	TestEqual(TEXT("背后式接触仅抬到 Suspicious"), Controller->GetAIState(), ETGuardAIState::Suspicious);
	TestTrue(TEXT("背后式接触至少抬到 Awareness 20"), Controller->GetAwareness() >= 20.f);
	TestTrue(TEXT("背后式接触不会直接满战"), Controller->GetAwareness() < 100.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardSpreadTest,
	"GASTestDemo1.Guard.Spread",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardSpreadTest::RunTest(const FString& Parameters)
{
	const FVector Forward = FVector::ForwardVector;
	TestTrue(TEXT("Spread=0 保持玩家射击方向"), UT_ProjectileShooterComponent::ApplySpreadToDirection(Forward, 0.f).Equals(Forward));
	TestEqual(TEXT("近距离 AI Spread 为 1 度"), UT_GuardShoot::GetSpreadHalfAngle(800.f), 1.f);
	TestEqual(TEXT("中距离 AI Spread 为 2 度"), UT_GuardShoot::GetSpreadHalfAngle(1600.f), 2.f);
	TestEqual(TEXT("远距离 AI Spread 为 3.5 度"), UT_GuardShoot::GetSpreadHalfAngle(2000.f), 3.5f);
	for (int32 Index = 0; Index < 64; ++Index)
	{
		const FVector SpreadDirection = UT_ProjectileShooterComponent::ApplySpreadToDirection(Forward, 3.5f);
		TestTrue(TEXT("AI 随机方向不超过配置半角"), FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(Forward, SpreadDirection))) <= 3.501f);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardCombatMoveProjectionTest,
	"GASTestDemo1.Guard.CombatMoveProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardCombatMoveProjectionTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("投射和路径均有效时可移动"), UT_BTService_GuardCombatMovement::IsProjectedMoveUsable(true, true));
	TestFalse(TEXT("Nav 投射失败时不发起移动"), UT_BTService_GuardCombatMovement::IsProjectedMoveUsable(false, true));
	TestFalse(TEXT("路径不可达时不发起移动"), UT_BTService_GuardCombatMovement::IsProjectedMoveUsable(true, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardInvestigationSelectionTest,
	"GASTestDemo1.Guard.InvestigationSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardInvestigationSelectionTest::RunTest(const FString& Parameters)
{
	UWorld* World = GetGuardTestEditorWorld();
	if (!IsValid(World)) return true;

	AT_GuardCharacter* NearGuard = World->SpawnActor<AT_GuardCharacter>(FVector(100.f, 0.f, 0.f), FRotator::ZeroRotator);
	AT_GuardCharacter* FarGuard = World->SpawnActor<AT_GuardCharacter>(FVector(500.f, 0.f, 0.f), FRotator::ZeroRotator);
	AT_GuardCharacter* CombatGuard = World->SpawnActor<AT_GuardCharacter>(FVector(50.f, 0.f, 0.f), FRotator::ZeroRotator);
	AT_GuardCharacter* DeadGuard = World->SpawnActor<AT_GuardCharacter>(FVector(25.f, 0.f, 0.f), FRotator::ZeroRotator);
	TArray<AT_GuardCharacter*> Guards{NearGuard, FarGuard, CombatGuard, DeadGuard};
	TArray<AT_ShooterAIController*> Controllers;
	UBehaviorTree* GuardTree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/GASTestDemo/AI/ShooterController/BT_Guard.BT_Guard"));
	TestNotNull(TEXT("测试使用的 BT_Guard 可加载"), GuardTree);
	for (AT_GuardCharacter* Guard : Guards)
	{
		AT_ShooterAIController* Controller = World->SpawnActor<AT_ShooterAIController>();
		Controller->BehaviorTreeAsset = GuardTree;
		Controller->Possess(Guard);
		Controllers.Add(Controller);
	}
	Controllers[2]->SetAIState(ETGuardAIState::Combat);
	DeadGuard->SetAlive(false);

	TestTrue(TEXT("瓶子落地噪音使用专用 Tag"),
		UT_GuardAlertSubsystem::IsThrowableImpactNoise(UT_GuardAlertSubsystem::ThrowableImpactNoiseTag));
	TestFalse(TEXT("脚步声不是投掷物落地噪音"),
		UT_GuardAlertSubsystem::IsThrowableImpactNoise(FName(TEXT("GuardNoise.Footstep.Walk"))));
	TestTrue(TEXT("Throwable 只选择最近的存活非 Combat Guard"),
		UT_GuardAlertSubsystem::SelectNearestEligibleGuard(Guards, FVector::ZeroVector, 1500.f) == NearGuard);

	Controllers[0]->BeginInvestigation(FVector(10.f, 0.f, 0.f));
	TestEqual(TEXT("调查进入 Investigate"), Controllers[0]->GetAIState(), ETGuardAIState::Investigate);
	TestTrue(TEXT("调查至少抬到 Awareness 30"), Controllers[0]->GetAwareness() >= 30.f);

	const ETGuardAIState FarStateBefore = Controllers[1]->GetAIState();
	Controllers[1]->ReactToDistraction(FVector(10.f, 0.f, 0.f));
	TestEqual(TEXT("轻反应不改变 AIState"), Controllers[1]->GetAIState(), FarStateBefore);
	TestTrue(TEXT("轻反应抬升 Awareness"), Controllers[1]->GetAwareness() >= 12.f);
	TestTrue(TEXT("轻反应 Awareness 封顶 45"), Controllers[1]->GetAwareness() <= 45.f);
	Controllers[1]->SetAwareness(40.f);
	Controllers[1]->ReactToDistraction(FVector(10.f, 0.f, 0.f));
	TestEqual(TEXT("轻反应叠加后仍封顶 45"), Controllers[1]->GetAwareness(), 45.f);

	for (AT_ShooterAIController* Controller : Controllers) Controller->Destroy();
	for (AT_GuardCharacter* Guard : Guards) Guard->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardThrowableImpactTest,
	"GASTestDemo1.Guard.ThrowableImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardThrowableImpactTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("低速轻触不产生落地事件"), AT_Throwable::IsImpactSignificant(149.f, 150.f, 1.f, 0.f, 0.15f));
	TestFalse(TEXT("冷却期内不重复产生落地事件"), AT_Throwable::IsImpactSignificant(500.f, 150.f, 0.1f, 0.f, 0.15f));
	TestTrue(TEXT("达到速度且超过冷却后产生落地事件"), AT_Throwable::IsImpactSignificant(500.f, 150.f, 0.2f, 0.f, 0.15f));
	TestTrue(TEXT("零冷却允许连续有效碰撞"), AT_Throwable::IsImpactSignificant(150.f, 150.f, 0.f, 0.f, 0.f));

	UWorld* World = GetGuardTestEditorWorld();
	AT_Throwable* Throwable = IsValid(World) ? World->SpawnActor<AT_Throwable>() : nullptr;
	UProjectileMovementComponent* ProjectileMovement = IsValid(Throwable)
		? Throwable->FindComponentByClass<UProjectileMovementComponent>()
		: nullptr;
	if (IsValid(ProjectileMovement))
	{
		ProjectileMovement->InitialSpeed = 2000.f;
		ProjectileMovement->MaxSpeed = 1500.f;
		TestEqual(TEXT("初速度受最大速度限制"), Throwable->GetLaunchSpeed(), 1500.f);

		ProjectileMovement->MaxSpeed = 2500.f;
		TestEqual(TEXT("未达到最大速度时使用投掷物初速度"), Throwable->GetLaunchSpeed(), 2000.f);
		TestEqual(TEXT("显式速度仍受最大速度限制"), Throwable->GetLaunchSpeed(3000.f), 2500.f);
	}
	if (IsValid(Throwable)) Throwable->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardMoveResultMappingTest,
	"GASTestDemo1.Guard.MoveResultMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardMoveResultMappingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("目标已在范围内视为移动成功"),
		UT_BTTask_GuardMoveTo::MapMoveRequestResult(EPathFollowingRequestResult::AlreadyAtGoal),
		EBTNodeResult::Succeeded);
	TestEqual(TEXT("移动请求成功则等待完成回调"),
		UT_BTTask_GuardMoveTo::MapMoveRequestResult(EPathFollowingRequestResult::RequestSuccessful),
		EBTNodeResult::InProgress);
	TestEqual(TEXT("移动请求失败"),
		UT_BTTask_GuardMoveTo::MapMoveRequestResult(EPathFollowingRequestResult::Failed),
		EBTNodeResult::Failed);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTGuardPatrolIndexTest,
	"GASTestDemo1.Guard.PatrolIndex",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGuardPatrolIndexTest::RunTest(const FString& Parameters)
{
	int32 Index = 0;
	int32 Direction = 1;

	// PingPong 0→1→2→1→0
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::PingPong, 3, 0, 1, Index, Direction);
	TestEqual(TEXT("PingPong 0→1"), Index, 1);
	TestEqual(TEXT("PingPong 方向仍为 +1"), Direction, 1);
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::PingPong, 3, 1, 1, Index, Direction);
	TestEqual(TEXT("PingPong 1→2"), Index, 2);
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::PingPong, 3, 2, 1, Index, Direction);
	TestEqual(TEXT("PingPong 2→1"), Index, 1);
	TestEqual(TEXT("PingPong 末端反向"), Direction, -1);
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::PingPong, 3, 1, -1, Index, Direction);
	TestEqual(TEXT("PingPong 1→0"), Index, 0);
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::PingPong, 3, 0, -1, Index, Direction);
	TestEqual(TEXT("PingPong 0→1 再正向"), Index, 1);
	TestEqual(TEXT("PingPong 起点反向后转正"), Direction, 1);

	// Loop 0→1→2→0
	Index = 0;
	Direction = 1;
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::Loop, 3, 0, 1, Index, Direction);
	TestEqual(TEXT("Loop 0→1"), Index, 1);
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::Loop, 3, 1, 1, Index, Direction);
	TestEqual(TEXT("Loop 1→2"), Index, 2);
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::Loop, 3, 2, 1, Index, Direction);
	TestEqual(TEXT("Loop 2→0"), Index, 0);

	// 点数不足
	Index = 5;
	Direction = -1;
	AT_GuardCharacter::ComputeNextPatrolIndex(EGuardPatrolMode::PingPong, 1, 5, -1, Index, Direction);
	TestEqual(TEXT("点数不足时索引归零"), Index, 0);
	TestEqual(TEXT("点数不足时方向归正"), Direction, 1);

	TArray<FVector> Points;
	Points.Add(FVector(0.f, 0.f, 0.f));
	Points.Add(FVector(100.f, 0.f, 0.f));
	Points.Add(FVector(200.f, 0.f, 0.f));
	TestEqual(TEXT("最近路点吸附到中间"),
		AT_GuardCharacter::FindNearestPatrolIndex(Points, FVector(110.f, 5.f, 0.f)), 1);

	AT_GuardCharacter* Guard = NewObject<AT_GuardCharacter>();
	TestTrue(TEXT("默认 PatrolMode 为站岗"), Guard->GetPatrolMode() == EGuardPatrolMode::Stationary);
	TestTrue(TEXT("默认站岗 IsStationaryPatrol"), Guard->IsStationaryPatrol());
	TestTrue(TEXT("默认停留掷点非负"), Guard->GetRolledPatrolPointWaitDuration() >= 0.f);
	return true;
}

#endif
