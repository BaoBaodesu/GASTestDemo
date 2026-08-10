// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTestDemo1/Public/Player/T_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputAction.h"
#include "Blueprint/UserWidget.h"
#include "Components/PanelWidget.h"
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
#include "Blueprint/WidgetTree.h"
#include "UI/T_AttributeWidget.h"

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
	if (!IsValid(ReloadAction))
	{
		ReloadAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/GASTestDemo/Input/AbilitiesActions/IA_Reload.IA_Reload"));
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
	if (IsValid(ReloadAction)) EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ThisClass::Reload);

	EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &ThisClass::StartLockOn);
	EnhancedInputComponent->BindAction(SwitchLockOnAction, ETriggerEvent::Started, this, &ThisClass::SwitchLockOnTarget);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Started, this, &ThisClass::StartCatch);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Completed, this, &ThisClass::StopCatch);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Canceled, this, &ThisClass::StopCatch);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::ToggleCrouch);
	
	
	EnhancedInputComponent->BindAction(ReleaseAction, ETriggerEvent::Started, this, &ThisClass::ReleaseGrab);
	EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Started, this, &ThisClass::PickUp);
	
	if (IsValid(InventoryAction)) EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
	InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &ThisClass::ActivateQuickSlot);
}

void AT_PlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController()) return;

	if (!IsValid(PlayerHUDWidgetClass))
	{
		PlayerHUDWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/GASTestDemo/UI/WBP_PlayerHUD.WBP_PlayerHUD_C"));
	}
	if (!IsValid(PlayerHUDWidgetClass)) return;

	PlayerHUDWidget = CreateWidget<UUserWidget>(this, PlayerHUDWidgetClass);
	if (IsValid(PlayerHUDWidget)) PlayerHUDWidget->AddToViewport();

	BindPlayerStatusWidgets();
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

	StopAim();
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

void AT_PlayerController::ToggleCrouch()
{
	ACharacter* ControlledCharacter = GetCharacter();
	if (!IsValid(ControlledCharacter)) return;

	if (ControlledCharacter->bIsCrouched) ControlledCharacter->UnCrouch();
	else ControlledCharacter->Crouch();
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

void AT_PlayerController::Reload()
{
	if (IsValid(InventoryWidget)) return;
	ActivateAbility(TTags::TAbilities::Reload);
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

void AT_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	BindPlayerStatusWidgets();
}

void AT_PlayerController::BindPlayerStatusWidgets()
{
	if (!IsValid(PlayerHUDWidget)) return;

	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter)) return;

	UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent();
	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet());

	if (!IsValid(ASC) || !IsValid(AttributeSet))
	{
		PlayerCharacter->OnASCInitialized.AddUniqueDynamic(this, &ThisClass::OnPlayerHUDASCInitialized);
		return;
	}

	DoBindHUDWidgets(ASC, AttributeSet, PlayerCharacter);
}

void AT_PlayerController::OnPlayerHUDASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter)) return;

	PlayerCharacter->OnASCInitialized.RemoveDynamic(this, &ThisClass::OnPlayerHUDASCInitialized);

	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(AS);
	if (!IsValid(ASC) || !IsValid(AttributeSet)) return;

	DoBindHUDWidgets(ASC, AttributeSet, PlayerCharacter);
}

void AT_PlayerController::DoBindHUDWidgets(UAbilitySystemComponent* ASC, UT_AttributeSet* AttributeSet, AT_PlayerCharacter* PlayerCharacter)
{
	if (!IsValid(PlayerHUDWidget)) return;

	// 先清理旧绑定，避免重生/重复 Possess 后同一控件被重复绑定
	for (const FGameplayAttribute& Key : BoundHUDAttributeKeys)
	{
		ASC->GetGameplayAttributeValueChangeDelegate(Key).RemoveAll(this);
	}
	BoundHUDAttributeKeys.Reset();

	// 递归遍历：先对控件实例本身（包括 UserWidget 实例）做属性控件匹配，
	// 再下钻 Panel 子控件与 UserWidget 自己的 WidgetTree。
	// 不能只用 ForEachWidgetAndDescendants：引擎实现会跳过带 WidgetTree 的子 UserWidget 实例本身。
	TFunction<void(UWidget*)> VisitWidget = [&](UWidget* Widget)
	{
		if (UT_AttributeWidget* AttrWidget = Cast<UT_AttributeWidget>(Widget))
		{
			const FGameplayAttribute Attribute = AttrWidget->Attribute;
			const FGameplayAttribute MaxAttribute = AttrWidget->MaxAttribute;
			if (Attribute.IsValid() && MaxAttribute.IsValid())
			{
				// 设置 AvatarActor，供伤害数字 Niagar 等使用
				AttrWidget->AvatarActor = PlayerCharacter;
				// 初始刷新
				AttrWidget->OnAttributeChange(TTuple<FGameplayAttribute, FGameplayAttribute>(Attribute, MaxAttribute), AttributeSet, 0.f);
				// 绑定 ASC 属性变化委托
				ASC->GetGameplayAttributeValueChangeDelegate(Attribute)
					.AddUObject(this, &ThisClass::OnHUDWidgetAttributeChanged, TWeakObjectPtr<UT_AttributeWidget>(AttrWidget), Attribute, MaxAttribute);
				BoundHUDAttributeKeys.Add(Attribute);
			}
		}

		if (UPanelWidget* Panel = Cast<UPanelWidget>(Widget))
		{
			for (int32 ChildIndex = 0; ChildIndex < Panel->GetChildrenCount(); ++ChildIndex)
			{
				VisitWidget(Panel->GetChildAt(ChildIndex));
			}
		}
		else if (UUserWidget* UserWidgetChild = Cast<UUserWidget>(Widget))
		{
			if (IsValid(UserWidgetChild->WidgetTree) && IsValid(UserWidgetChild->WidgetTree->RootWidget))
			{
				VisitWidget(UserWidgetChild->WidgetTree->RootWidget);
			}
		}
	};

	if (IsValid(PlayerHUDWidget->WidgetTree) && IsValid(PlayerHUDWidget->WidgetTree->RootWidget))
	{
		VisitWidget(PlayerHUDWidget->WidgetTree->RootWidget);
	}
}

void AT_PlayerController::OnHUDWidgetAttributeChanged(const FOnAttributeChangeData& ChangeData, TWeakObjectPtr<UT_AttributeWidget> Widget, FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute)
{
	UT_AttributeWidget* AttrWidget = Widget.Get();
	if (!IsValid(AttrWidget)) return;

	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter)) return;

	UT_AttributeSet* AttributeSet = Cast<UT_AttributeSet>(PlayerCharacter->GetAttributeSet());
	if (!IsValid(AttributeSet)) return;

	AttrWidget->OnAttributeChange(TTuple<FGameplayAttribute, FGameplayAttribute>(Attribute, MaxAttribute), AttributeSet, ChangeData.OldValue);
}
