// Fill out your copyright notice in the Description page of Project Settings.


#include "GASTestDemo1/Public/Player/T_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/T_AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "InputAction.h"
#include "Components/PanelWidget.h"
#include "Characters/T_BaseCharacter.h"
#include "Characters/T_PlayerCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/TTags.h"
#include "Player/Components/T_AimingComponent.h"
#include "Player/Components/T_LockOnComponent.h"
#include "Player/Components/T_PickUpComponent.h"
#include "Player/Components/T_TraversalComponent.h"
#include "Inventory/T_InventoryComponent.h"
#include "UI/Inventory/T_InventoryWidgets.h"
#include "Blueprint/WidgetTree.h"
#include "UI/T_AttributeWidget.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Quest/T_QuestGameState.h"
#include "TimerManager.h"
#include "UI/Quest/T_GameMenuWidget.h"
#include "UI/Quest/T_LastChanceWidget.h"
#include "UI/Quest/T_QuestWidget.h"

namespace
{
	void RestoreGameplayMouseLook(APlayerController* PlayerController)
	{
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController()) return;
		PlayerController->bShowMouseCursor = false;
		PlayerController->SetIgnoreLookInput(false);
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		PlayerController->SetInputMode(InputMode);
		if (FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace
{
	enum class ECatchMovementModeAction : uint8
	{
		None,
		StartCatch,
		StopCatch
	};

	ECatchMovementModeAction GetCatchMovementModeAction(EMovementMode MovementMode)
	{
		if (MovementMode == MOVE_Falling) return ECatchMovementModeAction::StartCatch;
		if (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking) return ECatchMovementModeAction::StopCatch;
		return ECatchMovementModeAction::None;
	}
}

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
	if (!IsValid(QuestAction))
	{
		QuestAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/GASTestDemo/Input/AbilitiesActions/IA_Quest.IA_Quest"));
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
	EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &ThisClass::StopPrimary);
	EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Canceled, this, &ThisClass::StopPrimary);

	EnhancedInputComponent->BindAction(LockOnAction, ETriggerEvent::Started, this, &ThisClass::StartLockOn);
	EnhancedInputComponent->BindAction(SwitchLockOnAction, ETriggerEvent::Started, this, &ThisClass::SwitchLockOnTarget);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Started, this, &ThisClass::StartCatch);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Completed, this, &ThisClass::StopCatch);
	EnhancedInputComponent->BindAction(CatchAction, ETriggerEvent::Canceled, this, &ThisClass::StopCatch);
	EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::ToggleCrouch);
	
	
	EnhancedInputComponent->BindAction(ReleaseAction, ETriggerEvent::Started, this, &ThisClass::ReleaseGrab);
	EnhancedInputComponent->BindAction(PickUpAction, ETriggerEvent::Started, this, &ThisClass::PickUp);
	
	if (IsValid(InventoryAction)) EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
	if (IsValid(QuestAction)) EnhancedInputComponent->BindAction(QuestAction, ETriggerEvent::Started, this, &ThisClass::ToggleQuestUI);
	FInputKeyBinding& EscapeBinding = InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::ToggleGameMenu);
	EscapeBinding.bExecuteWhenPaused = true;
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
	if (IsValid(PlayerHUDWidgetClass))
	{
		PlayerHUDWidget = CreateWidget<UUserWidget>(this, PlayerHUDWidgetClass);
		if (IsValid(PlayerHUDWidget)) PlayerHUDWidget->AddToViewport();
	}
	if (!IsValid(QuestWidgetClass)) QuestWidgetClass = LoadClass<UT_QuestWidget>(nullptr, TEXT("/Game/GASTestDemo/UI/WBP_QuestUI.WBP_QuestUI_C"));
	if (!IsValid(GameMenuWidgetClass)) GameMenuWidgetClass = LoadClass<UT_GameMenuWidget>(nullptr, TEXT("/Game/GASTestDemo/UI/WBP_GameMenu.WBP_GameMenu_C"));

	BindPlayerStatusWidgets();
	BindQuestState();
	RestoreGameplayMouseLook(this);
}

void AT_PlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(LastChanceTimerHandle);
	if (IsValid(BoundQuestGameState)) BoundQuestGameState->OnQuestStateChanged.RemoveDynamic(this, &ThisClass::HandleQuestStateChanged);
	Super::EndPlay(EndPlayReason);
}

