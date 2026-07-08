// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"


namespace TTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(None);
	
	namespace State
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LockOn);
		
		namespace Action
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Busy);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacking);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dodging);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Casting);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dead);
		}
	}
	
	namespace SetByCaller
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Projectile);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Melee);
		namespace Player
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
		}
	}
	namespace TAbilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(BlockHitReact);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(StandingDodge);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(LockOn);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchLockOnTargetLeft);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchLockOnTargetRight);
		
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
		}
	}
	namespace Events
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(KillScored);
		
		namespace Player
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(StandingDodge);
			
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(LockOn);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchLockOnTargetLeft);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(SwitchLockOnTargetRight);
		}
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndAttack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTraceHit);
		}
	}
	namespace Cooldown
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
	}
}
