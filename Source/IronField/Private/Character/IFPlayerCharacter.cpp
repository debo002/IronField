#include "Character/IFPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Character/IFCharacterSetupUtils.h"
#include "Combat/IFCombatComponent.h"
#include "Combat/IFPlayerCombatComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/IFPlayerSubsystem.h"
#include "Stats/IFHealthComponent.h"
#include "Stats/IFStaminaComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"

AIFPlayerCharacter::AIFPlayerCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UIFPlayerCombatComponent>(TEXT("Combat")))
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = NormalCameraArmLength;
	CameraBoom->SocketOffset = NormalCameraSocketOffset;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	CameraBoom->SetRelativeRotation(FRotator(CameraBoomPitch, 0.f, 0.f));
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = CameraLagSpeed;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = CameraRotationLagSpeed;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;

	WeaponCollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("WeaponCollision"));
	IFCharacterSetupUtils::ConfigureWeaponCollisionBox(WeaponCollisionBox, GetMesh());

	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sword"));
	IFCharacterSetupUtils::ConfigureCosmeticEquipmentMesh(SwordMesh, GetMesh(), TEXT("weapon_r"));

	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Shield"));
	IFCharacterSetupUtils::ConfigureCosmeticEquipmentMesh(ShieldMesh, GetMesh(), TEXT("weapon_l"));
}

float AIFPlayerCharacter::GetHealthPercent() const
{
	const UIFHealthComponent* const Health = GetHealthComponent();
	return Health ? Health->GetHealthPercent() : 0.f;
}

float AIFPlayerCharacter::GetStaminaPercent() const
{
	const UIFStaminaComponent* const Stamina = GetStaminaComponent();
	return Stamina ? Stamina->GetStaminaPercent() : 0.f;
}

void AIFPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* const World = GetWorld())
	{
		if (UIFPlayerSubsystem* const Subsystem = World->GetSubsystem<UIFPlayerSubsystem>())
		{
			Subsystem->RegisterPlayer(this);
		}
	}

	if (UIFCombatComponent* const Combat = GetCombatComponent())
	{
		Combat->OnCombatStateChanged.AddDynamic(this, &AIFPlayerCharacter::HandleCombatStateChanged);
	}

	UpdateMovementSpeed();
}

void AIFPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetVelocity().SizeSquared2D() <= SprintExitSpeedSquared || !HasSprintInput())
	{
		StopSprint();
	}

	TickCameraTransition(DeltaTime);
}

void AIFPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearReviveTimers();
	StopSprint();

	if (UIFCombatComponent* const Combat = GetCombatComponent())
	{
		Combat->OnCombatStateChanged.RemoveDynamic(this, &AIFPlayerCharacter::HandleCombatStateChanged);
	}

	if (UWorld* const World = GetWorld())
	{
		if (UIFPlayerSubsystem* const Subsystem = World->GetSubsystem<UIFPlayerSubsystem>())
		{
			Subsystem->UnregisterPlayer(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AIFPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (DefaultInputMappingContext)
	{
		if (APlayerController* const PlayerController = Cast<APlayerController>(GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* const InputSubsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				InputSubsystem->AddMappingContext(DefaultInputMappingContext, 0);
			}
		}
	}

	UEnhancedInputComponent* const EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		return;
	}

	if (MoveInputAction)
	{
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AIFPlayerCharacter::Move);
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopMoving);
		EnhancedInput->BindAction(MoveInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopMoving);
	}

	if (LookInputAction)
	{
		EnhancedInput->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &AIFPlayerCharacter::Look);
	}

	if (JumpInputAction)
	{
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::Jump);
		EnhancedInput->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}

	if (SprintInputAction)
	{
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::StartSprint);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopSprint);
		EnhancedInput->BindAction(SprintInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopSprint);
	}

	if (BlockInputAction)
	{
		EnhancedInput->BindAction(BlockInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::StartBlock);
		EnhancedInput->BindAction(BlockInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopBlock);
		EnhancedInput->BindAction(BlockInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopBlock);
	}

	if (AttackInputAction)
	{
		EnhancedInput->BindAction(AttackInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::Attack);
	}

	if (SpinAttackInputAction)
	{
		EnhancedInput->BindAction(SpinAttackInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::StartSpinAttack);
		EnhancedInput->BindAction(SpinAttackInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopSpinAttack);
		EnhancedInput->BindAction(SpinAttackInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopSpinAttack);
	}
}

