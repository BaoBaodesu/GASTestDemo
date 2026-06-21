// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"


namespace TTags
{
	namespace TAbilities
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(ActivateOnGiven);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Death);
		
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Primary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Secondary);
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(Tertiary);
		
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attack);
		}
	}
	namespace Events
	{
		UE_DECLARE_GAMEPLAY_TAG_EXTERN(KillScored);
		namespace Enemy
		{
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(HitReact);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(EndAttack);
			UE_DECLARE_GAMEPLAY_TAG_EXTERN(MeleeTraceHit);
		}
	}
}
