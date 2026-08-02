#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Abilities/T_Grab.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameplayTags/TTags.h"
#include "Player/Components/T_GrabComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTGrabGASConfigurationTest, "GASTestDemo1.Grab.GASConfiguration", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTGrabGASConfigurationTest::RunTest(const FString& Parameters)
{
	const UT_Grab* GrabAbility = GetDefault<UT_Grab>();
	TestNotNull(TEXT("UT_Grab 默认对象有效"), GrabAbility);
	if (IsValid(GrabAbility)) TestTrue(TEXT("UT_Grab 包含 Grab Ability Asset Tag"), GrabAbility->GetAssetTags().HasTagExact(TTags::TAbilities::Grab));

	const AT_PlayerCharacter* PlayerCharacter = GetDefault<AT_PlayerCharacter>();
	TestNotNull(TEXT("玩家角色默认对象有效"), PlayerCharacter);
	const UT_GrabComponent* GrabComponent = IsValid(PlayerCharacter) ? PlayerCharacter->FindComponentByClass<UT_GrabComponent>() : nullptr;
	TestNotNull(TEXT("玩家角色包含独立抓握组件"), GrabComponent);
	if (IsValid(GrabComponent))
	{
		TestFalse(TEXT("抓握组件初始未抓握"), GrabComponent->IsGrabbed());
		TestTrue(TEXT("抓握组件初始允许移动"), GrabComponent->CanMove());
		TestEqual(TEXT("抓握组件初始类型为空"), GrabComponent->GetGrabType(), ET_GrabType::None);
	}

	return true;
}

#endif