void AIFPlayerCharacter::Jump()
{
	if (IsDead() || IsGettingUp())
	{
		return;
	}

	Super::Jump();
}

void AIFPlayerCharacter::OnDeathStarted()
{
	StopSprint();
	bIsCameraTransitioning = true;
	bUseControllerRotationYaw = false;
	UpdateTickEnabled();
}

void AIFPlayerCharacter::OnDeathSequenceStarted()
{
	StartReviveTimer();
}

void AIFPlayerCharacter::OnStaminaDepleted()
{
	StopSprint();
}

void AIFPlayerCharacter::ClearReviveTimers()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReviveTimerHandle);
		World->GetTimerManager().ClearTimer(GetUpFallbackTimerHandle);
	}
}

void AIFPlayerCharacter::StartReviveTimer()
{
	UWorld* const World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().SetTimer(ReviveTimerHandle, this, &AIFPlayerCharacter::AttemptRevive, ReviveDelaySeconds, false);
}

void AIFPlayerCharacter::AttemptRevive()
{
	UIFHealthComponent* const Health = GetHealthComponent();
	UIFCombatComponent* const Combat = GetCombatComponent();
	if (!Health || !Combat)
	{
		return;
	}

	Health->Revive();
	Health->SetInvincible(true);
	Combat->HandleOwnerRevived();
	RestoreCollisionAfterDeath();

	bIsGettingUp = true;

	// AnimBP should call NotifyGetUpFinished; this timer prevents a permanent lockout if that notify is missing.
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().SetTimer(GetUpFallbackTimerHandle, this, &AIFPlayerCharacter::NotifyGetUpFinished, GetUpDuration, false);
	}
}

void AIFPlayerCharacter::NotifyGetUpFinished()
{
	if (UWorld* const World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GetUpFallbackTimerHandle);
	}

	if (bIsGettingUp)
	{
		CompleteRevive();
	}
}

void AIFPlayerCharacter::CompleteRevive()
{
	bIsGettingUp = false;

	if (UIFHealthComponent* const Health = GetHealthComponent())
	{
		Health->SetInvincible(false);
	}

	OnReviveFinished();
}

void AIFPlayerCharacter::OnReviveFinished()
{
	bIsCameraTransitioning = true;
	bUseControllerRotationYaw = true;
	UpdateTickEnabled();
	UpdateMovementSpeed();
}

void AIFPlayerCharacter::TickCameraTransition(float DeltaTime)
{
	if (!bIsCameraTransitioning)
	{
		return;
	}

	const float TargetArmLength = IsDead() ? DeathCameraArmLength : NormalCameraArmLength;
	const FVector TargetSocketOffset = IsDead() ? DeathCameraSocketOffset : NormalCameraSocketOffset;

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetArmLength, DeltaTime, CameraTransitionInterpSpeed);
	CameraBoom->SocketOffset = FMath::VInterpTo(CameraBoom->SocketOffset, TargetSocketOffset, DeltaTime, CameraTransitionInterpSpeed);

	const bool bArmLengthReached = FMath::IsNearlyEqual(CameraBoom->TargetArmLength, TargetArmLength, 0.5f);
	const bool bSocketOffsetReached = CameraBoom->SocketOffset.Equals(TargetSocketOffset, 0.5f);

	if (bArmLengthReached && bSocketOffsetReached)
	{
		CameraBoom->TargetArmLength = TargetArmLength;
		CameraBoom->SocketOffset = TargetSocketOffset;
		bIsCameraTransitioning = false;
		UpdateTickEnabled();
	}
}

void AIFPlayerCharacter::UpdateTickEnabled()
{
	SetActorTickEnabled(bIsSprinting || bIsCameraTransitioning);
}

void AIFPlayerCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller || IsDead() || IsGettingUp())
	{
		return;
	}

	CachedMovementInput = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FRotationMatrix YawMatrix(YawRotation);

	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::X), CachedMovementInput.Y);
	AddMovementInput(YawMatrix.GetUnitAxis(EAxis::Y), CachedMovementInput.X);

	UpdateMovementSpeed();
}