void AT_PlayerController::EnsureQuestWidget()
{
	if (IsValid(QuestWidget) || !IsValid(QuestWidgetClass)) return;
	QuestWidget = CreateWidget<UT_QuestWidget>(this, QuestWidgetClass);
	if (!IsValid(QuestWidget)) return;
	QuestWidget->AddToViewport(20);
	QuestWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AT_PlayerController::SetQuestWidgetVisible(bool bVisible)
{
	EnsureQuestWidget();
	if (!IsValid(QuestWidget)) return;
	QuestWidget->InitializeQuest(Cast<AT_PlayerCharacter>(GetPawn()));
	QuestWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void AT_PlayerController::ToggleQuestUI()
{
	if (IsValid(GameMenuWidget) && GameMenuWidget->IsVisible()) return;
	AT_QuestGameState* QuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr;
	if (!IsValid(QuestGameState) || QuestGameState->GetQuestOutcome() != EQuestOutcome::InProgress || !IsValid(QuestWidgetClass)) return;

	EnsureQuestWidget();
	if (!IsValid(QuestWidget)) return;
	SetQuestWidgetVisible(!QuestWidget->IsVisible());
}

void AT_PlayerController::ToggleGameMenu()
{
	AT_QuestGameState* QuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr;
	if (IsValid(QuestGameState) && QuestGameState->GetQuestOutcome() != EQuestOutcome::InProgress) return;
	if (IsValid(GameMenuWidget) && GameMenuWidget->IsVisible()) CloseGameMenu();
	else OpenGameMenu(static_cast<uint8>(ETGameMenuMode::Pause));
}

void AT_PlayerController::OpenGameMenu(uint8 MenuMode)
{
	if (!IsLocalController() || !IsValid(GameMenuWidgetClass)) return;
	if (IsValid(InventoryWidget)) CloseInventory();
	if (IsValid(QuestWidget)) QuestWidget->SetVisibility(ESlateVisibility::Collapsed);

	if (!IsValid(GameMenuWidget))
	{
		GameMenuWidget = CreateWidget<UT_GameMenuWidget>(this, GameMenuWidgetClass);
		if (!IsValid(GameMenuWidget)) return;
		GameMenuWidget->AddToViewport(50);
	}
	GameMenuWidget->SetMenuMode(static_cast<ETGameMenuMode>(MenuMode), GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr);
	GameMenuWidget->SetVisibility(ESlateVisibility::Visible);
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(GameMenuWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	SetPause(true);
}

void AT_PlayerController::CloseGameMenu()
{
	if (!IsValid(GameMenuWidget) || GameMenuWidget->GetMenuMode() != ETGameMenuMode::Pause) return;
	GameMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	bShowMouseCursor = false;
	SetInputMode(FInputModeGameOnly());
	SetPause(false);
}

void AT_PlayerController::HandleGameMenuContinue()
{
	if (!IsValid(GameMenuWidget)) return;
	if (GameMenuWidget->GetMenuMode() == ETGameMenuMode::Victory) GameMenuWidget->SetNoMoreLevelsMessage();
	else if (GameMenuWidget->GetMenuMode() == ETGameMenuMode::Pause) CloseGameMenu();
}

void AT_PlayerController::RestartQuestLevel()
{
	RestoreGameplayMouseLook(this);
	SetPause(false);
	if (IsValid(GameMenuWidget))
	{
		GameMenuWidget->RemoveFromParent();
		GameMenuWidget = nullptr;
	}
	const FString LevelName = UGameplayStatics::GetCurrentLevelName(this, true);
	UGameplayStatics::OpenLevel(this, FName(*LevelName));
}

void AT_PlayerController::QuitQuestGame()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}

void AT_PlayerController::BindQuestState()
{
	if (IsValid(BoundQuestGameState)) BoundQuestGameState->OnQuestStateChanged.RemoveDynamic(this, &ThisClass::HandleQuestStateChanged);
	BoundQuestGameState = GetWorld() ? GetWorld()->GetGameState<AT_QuestGameState>() : nullptr;
	if (!IsValid(BoundQuestGameState)) return;
	BoundQuestGameState->OnQuestStateChanged.AddUniqueDynamic(this, &ThisClass::HandleQuestStateChanged);
	HandleQuestStateChanged();
}

void AT_PlayerController::HandleQuestStateChanged()
{
	if (!IsLocalController() || !IsValid(BoundQuestGameState)) return;
	if (BoundQuestGameState->GetQuestOutcome() == EQuestOutcome::Victory)
	{
		if (IsValid(QuestWidget)) QuestWidget->SetVisibility(ESlateVisibility::Collapsed);
		OpenGameMenu(static_cast<uint8>(ETGameMenuMode::Victory));
	}
	else if (BoundQuestGameState->GetQuestOutcome() == EQuestOutcome::Failure)
	{
		if (IsValid(QuestWidget)) QuestWidget->SetVisibility(ESlateVisibility::Collapsed);
		OpenGameMenu(static_cast<uint8>(ETGameMenuMode::Failure));
	}
	else if (BoundQuestGameState->GetQuestOutcome() == EQuestOutcome::InProgress && !IsValid(QuestWidget))
	{
		SetQuestWidgetVisible(true);
	}
}

void AT_PlayerController::ClientShowLastChance_Implementation()
{
	if (!IsValid(LastChanceWidget))
	{
		LastChanceWidget = CreateWidget<UT_LastChanceWidget>(this, UT_LastChanceWidget::StaticClass());
		if (IsValid(LastChanceWidget)) LastChanceWidget->AddToViewport(40);
	}
	if (IsValid(LastChanceWidget)) LastChanceWidget->SetVisibility(ESlateVisibility::Visible);
	GetWorldTimerManager().ClearTimer(LastChanceTimerHandle);
	GetWorldTimerManager().SetTimer(LastChanceTimerHandle, this, &ThisClass::HideLastChance, 3.f, false);
}

void AT_PlayerController::HideLastChance()
{
	if (IsValid(LastChanceWidget)) LastChanceWidget->SetVisibility(ESlateVisibility::Collapsed);
}

void AT_PlayerController::ToggleInventory()
{
	if (IsValid(InventoryWidget)) CloseInventory();
	else OpenInventory();
}

void AT_PlayerController::OpenInventory(UT_InventoryComponent* StorageInventory)
{
	if (IsValid(GameMenuWidget) && GameMenuWidget->IsVisible()) return;
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetPawn());
	if (!IsValid(PlayerCharacter) || !IsValid(PlayerCharacter->GetInventoryComponent()) || !IsValid(InventoryWidgetClass)) return;

	if (!IsValid(InventoryWidget))
	{
		InventoryWidget = CreateWidget<UT_InventoryWidget>(this, InventoryWidgetClass);
		if (!IsValid(InventoryWidget)) return;
		InventoryWidget->AddToViewport(30);
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

void AT_PlayerController::StopPrimary()
{
	ReleaseAbility(TTags::TAbilities::Primary);
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
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetCharacter());
	if (!IsValid(PlayerCharacter)) return;

	PlayerCharacter->SetRunInputHeld(true);
	if (PlayerCharacter->GetCharacterMovement()->IsFalling()) SendCatchEvent();
}

void AT_PlayerController::StopCatch()
{
	if (AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetCharacter())) PlayerCharacter->SetRunInputHeld(false);
	SendPlayerGameplayEvent(TTags::Events::Player::Grab::StopCatch);
}

void AT_PlayerController::HandleCatchMovementModeChanged(EMovementMode MovementMode)
{
	AT_PlayerCharacter* PlayerCharacter = Cast<AT_PlayerCharacter>(GetCharacter());
	if (!IsValid(PlayerCharacter) || !PlayerCharacter->IsRunInputHeld()) return;

	const ECatchMovementModeAction Action = GetCatchMovementModeAction(MovementMode);
	if (Action == ECatchMovementModeAction::StartCatch) SendCatchEvent();
	else if (Action == ECatchMovementModeAction::StopCatch) SendPlayerGameplayEvent(TTags::Events::Player::Grab::StopCatch);
	PlayerCharacter->RefreshNormalMovementSpeed();
}

void AT_PlayerController::CancelRunAndCatch()
{
	StopCatch();
}

void AT_PlayerController::SendCatchEvent()
{
	SendPlayerGameplayEvent(TTags::Events::Player::Grab::Catch);
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
	RestoreGameplayMouseLook(this);
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

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTCatchMovementModeActionTest,
	"GASTestDemo1.Grab.CatchMovementModeAction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTCatchMovementModeActionTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("进入 Falling 时开始 Catch 检测"), GetCatchMovementModeAction(MOVE_Falling) == ECatchMovementModeAction::StartCatch);
	TestTrue(TEXT("落地 Walking 时停止 Catch 检测"), GetCatchMovementModeAction(MOVE_Walking) == ECatchMovementModeAction::StopCatch);
	TestTrue(TEXT("落地 NavWalking 时停止 Catch 检测"), GetCatchMovementModeAction(MOVE_NavWalking) == ECatchMovementModeAction::StopCatch);
	TestTrue(TEXT("抓取过渡 Flying 时保持 Catch 状态"), GetCatchMovementModeAction(MOVE_Flying) == ECatchMovementModeAction::None);
	TestTrue(TEXT("其他自定义移动模式不干预 Catch"), GetCatchMovementModeAction(MOVE_Custom) == ECatchMovementModeAction::None);
	return true;
}
#endif
