#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AI/Abilities/T_GuardAim.h"
#include "AI/Abilities/T_GuardAmmoLibrary.h"
#include "AI/Abilities/T_GuardReload.h"
#include "AI/Abilities/T_GuardShoot.h"
#include "AI/T_ShooterAIController.h"
#include "AI/BehaviorTree/T_BTTask_GuardMoveTo.h"
#include "AbilitySystem/T_AttributeSet.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "Characters/T_GuardCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "Editor.h"
#include "GameplayTags/TTags.h"

namespace
{
	const FGameplayTag& GetGuardDeadTag()
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
		if (GetGuardDeadTag().IsValid())
		{
			TestTrue(TEXT("死亡状态阻挡射击"), Shoot->GetActivationBlockedTags().HasTagExact(GetGuardDeadTag()));
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
	TestTrue(TEXT("死亡标签可中断射击"), UT_GuardShoot::IsInterruptingTag(GetGuardDeadTag()));
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
	AddBlackboardKey(BlackboardData, TEXT("Enemy"), NewObject<UBlackboardKeyType_Object>());
	AddBlackboardKey(BlackboardData, TEXT("Enemy Spotted"), NewObject<UBlackboardKeyType_Bool>());
	AddBlackboardKey(BlackboardData, TEXT("Move Location"), NewObject<UBlackboardKeyType_Vector>());
	AddBlackboardKey(BlackboardData, TEXT("Noise Location"), NewObject<UBlackboardKeyType_Vector>());

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
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

	BlackboardComp->SetValueAsObject(TEXT("Enemy"), NewObject<AT_PlayerCharacter>());
	BlackboardComp->SetValueAsBool(TEXT("Enemy Spotted"), true);
	BlackboardComp->SetValueAsVector(TEXT("Move Location"), FVector(100.f, 0.f, 0.f));
	BlackboardComp->SetValueAsVector(TEXT("Noise Location"), FVector(200.f, 0.f, 0.f));

	AT_ShooterAIController::ClearGuardBlackboard(BlackboardComp);

	TestNull(TEXT("Enemy 键已清空"), BlackboardComp->GetValueAsObject(TEXT("Enemy")));
	TestFalse(TEXT("Enemy Spotted 键已清空"), BlackboardComp->GetValueAsBool(TEXT("Enemy Spotted")));
	TestFalse(TEXT("Move Location 键已清空"), BlackboardComp->IsVectorValueSet(TEXT("Move Location")));
	TestFalse(TEXT("Noise Location 键已清空"), BlackboardComp->IsVectorValueSet(TEXT("Noise Location")));
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

#endif
