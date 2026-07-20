// Copyright Epic Games, Inc. All Rights Reserved.

#include "LMS_TeamProjectCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "AbilitySystemComponent.h"
#include "LMSAttributeSet.h"
#include "LMS_TeamProjectPlayerState.h"
#include "LMSGameplayAbility.h"
#include "GameplayEffect.h"
#include "Weapons/LMSWeaponComponent.h"
#include "PingMarker.h"
#include "GameplayTagContainer.h"
#include "UI/IndicatorManagerComponent.h"
#include "UI/LMSCombatHUDPresenterComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// ALMS_TeamProjectCharacter

ALMS_TeamProjectCharacter::ALMS_TeamProjectCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
		
	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	WeaponComponent = CreateDefaultSubobject<ULMSWeaponComponent>(TEXT("WeaponComponent"));

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)




}

UAbilitySystemComponent* ALMS_TeamProjectCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALMS_TeamProjectCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

		InitAbilityActorInfo();
		GiveDefaultAbilities();
		ApplyDefaultEffects();
}

void ALMS_TeamProjectCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라: PlayerState가 복제돼서 도착한 시점
	InitAbilityActorInfo();
}



void ALMS_TeamProjectCharacter::InitAbilityActorInfo()
{
	// PlayerState 가져오기
	ALMS_TeamProjectPlayerState* PS = GetPlayerState<ALMS_TeamProjectPlayerState>();
	if (!PS)
	{
		return;   // 아직 준비 안 됨
	}

	// PlayerState의 ASC/AttributeSet을 캐릭터에 캐싱
	AbilitySystemComponent = PS->GetAbilitySystemComponent();
	AttributeSet = PS->GetAttributeSet();

	// ★ 핵심: Owner=PlayerState, Avatar=이 캐릭터
	AbilitySystemComponent->InitAbilityActorInfo(PS, this);

	// 인디케이터 시스템이 팀원(녹색)/적(빨강)을 구분할 때 사용하는 팀 태그입니다.
	static const FGameplayTag TeamPlayerTag = FGameplayTag::RequestGameplayTag(FName("Team.Player"));
	if (HasAuthority() && !AbilitySystemComponent->HasMatchingGameplayTag(TeamPlayerTag))
	{
		AbilitySystemComponent->AddReplicatedLooseGameplayTag(TeamPlayerTag);
	}

	if (WeaponComponent)
	{
		WeaponComponent->RefreshGrantedAbilities();
	}

	// ★ MoveSpeed 변경 구독
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetSpeedAttribute())
		.AddUObject(this, &ALMS_TeamProjectCharacter::OnSpeedChanged);

	// 초기값 즉시 반영 (구독 전 이미 세팅된 값 대비)
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetSpeed();
	}

	// OnHealthZero는 PostGameplayEffectExecute(서버)에서만 브로드캐스트되므로
	// 서버에서만 바인딩. 클라는 태그 복제로 다운 상태를 표현.
	if (HasAuthority() && AttributeSet)
	{
		// 중복 방지 — 기존 바인딩 제거 후 재바인딩
		AttributeSet->OnHealthZero.RemoveAll(this);
		AttributeSet->OnHealthZero.AddUObject(this, &ALMS_TeamProjectCharacter::HandleHealthZero);
		AttributeSet->OnIncapHealthZero.RemoveAll(this);
		AttributeSet->OnIncapHealthZero.AddUObject(this, &ALMS_TeamProjectCharacter::HandleIncapHealthZero);
	}

}

void ALMS_TeamProjectCharacter::GiveDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		// ★ 어빌리티의 CDO에서 InputID 꺼내기
		const ULMSGameplayAbility* AbilityCDO =
			Cast<ULMSGameplayAbility>(AbilityClass->GetDefaultObject());

		const int32 InputID = AbilityCDO
			? (int32)AbilityCDO->AbilityInputID
			: INDEX_NONE;

		AbilitySystemComponent->GiveAbility(
			FGameplayAbilitySpec(AbilityClass, 1, InputID, this));
	}
}

void ALMS_TeamProjectCharacter::ApplyDefaultEffects()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	for (const TSubclassOf<UGameplayEffect>& EffectClass : DefaultEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Context);

		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}

	}
}

void ALMS_TeamProjectCharacter::OnSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
	}
}

