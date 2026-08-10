#include "Notifies/AnimNotify_ContextualExecution.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GameplayEffects/T_ExecutionDamageEffect.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_EnemyCharacter.h"
#include "ContextualAnimSceneActorComponent.h"
#include "ContextualAnimTypes.h"
#include "GameplayEffect.h"
#include "GameplayTags/TTags.h"

UAnimNotify_ContextualExecution::UAnimNotify_ContextualExecution()
{
	DamageEffectClass = UT_ExecutionDamageEffect::StaticClass();
	SetByCallerDataTag = TTags::SetByCaller::Melee.GetTag();
}

void UAnimNotify_ContextualExecution::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!IsValid(MeshComp)) return;

	AActor* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner)) return;
	if (!Owner->HasAuthority()) return;

	AActor* TargetActor = ResolveExecutionTarget(Owner);
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimNotify_ContextualExecution: no valid target. Owner=%s Role=%s."),
			*Owner->GetName(), *TargetRole.ToString());
		return;
	}

	AT_BaseCharacter* TargetCharacter = Cast<AT_BaseCharacter>(TargetActor);
	if (!IsValid(TargetCharacter) || !TargetCharacter->IsAlive()) return;

	if (bSkipDeathAnimation)
	{
		if (AT_EnemyCharacter* Enemy = Cast<AT_EnemyCharacter>(TargetCharacter))
		{
			Enemy->RequestSkipDeathPresentation(DestroyDelay);
		}
	}

	if (!IsValid(DamageEffectClass))
	{
		UE_LOG(LogTemp, Error, TEXT("AnimNotify_ContextualExecution: DamageEffectClass is invalid. Owner=%s Target=%s."),
			*Owner->GetName(), *TargetActor->GetName());
		return;
	}

	if (!SetByCallerDataTag.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("AnimNotify_ContextualExecution: SetByCallerDataTag is invalid. Owner=%s Target=%s."),
			*Owner->GetName(), *TargetActor->GetName());
		return;
	}

	AActor* InstigatorActor = ResolveInstigator(Owner, TargetActor);
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InstigatorActor);
	if (!IsValid(SourceASC))
	{
		SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		UE_LOG(LogTemp, Error, TEXT("AnimNotify_ContextualExecution: missing ASC. Instigator=%s Target=%s."),
			*GetNameSafe(InstigatorActor), *TargetActor->GetName());
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(InstigatorActor, InstigatorActor);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("AnimNotify_ContextualExecution: failed to make effect spec. Effect=%s."),
			*GetNameSafe(DamageEffectClass));
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(SetByCallerDataTag, -Damage);
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

AActor* UAnimNotify_ContextualExecution::ResolveExecutionTarget(AActor* Owner) const
{
	if (!IsValid(Owner)) return nullptr;

	UContextualAnimSceneActorComponent* SceneActorComp =
		Owner->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (!IsValid(SceneActorComp))
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimNotify_ContextualExecution: %s is missing ContextualAnimSceneActorComponent."),
			*Owner->GetName());
		return nullptr;
	}

	const FContextualAnimSceneBindings& Bindings = SceneActorComp->GetBindings();
	if (!Bindings.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("AnimNotify_ContextualExecution: bindings invalid. Owner=%s."),
			*Owner->GetName());
		return nullptr;
	}

	if (const FContextualAnimSceneBinding* RoleBinding = Bindings.FindBindingByRole(TargetRole))
	{
		AActor* RoleActor = RoleBinding->GetActor();
		if (IsValid(RoleActor))
		{
			return RoleActor;
		}
	}

	for (const FContextualAnimSceneBinding& Binding : Bindings.GetBindings())
	{
		AActor* BindingActor = Binding.GetActor();
		if (IsValid(BindingActor) && BindingActor != Owner)
		{
			return BindingActor;
		}
	}

	return nullptr;
}

AActor* UAnimNotify_ContextualExecution::ResolveInstigator(AActor* Owner, AActor* TargetActor) const
{
	if (IsValid(Owner) && Owner != TargetActor)
	{
		return Owner;
	}

	if (!IsValid(Owner)) return TargetActor;

	UContextualAnimSceneActorComponent* SceneActorComp =
		Owner->FindComponentByClass<UContextualAnimSceneActorComponent>();
	if (!IsValid(SceneActorComp)) return Owner;

	const FContextualAnimSceneBindings& Bindings = SceneActorComp->GetBindings();
	for (const FContextualAnimSceneBinding& Binding : Bindings.GetBindings())
	{
		AActor* BindingActor = Binding.GetActor();
		if (IsValid(BindingActor) && BindingActor != TargetActor)
		{
			return BindingActor;
		}
	}

	return Owner;
}
