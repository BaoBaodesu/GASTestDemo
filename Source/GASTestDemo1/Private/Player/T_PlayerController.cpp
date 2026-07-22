// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTestDemo1/Public/Player/T_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Characters/T_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameplayTags/TTags.h"
#include "Player/Components/T_LockOnComponent.h"
#include "Player/Components/T_TraversalComponent.h"

void AT_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
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
	
	EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &ThisClass::StartLockOn);
	EnhancedInputComponent->BindAction(SwitchLockOnAction, ETriggerEvent::Started, this, &ThisClass::SwitchLockOnTarget);
}

void AT_PlayerController::Jump()
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (!IsValid(ControlledCharacter)) return;
	if (!IsAlive()) return;
	
	UT_TraversalComponent* TraversalComponent =
		ControlledCharacter->FindComponentByClass<UT_TraversalComponent>();
	if (IsValid(TraversalComponent) && TraversalComponent->Jump())
	{
		return;
	}
	
	ControlledCharacter->Jump();
}

void AT_PlayerController::StopJumping()
{
	if (!IsValid(GetCharacter())) return;

	GetCharacter()->StopJumping();
}

void AT_PlayerController::Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;
	if (!IsAlive()) return;
	
	MovementVector = Value.Get<FVector2D>();
	
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
}

void AT_PlayerController::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AT_PlayerController::Primary()
{
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