void ALMS_TeamProjectCharacter::OnAbilityInputPressed(ELMSAbilityInputID InputID)
{
	UE_LOG(LogTemplateCharacter, Log, TEXT("Ability input pressed: %d"), static_cast<int32>(InputID));

	if (AbilitySystemComponent)
	{
		const int32 InputIDValue = static_cast<int32>(InputID);
		bool bFoundMatchingAbility = false;

		AbilitySystemComponent->AbilityLocalInputPressed(InputIDValue);

		for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
		{
			if (AbilitySpec.InputID != InputIDValue || !AbilitySpec.Ability)
			{
				continue;
			}

			bFoundMatchingAbility = true;

			if (!AbilitySpec.IsActive())
			{
				AbilitySystemComponent->TryActivateAbility(AbilitySpec.Handle);
			}
		}

		if (!bFoundMatchingAbility)
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("No ability found for input: %d"), InputIDValue);
		}
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Ability input ignored because AbilitySystemComponent is null."));
	}
}

void ALMS_TeamProjectCharacter::OnAbilityInputReleased(ELMSAbilityInputID InputID)
{
	UE_LOG(LogTemplateCharacter, Log, TEXT("Ability input released: %d"), static_cast<int32>(InputID));

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AbilityLocalInputReleased((int32)InputID);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("Ability input release ignored because AbilitySystemComponent is null."));
	}
}

void ALMS_TeamProjectCharacter::TakeDamage(float Damage)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->ApplyModToAttribute(
		ULMSAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		-Damage);

	UE_LOG(LogTemplateCharacter, Log, TEXT("%s took %.1f damage, remaining Health = %.1f"),
		*GetName(), Damage, AttributeSet ? AttributeSet->GetHealth() : 0.f);
}

void ALMS_TeamProjectCharacter::HandleHealthZero(const FGameplayEffectModCallbackData& Data)
{
	if (!HasAuthority() || !AbilitySystemComponent || !IncapacitatedEffect)
	{
		return;
	}

	// 중복 방지 — 이미 다운 상태면 스킵
	static const FGameplayTag IncapTag =
		FGameplayTag::RequestGameplayTag(FName("state.Incapacitated"));

	if (AbilitySystemComponent->HasMatchingGameplayTag(IncapTag))
	{
		return;
	}

	// GE_Incapacitated 적용
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec =
		AbilitySystemComponent->MakeOutgoingSpec(IncapacitatedEffect, 1.f, Context);

	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	if (BleedOutEffect)
	{
		FGameplayEffectSpecHandle BleedSpec =
			AbilitySystemComponent->MakeOutgoingSpec(BleedOutEffect, 1.f, Context);
		if (BleedSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*BleedSpec.Data.Get());

		}
	}
}

void ALMS_TeamProjectCharacter::HandleIncapHealthZero(const FGameplayEffectModCallbackData& Data)
{
	if (!HasAuthority() || !AbilitySystemComponent || !DeadEffect)
	{
		return;
	}

	static const FGameplayTag DeadTag =
		FGameplayTag::RequestGameplayTag(FName("state.Dead"));
	if (AbilitySystemComponent->HasMatchingGameplayTag(DeadTag)) return;

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle DeadSpec =
		AbilitySystemComponent->MakeOutgoingSpec(DeadEffect, 1.f, Context);
	if (DeadSpec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DeadSpec.Data.Get());

	}

}

void ALMS_TeamProjectCharacter::TraceForReviveTarget()
{
	// 로컬 컨트롤 플레이어만 트레이스 (남의 화면 기준은 의미 없음)
	if (!IsLocallyControlled())
	{
		return;
	}

	const FVector Start = GetActorLocation();
	const FVector End = Start + GetActorForwardVector() * ReviveTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);   // 나 자신 무시

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit, Start, End, ECC_Pawn, Params);

	// 디버그 시각화
	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 0.2f, 0, 1.f);
	if (bHit)
	{
		DrawDebugSphere(GetWorld(), Hit.ImpactPoint, 10.f, 8, FColor::Green, false, 0.2f);
	}

	// 맞은 대상이 다운 상태(state.Incapacitated)면 부활 후보로 확정
	AActor* NewTarget = nullptr;
	if (bHit && Hit.GetActor())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Hit.GetActor()))
		{
			if (UAbilitySystemComponent* TargetASC = ASI->GetAbilitySystemComponent())
			{
				if (TargetASC->HasMatchingGameplayTag(
					FGameplayTag::RequestGameplayTag("state.Incapacitated")))
				{
					NewTarget = Hit.GetActor();
				}
			}
		}
	}

	CurrentReviveTarget = NewTarget;

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	ULMSCombatHUDPresenterComponent* CombatHUDPresenter =
		PlayerController->FindComponentByClass<ULMSCombatHUDPresenterComponent>();
	if (!CombatHUDPresenter)
	{
		return;
	}

	if (CurrentReviveTarget)
	{
		CombatHUDPresenter->ShowInteractionPrompt(
			FText::FromString(TEXT("E")),
			FText::FromString(TEXT("구조하기"))
		);
	}
	else
	{
		CombatHUDPresenter->HideInteractionPrompt();
	}
}

