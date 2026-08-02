// Fill out your copyright notice in the Description page of Project Settings.


#include "Notifies/AnimNotify_SendGameplayEvent.h"

#include "AbilitySystemBlueprintLibrary.h"

void UAnimNotify_SendGameplayEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	if (!IsValid(MeshComp) || !EventTag.IsValid()) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner)) return;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Owner;
	Payload.Target = Owner;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Owner, EventTag, Payload);
}
