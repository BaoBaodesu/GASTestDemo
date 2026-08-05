// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTestDemo1/Public/Player/T_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameplayTags/TTags.h"
#include "Player/Components/T_AimingComponent.h"
#include "Player/Components/T_LockOnComponent.h"
#include "Player/Components/T_PickUpComponent.h"
#include "Player/Components/T_TraversalComponent.h"
#include "Inventory/T_InventoryComponent.h"
#include "UI/Inventory/T_InventoryWidgets.h"

void AT_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (!IsValid(InventoryAction))
	{
		InventoryAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/GASTestDemo/Input/AbilitiesActions/IA_Backpack.IA_Backpack"));
	}
	if (!IsValid(PickUpAction))
	{
		PickUpAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/GASTestDemo/Input/AbilitiesActions/IA_Interactive.IA_Interactive"));
	}
	
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem)) return;

	for (UInputMappingContext* Context : InputMappingContexts)
	{
		InputSubsystem->AddMappingContext(Context, 0);
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJumping);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::StopMove);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	
	EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &ThisClass::Primary);
	EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Started, this, &ThisClass::Secondary);
	EnhancedInputComponent->BindAction(TertiaryAction, ETriggerEvent::Started, this, &ThisClass::Tertiary);
	EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &ThisClass::Roll);
	
	EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::StartAim);

	EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &ThisClass::StartLockOn);
	EnhancedInputComponent->BindAction(SwitchLockOnAction, ETriggerEvent::Started, this, &ThisClass::SwitchLockOnTarget);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Started, this, &ThisClass::StartCatch);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Completed, this, &ThisClass::StopCatch);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Canceled, this, &ThisClass::StopCatch);
	EnhancedInputComponent->BindAction(ReleaseAction, ETriggerEvent::Started, this, &ThisClass::ReleaseGrab);
	EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Started, this, &ThisClass::PickUp);
	if (IsValid(InventoryAction)) EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
}

void AT_PlayerController::ToggleInventory()
{
	if (IsValid(InventoryWidget)) CloseInventory();
	else OpenInventory();
}

void AT_PlayerController::OpenInventory(UT_InventoryComponent* StorageInventory)
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter) || !IsValid(PlayerCharacter->GetInventoryComponent()) || !IsValid(InventoryWidgetClass)) return;

	if (!IsValid(InventoryWidget))
	{
		InventoryWidget = CreateWidget<UT_InventoryWidget>(this, InventoryWidgetClass);
		if (!IsValid(InventoryWidget)) return;
		InventoryWidget->AddToViewport();
	}

	InventoryWidget->InitializeInventory(PlayerCharacter->GetInventoryComponent(), StorageInventory);
	bWasMouseCursorVisible = bShowMouseCursor;
	bShowMouseCursor = true;

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	InventoryWidget->SetKeyboardFocus();
}

void AT_PlayerController::CloseInventory()
{
	if (!IsValid(InventoryWidget)) return;

	InventoryWidget->RemoveFromParent();
	InventoryWidget = nullptr;
	bShowMouseCursor = bWasMouseCursorVisible;
	SetInputMode(FInputModeGameOnly());
}

void AT_PlayerController::PickUp()
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter) || !IsValid(PlayerCharacter->GetPickUpComponent())) return;

	PlayerCharacter->GetPickUpComponent()->TryPickUp();
}

void AT_PlayerController::ActivateQuickSlot(FKey Key)
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter) || !IsValid(PlayerCharacter->GetInventoryComponent())) return;

	const int32 QuickSlotIndex = Key == EKeys::One ? 0 : Key == EKeys::Two ? 1 : Key == EKeys::Three ? 2 : Key == EKeys::Four ? 3 : INDEX_NONE;
	if (QuickSlotIndex != INDEX_NONE) PlayerCharacter->GetInventoryComponent()->ActivateQuickSlot(QuickSlotIndex);
}

void AT_PlayerController::Jump()
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (!IsValid(ControlledCharacter)) return;
	if (!IsAlive()) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledCharacter);
	if (IsValid(ASC) && ASC->HasMatchingGameplayTag(TTags::State::Action::Grabbing)) { SendPlayerGameplayEvent(TTags::Events::Player::Grab::Jump); return; }
	
	UT_TraversalComponent* TraversalComponent = ControlledCharacter->FindComponentByClass<UT_TraversalComponent>();
	if (IsValid(TraversalComponent) && TraversalComponent->Jump()) return;
	
	ControlledCharacter->Jump();
}

void AT_PlayerController::StopJumping()
{
	if (!IsValid(GetCharacter())) return;

	GetCharacter()->StopJumping();
}

