// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/TTags.h"

namespace TTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(None, "TTags.None", "无标签");
	
	namespace SetByCaller
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Projectile, "TTags.SetByCaller.Projectile", "投射物的标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Melee, "TTags.SetByCaller.Melee", "近战的标签");
		
		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary, "TTags.SetByCaller.Player.Secondary", "调用玩家次要攻击的标签");
		}
	}
	namespace TAbilities
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(ActivateOnGiven, "TTags.TAbilities.ActivateOnGiven", "被赋予能力后会立即激活的标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "TTags.TAbilities.Death", "死亡标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(BlockHitReact, "TTags.TAbilities.BlockHitReact", "阻止命中反应标签");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary, "TTags.TAbilities.Primary", "主要能力标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary, "TTags.TAbilities.Secondary", "次要能力标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Tertiary, "TTags.TAbilities.Tertiary", "第三能力标签");
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attack, "TTags.TAbilities.Enemy.Attack", "敌人攻击标签");
		}
	}
	namespace Events
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(KillScored, "TTags.Events.KillScored", "击杀得分的事件");
		
		namespace Player
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "TTags.Events.Player.HitReact", "玩家受击反应事件")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Death, "TTags.Events.Player.Death", "玩家死亡标签")
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Primary, "TTags.Events.Player.Primary", "主要能力事件标签")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary, "TTags.Events.Player.Secondary", "次要能力事件标签")
		}
		namespace Enemy
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "TTags.Events.Enemy.HitReact", "敌人受击反应事件");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(EndAttack, "TTags.Events.Enemy.EndAttack", "敌人攻击结束标签");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(MeleeTraceHit, "TTags.Events.Enemy.MeleeTraceHit", "敌人近战命中的标签");
		}
	}
	namespace Cooldown
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Secondary, "TTags.Cooldown.Secondary", "次要攻击冷却标签");
	}
}
