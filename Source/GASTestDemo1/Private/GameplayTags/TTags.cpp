// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayTags/TTags.h"

namespace TTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(None, "TTags.None", "无标签");
	
	namespace State
	{
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(LockOn, "TTags.State.LockOn", "角色正在锁定敌人");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Aiming, "TTags.State.Aiming", "角色正在瞄准");
		
		namespace Action
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Busy, "TTags.State.Action.Busy", "角色正在执行动作，阻止重复触发其他动作");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Grabbing, "TTags.State.Action.Grabbing", "角色正在抓握");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Attacking, "TTags.State.Action.Attacking", "角色正在攻击");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dodging, "TTags.State.Action.Dodging", "角色正在闪避");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Rolling, "TTags.State.Action.Rolling", "角色正在翻滚");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Traversing, "TTags.State.Action.Traversing", "角色正在执行翻越动作");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(HitReact, "TTags.State.Action.HitReact", "角色正在播放受击反应");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Casting, "TTags.State.Action.Casting", "角色正在施法或释放技能");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(ComboWindow, "TTags.State.Action.ComboWindow", "连击输入窗口");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Shooting, "TTags.State.Action.Shooting", "角色正在执行射击动作");
		}

		namespace Grab
		{
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Wall, "TTags.State.Grab.Wall", "角色正在抓握墙体");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Bar, "TTags.State.Grab.Bar", "角色正在抓握横杆");
		}
	}
	
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
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(StandingDodge, "TTags.TAbilities.StandingDodge", "站立闪避标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Roll, "TTags.TAbilities.Roll", "翻滚标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Traversal, "TTags.TAbilities.Traversal", "翻越能力标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Grab, "TTags.TAbilities.Grab", "抓握能力标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Aim, "TTags.TAbilities.Aim", "瞄准能力标签");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(Shoot, "TTags.TAbilities.Shoot", "射击能力标签");
		
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(LockOn, "TTags.TAbilities.LockOn", "锁定敌人能力");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SwitchLockOnTargetLeft, "TTags.TAbilities.SwitchLockOnTargetLeft", "向左切换锁定目标");
		UE_DEFINE_GAMEPLAY_TAG_COMMENT(SwitchLockOnTargetRight, "TTags.TAbilities.SwitchLockOnTargetRight", "向右切换锁定目标");
		
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
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(StandingDodge, "TTags.Events.Player.StandingDodge", "站立闪避事件标签")
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Roll, "TTags.Events.Player.Roll", "翻滚事件标签")
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(LockOn, "TTags.Events.Player.LockOn", "玩家锁定事件");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(SwitchLockOnTargetLeft, "TTags.Events.Player.SwitchLockOnTargetLeft", "玩家向左切换锁定目标事件");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(SwitchLockOnTargetRight, "TTags.Events.Player.SwitchLockOnTargetRight", "玩家向右切换锁定目标事件");
			
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combo_SaveInput, "TTags.Events.Player.Combo.SaveInput", "允许保存连击输入");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combo_Next, "TTags.Events.Player.Combo.Next", "进入下一段连击");
			UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combo_End, "TTags.Events.Player.Combo.End", "连击结束");

			namespace Grab
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Catch, "TTags.Events.Player.Grab.Catch", "开始抓握检测事件");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(StopCatch, "TTags.Events.Player.Grab.StopCatch", "停止抓握检测事件");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Release, "TTags.Events.Player.Grab.Release", "松开抓握事件");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Move, "TTags.Events.Player.Grab.Move", "抓握侧移事件");
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Jump, "TTags.Events.Player.Grab.Jump", "抓握跳跃事件");
			}

			namespace Shoot
			{
				UE_DEFINE_GAMEPLAY_TAG_COMMENT(Fire, "TTags.Events.Player.Shoot.Fire", "射击动画的实际开火事件");
			}
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

