#include "LMSSpectatorPawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

ALMSSpectatorPawn::ALMSSpectatorPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    bOnlyRelevantToOwner = true;
    SetReplicateMovement(false);

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(Root);
    SpringArm->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
    SpringArm->TargetArmLength = 400.f;
    SpringArm->bUsePawnControlRotation = true;
    SpringArm->bDoCollisionTest = true;

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;
}

void ALMSSpectatorPawn::BeginPlay()
{
    Super::BeginPlay();

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                PC->GetLocalPlayer()))
        {
            if (SpectatorMappingContext)
            {
                Subsystem->AddMappingContext(SpectatorMappingContext, 0);
            }
        }
    }
}

// ─────────────── 관전 대상 ───────────────

void ALMSSpectatorPawn::SetSpectateTarget(AActor* NewTarget)  // 서버에서 호출
{
    SpectateTarget = NewTarget;
    AttachToTarget();          // 서버도 즉시 붙임
}

void ALMSSpectatorPawn::OnRep_SpectateTarget()
{
    AttachToTarget();          // 클라도 복제받으면 붙임
}

void ALMSSpectatorPawn::AttachToTarget()
{
    if (!SpectateTarget) return;

    AttachToActor(
        SpectateTarget,
        FAttachmentTransformRules::SnapToTargetNotIncludingScale);
}

// ─────────────── 순환 ───────────────

void ALMSSpectatorPawn::OnNextTarget()  // 클라 로컬에서 눌림
{
    ServerCycleTarget();   // 서버에 요청
}

void ALMSSpectatorPawn::ServerCycleTarget_Implementation()
{
    TArray<AActor*> Allies = GetLivingAllies();
    if (Allies.Num() == 0) return;

    int32 CurrentIndex = Allies.IndexOfByKey(SpectateTarget);
    int32 NextIndex = (CurrentIndex + 1) % Allies.Num();

    SetSpectateTarget(Allies[NextIndex]);
}

TArray<AActor*> ALMSSpectatorPawn::GetLivingAllies() const
{
    TArray<AActor*> Result;

    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS) return Result;

    static const FGameplayTag DeadTag =
        FGameplayTag::RequestGameplayTag(FName("state.Dead"));

    for (APlayerState* PS : GS->PlayerArray)
    {
        if (!PS) continue;

        ACharacter* Char = Cast<ACharacter>(PS->GetPawn());
        if (!Char) continue;

        IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Char);
        if (!ASI) continue;

        UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
        if (!ASC || ASC->HasMatchingGameplayTag(DeadTag)) continue;

        Result.Add(Char);
    }
    return Result;
}

// ─────────────── 입력 ───────────────

void ALMSSpectatorPawn::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();
    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void ALMSSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALMSSpectatorPawn::Look);
        EnhancedInputComponent->BindAction(NextTargetAction, ETriggerEvent::Started, this, &ALMSSpectatorPawn::OnNextTarget);
    }
}

// ─────────────── 복제 ───────────────

void ALMSSpectatorPawn::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALMSSpectatorPawn, SpectateTarget);
}