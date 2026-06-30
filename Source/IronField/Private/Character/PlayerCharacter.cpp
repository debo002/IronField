#include "Character/PlayerCharacter.h"
#include "Camera/CameraComponent.h"
#include "Stats/HealthComponent.h"
#include "Combat/PlayerCombatComponent.h"
#include "Stats/StaminaComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

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
    CameraBoom->bEnableCameraLag = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    bUseControllerRotationYaw = true;
    GetCharacterMovement()->bOrientRotationToMovement = false;
}

bool AIFPlayerCharacter::IsBlocking() const
{
    const UIFCombatComponent* const Combat = GetCombatComponent();
    if (!Combat)
    {
        return false;
    }

    return Combat->IsBlocking();
}

float AIFPlayerCharacter::GetHealthPercent() const
{
    const UIFHealthComponent* const Health = GetHealthComponent();
    if (!Health)
    {
        return 0.f;
    }

    return Health->GetHealthPercent();
}

bool AIFPlayerCharacter::IsDead() const
{
    const UIFHealthComponent* const Health = GetHealthComponent();
    if (!Health)
    {
        return false;
    }

    return Health->IsDead();
}

float AIFPlayerCharacter::GetStaminaPercent() const
{
    const UIFStaminaComponent* const Stamina = GetStaminaComponent();
    if (!Stamina)
    {
        return 0.f;
    }

    return Stamina->GetStaminaPercent();
}

bool AIFPlayerCharacter::IsAttacking() const
{
    const UIFCombatComponent* const Combat = GetCombatComponent();
    if (!Combat)
    {
        return false;
    }

    return Combat->IsAttacking();
}

void AIFPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (UIFCombatComponent* const Combat = GetCombatComponent())
    {
        Combat->OnCombatStateChanged.AddDynamic(this, &AIFPlayerCharacter::HandleCombatStateChanged);
    }

    UpdateMovementSpeed();
}

void AIFPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (GetVelocity().SizeSquared2D() <= 100.f || !HasSprintInput())
    {
        StopSprint();
    }

    TickCameraTransition(DeltaTime);
}

void AIFPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    StopSprint();

    if (UIFCombatComponent* const Combat = GetCombatComponent())
    {
        Combat->OnCombatStateChanged.RemoveAll(this);
    }
}

void AIFPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (DefaultInputMappingContext)
    {
        if (APlayerController* const PlayerController = Cast<APlayerController>(GetController()))
        {
            if (UEnhancedInputLocalPlayerSubsystem* const InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
            {
                InputSubsystem->AddMappingContext(DefaultInputMappingContext, 0);
            }
        }
    }

    if (UEnhancedInputComponent* const EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveInputAction)
        {
            EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Triggered, this, &AIFPlayerCharacter::Move);
            EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopMoving);
            EnhancedInputComponent->BindAction(MoveInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopMoving);
        }

        if (LookInputAction)
        {
            EnhancedInputComponent->BindAction(LookInputAction, ETriggerEvent::Triggered, this, &AIFPlayerCharacter::Look);
        }

        if (JumpInputAction)
        {
            EnhancedInputComponent->BindAction(JumpInputAction, ETriggerEvent::Started, this, &ACharacter::Jump);
            EnhancedInputComponent->BindAction(JumpInputAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
        }

        if (SprintInputAction)
        {
            EnhancedInputComponent->BindAction(SprintInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::StartSprint);
            EnhancedInputComponent->BindAction(SprintInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopSprint);
            EnhancedInputComponent->BindAction(SprintInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopSprint);
        }

        if (BlockInputAction)
        {
            EnhancedInputComponent->BindAction(BlockInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::StartBlock);
            EnhancedInputComponent->BindAction(BlockInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopBlock);
            EnhancedInputComponent->BindAction(BlockInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopBlock);
        }

        if (AttackInputAction)
        {
            EnhancedInputComponent->BindAction(AttackInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::Attack);
        }

        if (SpinAttackInputAction)
        {
            EnhancedInputComponent->BindAction(SpinAttackInputAction, ETriggerEvent::Started, this, &AIFPlayerCharacter::StartSpinAttack);
            EnhancedInputComponent->BindAction(SpinAttackInputAction, ETriggerEvent::Completed, this, &AIFPlayerCharacter::StopSpinAttack);
            EnhancedInputComponent->BindAction(SpinAttackInputAction, ETriggerEvent::Canceled, this, &AIFPlayerCharacter::StopSpinAttack);
        }
    }
}

void AIFPlayerCharacter::OnDeathStarted()
{
    StopSprint();
    bIsCameraTransitioning = true;
    bUseControllerRotationYaw = false;
    UpdateTickEnabled();
}

void AIFPlayerCharacter::OnStaminaDepleted()
{
    StopSprint();
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
    if (!Controller || IsDead())
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
    if (bIsSprinting)
    {
        return;
    }

    const UIFCombatComponent* const Combat = GetCombatComponent();
    if (!Combat || Combat->GetCombatState() != ECombatState::Idle)
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

void AIFPlayerCharacter::Attack()
{
    if (IsDead())
    {
        return;
    }

    StopSprint();

    if (UIFCombatComponent* const Combat = GetCombatComponent())
    {
        Combat->StartAttack();
    }
}

void AIFPlayerCharacter::StartBlock()
{
    if (IsDead())
    {
        return;
    }

    StopSprint();

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
    if (IsDead())
    {
        return;
    }

    StopSprint();

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

void AIFPlayerCharacter::HandleCombatStateChanged(ECombatState /*PreviousState*/, ECombatState /*NewState*/)
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
    const UIFCombatComponent* const Combat = GetCombatComponent();
    if (Combat && Combat->IsAttacking())
    {
        return AttackMoveSpeed;
    }

    if (Combat && Combat->IsBlocking())
    {
        return BlockSpeed;
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

