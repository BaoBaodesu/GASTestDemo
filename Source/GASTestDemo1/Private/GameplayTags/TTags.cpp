// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/TTags.h"

namespace TTags
{
	namespace TAbilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "TTags.TAbilities.ActivateOnGiven", "被赋予能力后会立即激活的标签");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary, "TTags.TAbilities.Primary", "主要能力标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary, "TTags.TAbilities.Secondary", "次要能力标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary, "TTags.TAbilities.Tertiary", "第三能力标签");
	}
	namespace Events
	{
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "TTags.Events.Enemy.HitReact", "敌人受击反应事件")
		}
	}
}
