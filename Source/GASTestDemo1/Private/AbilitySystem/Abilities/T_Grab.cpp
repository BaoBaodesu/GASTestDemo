#include "AbilitySystem/Abilities/T_Grab.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/TTags.h"

UT_Grab::UT_Grab()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	SetAssetTags(FGameplayTagContainer(TTags::TAbilities::Grab.GetTag()));

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TTags::Events::Player::Grab::Catch;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UT_Grab::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bGrabTagsApplied = false;
	bHasGrabStarted = false;
	bEndingFromGrabComponent = false;
	bEndingAbility = false;
	CurrentGrabTypeTag = FGameplayTag();

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	GrabComponent = AvatarActor->FindComponentByClass<UT_GrabComponent>();
	if (!IsValid(GrabComponent)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) { EndAbility(Handle, ActorInfo, ActivationInfo, true, true); return; }

	GrabComponent->OnGrabStarted.AddUObject(this, &ThisClass::HandleGrabStarted);
	GrabComponent->OnGrabEnded.AddUObject(this, &ThisClass::HandleGrabEnded);
	CreateEventTasks();
	GrabComponent->StartGrabCheck();
}

void UT_Grab::CreateEventTasks()
{
	StopCatchTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TTags::Events::Player::Grab::StopCatch, nullptr, false, true);
	ReleaseTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TTags::Events::Player::Grab::Release, nullptr, false, true);
	MoveTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TTags::Events::Player::Grab::Move, nullptr, false, true);
	JumpTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, TTags::Events::Player::Grab::Jump, nullptr, false, true);

	if (IsValid(StopCatchTask)) { StopCatchTask->EventReceived.AddDynamic(this, &ThisClass::HandleStopCatch); StopCatchTask->ReadyForActivation(); }
	if (IsValid(ReleaseTask)) { ReleaseTask->EventReceived.AddDynamic(this, &ThisClass::HandleRelease); ReleaseTask->ReadyForActivation(); }
	if (IsValid(MoveTask)) { MoveTask->EventReceived.AddDynamic(this, &ThisClass::HandleMove); MoveTask->ReadyForActivation(); }
	if (IsValid(JumpTask)) { JumpTask->EventReceived.AddDynamic(this, &ThisClass::HandleJump); JumpTask->ReadyForActivation(); }
}

void UT_Grab::EndEventTasks()
{
	if (IsValid(StopCatchTask)) StopCatchTask->EndTask();
	if (IsValid(ReleaseTask)) ReleaseTask->EndTask();
	if (IsValid(MoveTask)) MoveTask->EndTask();
	if (IsValid(JumpTask)) JumpTask->EndTask();
	StopCatchTask = nullptr;
	ReleaseTask = nullptr;
	MoveTask = nullptr;
	JumpTask = nullptr;
}

void UT_Grab::HandleGrabStarted(ET_GrabType GrabType)
{
	if (bHasGrabStarted) return;

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!IsValid(AbilitySystemComponent)) { CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true); return; }

	bHasGrabStarted = true;
	bGrabTagsApplied = true;
	CurrentGrabTypeTag = GrabType == ET_GrabType::Bar ? TTags::State::Grab::Bar : TTags::State::Grab::Wall;
	AbilitySystemComponent->AddLooseGameplayTag(TTags::State::Action::Grabbing);
	AbilitySystemComponent->AddLooseGameplayTag(TTags::State::Action::Busy);
	AbilitySystemComponent->AddLooseGameplayTag(CurrentGrabTypeTag);
}

void UT_Grab::HandleGrabEnded()
{
	if (bEndingAbility) return;
	bEndingFromGrabComponent = true;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_Grab::HandleStopCatch(FGameplayEventData Payload)
{
	if (!IsValid(GrabComponent)) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return; }
	GrabComponent->StopGrabCheck();
	if (!bHasGrabStarted) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UT_Grab::HandleRelease(FGameplayEventData Payload)
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsGrabbed()) { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); return; }
	GrabComponent->Detach();
}

void UT_Grab::HandleMove(FGameplayEventData Payload)
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsGrabbed()) return;
	GrabComponent->Shimmy(Payload.EventMagnitude);
}

void UT_Grab::HandleJump(FGameplayEventData Payload)
{
	if (!IsValid(GrabComponent) || !GrabComponent->IsGrabbed()) return;
	if (GrabComponent->IsOnBar()) GrabComponent->BarJump();
	else GrabComponent->LedgeJump();
}

void UT_Grab::RemoveGrabTags()
{
	if (!bGrabTagsApplied) return;

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(TTags::State::Action::Grabbing);
		AbilitySystemComponent->RemoveLooseGameplayTag(TTags::State::Action::Busy);
		if (CurrentGrabTypeTag.IsValid()) AbilitySystemComponent->RemoveLooseGameplayTag(CurrentGrabTypeTag);
	}

	bGrabTagsApplied = false;
	CurrentGrabTypeTag = FGameplayTag();
}

void UT_Grab::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEndingAbility) return;
	bEndingAbility = true;

	if (IsValid(GrabComponent))
	{
		GrabComponent->StopGrabCheck();
		GrabComponent->OnGrabStarted.RemoveAll(this);
		GrabComponent->OnGrabEnded.RemoveAll(this);
		if (!bEndingFromGrabComponent && GrabComponent->IsGrabbed()) GrabComponent->Detach();
	}

	EndEventTasks();
	RemoveGrabTags();
	GrabComponent = nullptr;
	bHasGrabStarted = false;
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	bEndingFromGrabComponent = false;
	bEndingAbility = false;
}