void ALMS_TeamProjectCharacter::BeginPlay()
{
	// Call the base class
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 로컬 플레이어의 인디케이터 시스템에 자신을 팀원(녹색)으로 등록합니다.
	if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (UIndicatorManagerComponent* IndicatorManager = LocalPC->FindComponentByClass<UIndicatorManagerComponent>())
		{
			IndicatorManager->RegisterTarget(this, ELMSIndicatorType::Ally);
		}
	}

	GetWorldTimerManager().SetTimer(
		ReviveTraceTimerHandle, this,
		&ALMS_TeamProjectCharacter::TraceForReviveTarget,
		0.15f, true);
}

void ALMS_TeamProjectCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (UIndicatorManagerComponent* IndicatorManager = LocalPC->FindComponentByClass<UIndicatorManagerComponent>())
		{
			IndicatorManager->UnregisterTarget(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

ELMSIndicatorType ALMS_TeamProjectCharacter::GetIndicatorType_Implementation() const
{
	return ELMSIndicatorType::Ally;
}

bool ALMS_TeamProjectCharacter::ShouldShowIndicator_Implementation() const
{
	// 로컬 플레이어 자기 자신은 인디케이터로 표시하지 않습니다.
	if (const APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (LocalPC->GetPawn() == this)
		{
			return false;
		}
	}

	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("state.Dead"));
	return !AbilitySystemComponent || !AbilitySystemComponent->HasMatchingGameplayTag(DeadTag);
}

//////////////////////////////////////////////////////////////////////////
// Input

void ALMS_TeamProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent)) {
		
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALMS_TeamProjectCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALMS_TeamProjectCharacter::Look);

		// Sprint (홀드)
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::Sprint);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ALMS_TeamProjectCharacter::OnAbilityInputReleased, ELMSAbilityInputID::Sprint);

		// Dash (단발)
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::Dash);

		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::Interact);
		EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Completed, this, &ALMS_TeamProjectCharacter::OnAbilityInputReleased, ELMSAbilityInputID::Interact);

		if (PrimaryAction)
		{
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::PrimaryAttack);
			EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Completed, this, &ALMS_TeamProjectCharacter::OnAbilityInputReleased, ELMSAbilityInputID::PrimaryAttack);
		}
		else
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("PrimaryAction is not assigned on %s."), *GetNameSafe(this));
		}

		// Ping (마우스 휠 클릭)
		if (PingAction)
		{
			EnhancedInputComponent->BindAction(PingAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::RequestPing);
		}

		if (SecondaryAction)
		{
			EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::SecondaryAttack);
			EnhancedInputComponent->BindAction(SecondaryAction, ETriggerEvent::Completed, this, &ALMS_TeamProjectCharacter::OnAbilityInputReleased, ELMSAbilityInputID::SecondaryAttack);
		}
		else
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("SecondaryAction is not assigned on %s."), *GetNameSafe(this));
		}

		if (WeaponSkillAction)
		{
			EnhancedInputComponent->BindAction(WeaponSkillAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::WeaponSkill);
		}
		else
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("WeaponSkillAction is not assigned on %s."), *GetNameSafe(this));
		}

		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::Reload);
		}
		else
		{
			UE_LOG(LogTemplateCharacter, Warning, TEXT("ReloadAction is not assigned on %s."), *GetNameSafe(this));
		}



	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ALMS_TeamProjectCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	
		// get right vector 
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement 
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ALMS_TeamProjectCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void ALMS_TeamProjectCharacter::RequestPing(const FInputActionValue& Value)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// 1인칭/3인칭 무관하게 실제 카메라(크로스헤어) 기준으로 라인트레이스합니다.
	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * PingTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Ping), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, TraceEnd, ECC_Visibility, Params);
	const FVector PingLocation = bHit ? Hit.Location : TraceEnd;

	Server_RequestPing(PingLocation);
}

void ALMS_TeamProjectCharacter::Server_RequestPing_Implementation(FVector_NetQuantize PingLocation)
{
	if (!PingMarkerClass)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("PingMarkerClass is not assigned on %s."), *GetNameSafe(this));
		return;
	}

	const FTransform SpawnTransform(FRotator::ZeroRotator, PingLocation);
	GetWorld()->SpawnActor<APingMarker>(PingMarkerClass, SpawnTransform);
}