void AT_PlayerController::Move(const FInputActionValue& Value)
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (!IsValid(ControlledCharacter)) return;
	if (!IsAlive()) return;

	MovementVector = Value.Get<FVector2D>();

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledCharacter);
	if (IsValid(ASC) && ASC->HasMatchingGameplayTag(TTags::State::Action::Grabbing)) { SendPlayerGameplayEvent(TTags::Events::Player::Grab::Move, MovementVector.X); return; }
	
	// 用来找出哪个方向是前方
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void AT_PlayerController::StopMove()
{
	MovementVector = FVector2D::ZeroVector;
	if (!IsValid(GetPawn())) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (IsValid(ASC) && ASC->HasMatchingGameplayTag(TTags::State::Action::Grabbing)) SendPlayerGameplayEvent(TTags::Events::Player::Grab::Move, 0.0f);
}

void AT_PlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	const UT_AimingComponent* AimingComponent = IsValid(GetPawn()) ? GetPawn()->FindComponentByClass<UT_AimingComponent>() : nullptr;
	const float LookInputMultiplier = IsValid(AimingComponent) ? AimingComponent->GetLookInputMultiplier() : 1.f;

	AddYawInput(LookAxisVector.X * LookInputMultiplier);
	AddPitchInput(LookAxisVector.Y * LookInputMultiplier);
}

void AT_PlayerController::Primary()
{
	if (IsValid(InventoryWidget)) return;
	ActivateAbility(TTags::TAbilities::Primary);
}

void AT_PlayerController::Secondary()
{
	ActivateAbility(TTags::TAbilities::Secondary);
}

void AT_PlayerController::Tertiary()
{
	ActivateAbility(TTags::TAbilities::Tertiary);
}

void AT_PlayerController::StandingDodge()
{
	ActivateAbility(TTags::TAbilities::StandingDodge);
}

void AT_PlayerController::Roll()
{
	ActivateAbility(TTags::TAbilities::Roll);
}

void AT_PlayerController::StartAim()
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return;
	if (ASC->HasMatchingGameplayTag(TTags::State::Aiming)) { StopAim(); return; }

	ActivateAbility(TTags::TAbilities::Aim);
}

void AT_PlayerController::StopAim()
{
	ReleaseAbility(TTags::TAbilities::Aim);
}

void AT_PlayerController::ActivateAbility(const FGameplayTag& AbilityTag) const
{
	if (!IsAlive()) return;
	UAbilitySystemComponent* ASC =  UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return;
	
	if (UT_AbilitySystemComponent* T_ASC = Cast<UT_AbilitySystemComponent>(ASC))
	{
		T_ASC->AbilityInputTagPressed(AbilityTag);
		return;
	}
	
	ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
}

bool AT_PlayerController::IsAlive() const
{
	AT_BaseCharacter* BaseCharacter = Cast<AT_BaseCharacter>(GetPawn());
	if (!IsValid(BaseCharacter)) return false;
	return BaseCharacter->IsAlive();
}

void AT_PlayerController::StartLockOn()
{
	if (!IsAlive()) return;

	ActivateAbility(TTags::TAbilities::LockOn);
	
}

void AT_PlayerController::SwitchLockOnTarget(const FInputActionValue& Value)
{
	if (!IsAlive()) return;

	const float AxisValue = Value.Get<float>();

	if (FMath::IsNearlyZero(AxisValue)) return;

	FGameplayTagContainer AbilityTags;

	if (AxisValue > 0.0f)
	{
		AbilityTags.AddTag(TTags::TAbilities::SwitchLockOnTargetLeft);
	}
	else
	{
		AbilityTags.AddTag(TTags::TAbilities::SwitchLockOnTargetRight);
	}
	UAbilitySystemComponent* ASC =  UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return;	
	ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void AT_PlayerController::ReleaseAbility(const FGameplayTag& AbilityTag) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return;
	if (UT_AbilitySystemComponent* T_ASC = Cast<UT_AbilitySystemComponent>(ASC)) T_ASC->AbilityInputTagReleased(AbilityTag);
}

void AT_PlayerController::StartCatch()
{
	if (!IsAlive()) return;
	SendPlayerGameplayEvent(TTags::Events::Player::Grab::Catch);
}

void AT_PlayerController::StopCatch()
{
	SendPlayerGameplayEvent(TTags::Events::Player::Grab::StopCatch);
}

void AT_PlayerController::ReleaseGrab()
{
	SendPlayerGameplayEvent(TTags::Events::Player::Grab::Release);
}

void AT_PlayerController::SendPlayerGameplayEvent(const FGameplayTag& EventTag, float EventMagnitude) const
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (!IsValid(ControlledCharacter)) return;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = ControlledCharacter;
	Payload.Target = ControlledCharacter;
	Payload.EventMagnitude = EventMagnitude;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ControlledCharacter, EventTag, Payload);
}
