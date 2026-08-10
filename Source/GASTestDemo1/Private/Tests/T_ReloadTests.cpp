#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Abilities/T_Reload.h"
#include "AbilitySystem/Abilities/T_Shoot.h"
#include "GameplayTags/TTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTReloadTagsTest,
	"GASTestDemo1.Player.ReloadTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTReloadTagsTest::RunTest(const FString& Parameters)
{
	UT_Reload* Reload = NewObject<UT_Reload>();
	TestTrue(TEXT("换弹能力包含 Reload 标签"), Reload->GetAssetTags().HasTagExact(TTags::TAbilities::Reload));
	TestTrue(TEXT("换弹激活期间持有 Reloading 标签"), Reload->GetActivationOwnedTags().HasTagExact(TTags::State::Action::Reloading));
	TestTrue(TEXT("换弹阻挡射击状态，防止蒙太奇冲突"), Reload->GetActivationBlockedTags().HasTagExact(TTags::State::Action::Shooting));
	TestTrue(TEXT("换弹不能重复触发"), Reload->GetActivationBlockedTags().HasTagExact(TTags::State::Action::Reloading));

	UT_Shoot* Shoot = NewObject<UT_Shoot>();
	TestTrue(TEXT("射击被换弹状态阻挡"), Shoot->GetActivationBlockedTags().HasTagExact(TTags::State::Action::Reloading));
	return true;
}

#endif