void AIFPlayerCharacter::StopMoving()
{
	CachedMovementInput = FVector2D::ZeroVector;
	StopSprint();
}

void AIFPlayerCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookInput = Value.Get<FVector2D>();
	AddControllerYawInput(LookInput.X);
	AddControllerPitchInput(LookInput.Y);
}

void AIFPlayerCharacter::StartSprint()
{
	if (bIsSprinting || IsGettingUp())
	{
		return;
	}

	const UIFCombatComponent* const Combat = GetCombatComponent();
	if (!Combat || !Combat->IsIdle())
	{
		return;
	}

	UIFStaminaComponent* const Stamina = GetStaminaComponent();
	if (!Stamina || !Stamina->HasStamina(MinimumStaminaToStartSprint))
	{
		return;
	}

	bIsSprinting = true;
	Stamina->StartContinuousDrain(SprintStaminaDrainRate);
	UpdateTickEnabled();
	UpdateMovementSpeed();
}

void AIFPlayerCharacter::StopSprint()
{
	if (!bIsSprinting)
	{
		return;
	}

	bIsSprinting = false;

	if (UIFStaminaComponent* const Stamina = GetStaminaComponent())
	{
		Stamina->StopContinuousDrain();
	}

	UpdateTickEnabled();
	UpdateMovementSpeed();
}

bool AIFPlayerCharacter::TryPrepareCombatAction()
{
	if (IsDead() || IsGettingUp())
	{
		return false;
	}

	StopSprint();
	return true;
}

void AIFPlayerCharacter::Attack()
{
	if (!TryPrepareCombatAction())
	{
		return;
	}

	if (UIFCombatComponent* const Combat = GetCombatComponent())
	{
		Combat->StartAttack();
	}
}

void AIFPlayerCharacter::StartBlock()
{
	if (!TryPrepareCombatAction())
	{
		return;
	}

	if (UIFCombatComponent* const Combat = GetCombatComponent())
	{
		Combat->StartBlock();
	}
}

void AIFPlayerCharacter::StopBlock()
{
	if (UIFCombatComponent* const Combat = GetCombatComponent())
	{
		Combat->StopBlock();
	}
}

void AIFPlayerCharacter::StartSpinAttack()
{
	if (!TryPrepareCombatAction())
	{
		return;
	}

	if (UIFPlayerCombatComponent* const Combat = GetPlayerCombatComponent())
	{
		Combat->StartSpinAttack();
	}
}

void AIFPlayerCharacter::StopSpinAttack()
{
	if (UIFPlayerCombatComponent* const Combat = GetPlayerCombatComponent())
	{
		Combat->StopSpinAttack();
	}
}

void AIFPlayerCharacter::HandleCombatStateChanged(ECombatState, ECombatState)
{
	UpdateMovementSpeed();
}

void AIFPlayerCharacter::UpdateMovementSpeed()
{
	const float DesiredSpeed = CalculateDesiredMovementSpeed();

	UCharacterMovementComponent* const Movement = GetCharacterMovement();
	if (Movement && !FMath::IsNearlyEqual(Movement->MaxWalkSpeed, DesiredSpeed))
	{
		Movement->MaxWalkSpeed = DesiredSpeed;
	}
}

float AIFPlayerCharacter::CalculateDesiredMovementSpeed() const
{
	if (const UIFCombatComponent* const Combat = GetCombatComponent())
	{
		if (Combat->IsAttacking())
		{
			return AttackMoveSpeed;
		}

		if (Combat->IsBlocking())
		{
			return BlockingSpeed;
		}
	}

	if (CachedMovementInput.Y < BackpedalInputThreshold)
	{
		return BackpedalSpeed;
	}

	if (bIsSprinting && HasSprintInput())
	{
		return SprintSpeed;
	}

	return WalkSpeed;
}

bool AIFPlayerCharacter::HasSprintInput() const
{
	return CachedMovementInput.Y >= SprintInputThreshold;
}

UIFPlayerCombatComponent* AIFPlayerCharacter::GetPlayerCombatComponent() const
{
	return Cast<UIFPlayerCombatComponent>(GetCombatComponent());
}
