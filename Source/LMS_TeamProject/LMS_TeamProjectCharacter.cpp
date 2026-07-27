// Copyright Epic Games, Inc. All Rights Reserved.

#include "LMS_TeamProjectCharacter.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraShakeBase.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#include "InteractionDetectorComponent.h"
#include "LMSAttributeSet.h"
#include "LMSGameplayAbility.h"
#include "LMSInteractableInterface.h"
#include "LMSSpectatorPawn.h"
#include "LMS_TeamProjectGameMode.h"
#include "LMS_TeamProjectPlayerState.h"
#include "PingMarker.h"
#include "UI/IndicatorManagerComponent.h"
#include "UI/LMSCombatHUDPresenterComponent.h"
#include "Weapons/LMSWeaponComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

namespace LMSTags
{
	static const FName TeamPlayer(TEXT("Team.Player"));
	static const FName StateDead(TEXT("state.Dead"));
	static const FName StateIncapacitated(TEXT("state.Incapacitated"));
	static const FName StateBeingRevived(TEXT("state.BeingRevived"));
	static const FName BlockJump(TEXT("State.Block.Jump"));
	static const FName DataHeal(TEXT("Data.Heal"));
	static const FName InteractionRevive(TEXT("Interaction.Revive"));
	static const FName InteractionRescueAll(TEXT("Interaction.RescueAll"));
	static const FName AbilityRoot(TEXT("Ability"));
	static const FName AbilityDowned(TEXT("Ability.Downed"));
}

//////////////////////////////////////////////////////////////////////////
// 생성 / 라이프사이클

ALMS_TeamProjectCharacter::ALMS_TeamProjectCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// 3인칭 무기 트레이스 방향이 조준 방향과 일치하도록 캐릭터를 카메라 Yaw에 고정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->JumpZVelocity = 700.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	WeaponComponent = CreateDefaultSubobject<ULMSWeaponComponent>(TEXT("WeaponComponent"));

	InteractionDetector = CreateDefaultSubobject<UInteractionDetectorComponent>(TEXT("InteractionDetector"));
	InteractionDetector->SetupAttachment(RootComponent);

	// 스켈레탈 메시 / AnimBP는 파생 블루프린트에서 지정 (C++ 직접 참조 회피)
}

void ALMS_TeamProjectCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	// 로컬 플레이어의 인디케이터 시스템에 자신을 팀원(녹색)으로 등록
	if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (UIndicatorManagerComponent* IndicatorManager = LocalPC->FindComponentByClass<UIndicatorManagerComponent>())
		{
			IndicatorManager->RegisterTarget(this, ELMSIndicatorType::Ally);
		}
	}

	// 상호작용 대상 감지는 InteractionDetector가 담당.
	// 대상이 바뀔 때 HUD 프롬프트를 갱신 (컴포넌트가 로컬에서만 감지)
	if (InteractionDetector)
	{
		InteractionDetector->OnTargetChanged.AddDynamic(
			this, &ALMS_TeamProjectCharacter::OnInteractTargetChanged);
	}
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

	GetWorldTimerManager().ClearTimer(CoherencyTimerHandle);
	GetWorldTimerManager().ClearTimer(CorpseTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void ALMS_TeamProjectCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALMS_TeamProjectCharacter, bIsDead);
}

//////////////////////////////////////////////////////////////////////////
// GAS 초기화

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

	GetWorldTimerManager().SetTimer(
		CoherencyTimerHandle, this, &ThisClass::CheckCoherency, 0.25f, true);
}

void ALMS_TeamProjectCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라: PlayerState가 복제돼서 도착한 시점
	InitAbilityActorInfo();
}

void ALMS_TeamProjectCharacter::InitAbilityActorInfo()
{
	ALMS_TeamProjectPlayerState* PS = GetPlayerState<ALMS_TeamProjectPlayerState>();
	if (!PS)
	{
		return;   // 아직 준비 안 됨
	}

	// PlayerState가 소유한 ASC/AttributeSet을 캐릭터에 캐싱
	AbilitySystemComponent = PS->GetAbilitySystemComponent();
	AttributeSet = PS->GetAttributeSet();

	// 핵심: Owner = PlayerState, Avatar = 이 캐릭터
	AbilitySystemComponent->InitAbilityActorInfo(PS, this);

	// 팀 태그 — 아군 오사 판정(PostGameplayEffectExecute)과 인디케이터 색 구분에 쓰인다.
	// AddReplicatedLooseGameplayTag만으로는 서버 로컬 태그 카운트가 올라가지 않아
	// 서버 판정이 통과돼 버렸다. AddLooseGameplayTag를 함께 호출해야 서버에서도 매칭된다.
	static const FGameplayTag TeamPlayerTag = FGameplayTag::RequestGameplayTag(LMSTags::TeamPlayer);
	if (HasAuthority() && !AbilitySystemComponent->HasMatchingGameplayTag(TeamPlayerTag))
	{
		AbilitySystemComponent->AddLooseGameplayTag(TeamPlayerTag);
		AbilitySystemComponent->AddReplicatedLooseGameplayTag(TeamPlayerTag);
	}

	if (WeaponComponent)
	{
		WeaponComponent->RefreshGrantedAbilities();
	}

	// Speed 어트리뷰트 → MaxWalkSpeed 동기화
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetSpeedAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetSpeedAttribute())
		.AddUObject(this, &ALMS_TeamProjectCharacter::OnSpeedChanged);

	// 구독 전에 이미 세팅된 값 대비 — 초기값 즉시 반영
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = AttributeSet->GetSpeed();
	}

	// OnHealthZero 등은 PostGameplayEffectExecute(서버)에서만 브로드캐스트되므로 서버에서만 바인딩.
	// 클라는 태그 복제로 다운 상태를 표현한다.
	if (HasAuthority() && AttributeSet)
	{
		// 재possess 시 중복 방지 — 제거 후 재바인딩
		AttributeSet->OnDamaged.RemoveAll(this);
		AttributeSet->OnDamaged.AddUObject(this, &ALMS_TeamProjectCharacter::HandleDamaged);

		AttributeSet->OnHealthZero.RemoveAll(this);
		AttributeSet->OnHealthZero.AddUObject(this, &ALMS_TeamProjectCharacter::HandleHealthZero);

		AttributeSet->OnIncapHealthZero.RemoveAll(this);
		AttributeSet->OnIncapHealthZero.AddUObject(this, &ALMS_TeamProjectCharacter::HandleIncapHealthZero);
	}
}

void ALMS_TeamProjectCharacter::GiveDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent || bDefaultsInitialized)
	{
		return;
	}

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}

		// 어빌리티 CDO에서 InputID를 꺼내 스펙에 실어준다
		const ULMSGameplayAbility* AbilityCDO = Cast<ULMSGameplayAbility>(AbilityClass->GetDefaultObject());
		const int32 InputID = AbilityCDO ? static_cast<int32>(AbilityCDO->AbilityInputID) : INDEX_NONE;

		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, InputID, this));
	}
}

void ALMS_TeamProjectCharacter::ApplyDefaultEffects()
{
	if (!HasAuthority() || !AbilitySystemComponent || bDefaultsInitialized)
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

	bDefaultsInitialized = true;
}

//////////////////////////////////////////////////////////////////////////
// 입력

void ALMS_TeamProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemplateCharacter, Error,
			TEXT("'%s' Failed to find an Enhanced Input component!"), *GetNameSafe(this));
		return;
	}

	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::Input_Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ALMS_TeamProjectCharacter::Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ALMS_TeamProjectCharacter::Look);

	// Sprint — 홀드
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::Sprint);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ALMS_TeamProjectCharacter::OnAbilityInputReleased, ELMSAbilityInputID::Sprint);

	// Dash — 단발
	EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ALMS_TeamProjectCharacter::OnAbilityInputPressed, ELMSAbilityInputID::Dash);

	// Interact — 실제 InputID는 대상이 정한다 (OnAbilityInputPressed 참조)
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

void ALMS_TeamProjectCharacter::Input_Jump()
{
	static const FGameplayTag BlockJumpTag = FGameplayTag::RequestGameplayTag(LMSTags::BlockJump);

	if (AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(BlockJumpTag))
	{
		return;
	}

	Jump();
}

void ALMS_TeamProjectCharacter::Move(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	const FRotator YawRotation(0, Controller->GetControlRotation().Yaw, 0);

	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	AddMovementInput(ForwardDirection, MovementVector.Y);
	AddMovementInput(RightDirection, MovementVector.X);
}

void ALMS_TeamProjectCharacter::Look(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void ALMS_TeamProjectCharacter::OnAbilityInputPressed(ELMSAbilityInputID InputID)
{
	// Interact(E)는 고정 어빌리티가 없다.
	// 감지된 대상에게 물어서 실제 InputID로 치환한다. (부활 / 탄약 / 문 …)
	if (InputID == ELMSAbilityInputID::Interact)
	{
		AActor* Target = InteractionDetector ? InteractionDetector->GetCurrentTarget() : nullptr;
		if (!Target || !Target->Implements<ULMSInteractableInterface>())
		{
			return;
		}

		if (!ILMSInteractableInterface::Execute_CanInteract(Target, this))
		{
			return;
		}

		InputID = static_cast<ELMSAbilityInputID>(
			ILMSInteractableInterface::Execute_GetInteractInputID(Target));

		CachedInteractInputID = InputID;   // Released 때 같은 곳으로 라우팅
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->AbilityLocalInputPressed(static_cast<int32>(InputID));
}

void ALMS_TeamProjectCharacter::OnAbilityInputReleased(ELMSAbilityInputID InputID)
{
	if (InputID == ELMSAbilityInputID::Interact)
	{
		if (CachedInteractInputID == ELMSAbilityInputID::None)
		{
			return;   // Pressed 때 아무것도 활성화되지 않았음
		}

		InputID = CachedInteractInputID;
		CachedInteractInputID = ELMSAbilityInputID::None;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->AbilityLocalInputReleased(static_cast<int32>(InputID));
}

void ALMS_TeamProjectCharacter::RequestPing(const FInputActionValue& Value)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	// 1인칭/3인칭 무관하게 실제 카메라(크로스헤어) 기준으로 트레이스
	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector TraceEnd = CameraLocation + CameraRotation.Vector() * PingTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(Ping), false, this);
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, TraceEnd, ECC_Visibility, Params);

	Server_RequestPing(bHit ? Hit.Location : TraceEnd);
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

//////////////////////////////////////////////////////////////////////////
// GAS 콜백 — 피격 / 다운 / 사망

void ALMS_TeamProjectCharacter::OnSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
	}
}

void ALMS_TeamProjectCharacter::TakeDamage(float Damage)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->ApplyModToAttribute(
		ULMSAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -Damage);
}

void ALMS_TeamProjectCharacter::HandleDamaged(const FGameplayEffectModCallbackData& Data)
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	ClientPlayDamageCameraShake();

	if (!HealBlockEffect)
	{
		return;
	}

	// 피격 직후 일정 시간 회복 차단 (State.Block.Heal)
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(HealBlockEffect, 1.f, Context);

	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}

void ALMS_TeamProjectCharacter::ClientPlayDamageCameraShake_Implementation()
{
	if (!DamageCameraShake)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController || !PlayerController->PlayerCameraManager)
	{
		return;
	}

	PlayerController->PlayerCameraManager->StartCameraShake(DamageCameraShake, DamageCameraShakeScale);
}

void ALMS_TeamProjectCharacter::HandleHealthZero(const FGameplayEffectModCallbackData& Data)
{
	if (!HasAuthority() || !AbilitySystemComponent || !IncapacitatedEffect)
	{
		return;
	}

	// 중복 방지 — 이미 다운 상태면 스킵
	static const FGameplayTag IncapTag = FGameplayTag::RequestGameplayTag(LMSTags::StateIncapacitated);
	if (AbilitySystemComponent->HasMatchingGameplayTag(IncapTag))
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	// 다운 상태 진입
	FGameplayEffectSpecHandle IncapSpec =
		AbilitySystemComponent->MakeOutgoingSpec(IncapacitatedEffect, 1.f, Context);
	if (IncapSpec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*IncapSpec.Data.Get());
	}

	// 출혈 시작 (IncapHealth 주기 감소)
	if (BleedOutEffect)
	{
		FGameplayEffectSpecHandle BleedSpec =
			AbilitySystemComponent->MakeOutgoingSpec(BleedOutEffect, 1.f, Context);
		if (BleedSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*BleedSpec.Data.Get());
		}
	}

	// 진행 중인 어빌리티 취소 — 단, 다운 상태에서 써야 하는 것은 남긴다
	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(FGameplayTag::RequestGameplayTag(LMSTags::AbilityRoot));

	FGameplayTagContainer IgnoreTags;
	IgnoreTags.AddTag(FGameplayTag::RequestGameplayTag(LMSTags::AbilityDowned));

	AbilitySystemComponent->CancelAbilities(&CancelTags, &IgnoreTags);
}

void ALMS_TeamProjectCharacter::HandleIncapHealthZero(const FGameplayEffectModCallbackData& Data)
{
	if (!HasAuthority() || !AbilitySystemComponent || !DeadEffect)
	{
		return;
	}

	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(LMSTags::StateDead);
	if (AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;
	}

	// GE_Dead — Incapacitated/BleedingOut 제거 + state.Dead 부여
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle DeadSpec =
		AbilitySystemComponent->MakeOutgoingSpec(DeadEffect, 1.f, Context);
	if (DeadSpec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*DeadSpec.Data.Get());
	}

	FGameplayTagContainer CancelTags;
	CancelTags.AddTag(FGameplayTag::RequestGameplayTag(LMSTags::AbilityRoot));
	AbilitySystemComponent->CancelAbilities(&CancelTags);

	// 스펙테이터 폰으로 전환 — 부활 시 되돌리려면 PC를 따로 보관해야 한다
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ALMSSpectatorPawn* SpecPawn = GetWorld()->SpawnActor<ALMSSpectatorPawn>(
			SpectatorPawnClass, GetActorLocation(), GetActorRotation(), SpawnParams);

		if (SpecPawn)
		{
			CachedOwnerPC = PC;
			PC->Possess(SpecPawn);

			if (AActor* AllyToWatch = FindFirstLivingAlly())
			{
				SpecPawn->SetSpectateTarget(AllyToWatch);
			}
		}
	}

	bIsDead = true;
	StartRagdoll();   // 서버는 직접, 클라는 OnRep_IsDead 경유

	if (ALMS_TeamProjectGameMode* LMSGameMode = GetWorld()->GetAuthGameMode<ALMS_TeamProjectGameMode>())
	{
		LMSGameMode->NotifyPlayerCharacterDied();
	}
}

AActor* ALMS_TeamProjectCharacter::FindFirstLivingAlly() const
{
	AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS)
	{
		return nullptr;
	}

	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(LMSTags::StateDead);

	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS || PS == GetPlayerState())
		{
			continue;   // 자기 자신 제외
		}

		ACharacter* Char = Cast<ACharacter>(PS->GetPawn());
		if (!Char)
		{
			continue;
		}

		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Char);
		if (!ASI)
		{
			continue;
		}

		UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
		if (!ASC || ASC->HasMatchingGameplayTag(DeadTag))
		{
			continue;
		}

		return Char;
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
// 죽음 → 래그돌 → 시체 정리 → 구조 부활

void ALMS_TeamProjectCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		StartRagdoll();
	}
	else
	{
		RestoreFromCorpse();
	}
}

void ALMS_TeamProjectCharacter::StartRagdoll()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// 부활 시 되돌릴 원래 상대 트랜스폼 저장
	MeshRelativeTransform = MeshComp->GetRelativeTransform();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// 캡슐 콜리전 off — AI/트레이스가 시체를 못 맞히게
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComp->bBlendPhysics = true;
	MeshComp->SetSimulatePhysics(true);
	MeshComp->WakeAllRigidBodies();

	// 각 머신이 자기 타이머를 돌린다 → 복제 신경 쓸 필요 없음
	GetWorldTimerManager().SetTimer(
		CorpseTimerHandle, this,
		&ALMS_TeamProjectCharacter::FinishCorpseCleanup,
		CorpseRagdollDuration, false);
}

void ALMS_TeamProjectCharacter::FinishCorpseCleanup()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// 1) 먼저 숨김 — 이후 물리 해제 팝이 아무한테도 안 보이게
	MeshComp->SetVisibility(false, true);

	// 2) 물리 해제
	MeshComp->SetSimulatePhysics(false);
	MeshComp->bBlendPhysics = false;
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 3) 캡슐에 재부착 + 원래 자세 복구
	MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	MeshComp->SetRelativeTransform(MeshRelativeTransform);
}

void ALMS_TeamProjectCharacter::RestoreFromCorpse()
{
	// 타이머가 아직 안 돌았을 수도 있으니 먼저 취소
	GetWorldTimerManager().ClearTimer(CorpseTimerHandle);

	USkeletalMeshComponent* MeshComp = GetMesh();
	if (!MeshComp)
	{
		return;
	}

	// 타이머 전에 부활했을 경우를 대비해 물리 정리를 여기서도 보장
	MeshComp->SetSimulatePhysics(false);
	MeshComp->bBlendPhysics = false;
	MeshComp->SetCollisionProfileName(TEXT("CharacterMesh"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	MeshComp->SetRelativeTransform(MeshRelativeTransform);
	MeshComp->SetVisibility(true, true);

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->SetMovementMode(MOVE_Walking);
	}
}

void ALMS_TeamProjectCharacter::RescueFromDeath(const FVector& ReviveLocation, const FRotator& ReviveRotation)
{
	if (!HasAuthority() || !AbilitySystemComponent || !RescuedEffect)
	{
		return;
	}

	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(LMSTags::StateDead);
	if (!AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;   // 죽은 상태가 아니면 무시
	}

	// 1) 컨트롤러 확보 — 언포제스로 PlayerState가 끊겼으므로 캐시 사용
	APlayerController* PC = CachedOwnerPC;

	// 2) 숨겨진 상태에서 구조물 옆으로 이동 (아직 콜리전 off)
	SetActorLocationAndRotation(
		ReviveLocation, ReviveRotation, false, nullptr, ETeleportType::TeleportPhysics);

	// 3) 시체 해제 — 서버는 직접, 클라는 OnRep_IsDead
	bIsDead = false;
	RestoreFromCorpse();

	// 4) 재possess → PossessedBy → InitAbilityActorInfo로 Avatar 재연결
	APawn* SpecPawn = nullptr;
	if (PC)
	{
		SpecPawn = PC->GetPawn();   // 현재는 스펙테이터 폰
		PC->Possess(this);
	}

	// 5) GE_Rescued 적용 — possess 이후여야 InitStats에 안 덮인다
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(RescuedEffect, 1.f, Context);
	if (Spec.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}

	// 6) IncapHealth 원복 — GE 제거가 끝난 뒤여야 Periodic과 안 다툰다
	if (AttributeSet)
	{
		AttributeSet->SetIncapHealth(AttributeSet->GetMaxIncapHealth());
	}

	// 7) 남은 스펙테이터 폰 정리
	if (SpecPawn && SpecPawn != this)
	{
		SpecPawn->Destroy();
	}

	CachedOwnerPC = nullptr;
}

//////////////////////////////////////////////////////////////////////////
// Coherency — 주변 아군 수에 비례해 실드 회복

void ALMS_TeamProjectCharacter::CheckCoherency()
{
	if (!HasAuthority() || !AbilitySystemComponent || !HealEffect)
	{
		return;
	}

	static const FGameplayTag IncapTag = FGameplayTag::RequestGameplayTag(LMSTags::StateIncapacitated);
	static const FGameplayTag HealDataTag = FGameplayTag::RequestGameplayTag(LMSTags::DataHeal);

	const FVector MyLocation = GetActorLocation();
	const float CoherencyDistanceSq = FMath::Square(CoherencyDistance);

	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ALMS_TeamProjectCharacter::StaticClass(), Players);

	int32 NearbyCount = 0;
	for (AActor* Actor : Players)
	{
		ALMS_TeamProjectCharacter* Other = Cast<ALMS_TeamProjectCharacter>(Actor);
		if (!Other || Other == this || !Other->AbilitySystemComponent)
		{
			continue;
		}

		// 다운된 아군은 인원수에 포함하지 않는다
		if (Other->AbilitySystemComponent->HasMatchingGameplayTag(IncapTag))
		{
			continue;
		}

		if (FVector::DistSquared(MyLocation, Other->GetActorLocation()) > CoherencyDistanceSq)
		{
			continue;
		}

		++NearbyCount;
	}

	if (NearbyCount <= 0)
	{
		return;
	}

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle HealSpec = AbilitySystemComponent->MakeOutgoingSpec(HealEffect, 1.f, Context);
	if (HealSpec.IsValid())
	{
		HealSpec.Data->SetSetByCallerMagnitude(HealDataTag, NearbyCount * 3.f);
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*HealSpec.Data.Get());
	}
}

//////////////////////////////////////////////////////////////////////////
// ILMSInteractableInterface — 다운된 자신이 곧 상호작용(부활) 대상

bool ALMS_TeamProjectCharacter::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor || Interactor == this)
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	static const FGameplayTag IncapTag = FGameplayTag::RequestGameplayTag(LMSTags::StateIncapacitated);
	static const FGameplayTag BeingRevivedTag = FGameplayTag::RequestGameplayTag(LMSTags::StateBeingRevived);

	// 다운 상태이면서, 아직 아무도 부활을 시작하지 않았을 때만
	return ASC->HasMatchingGameplayTag(IncapTag) && !ASC->HasMatchingGameplayTag(BeingRevivedTag);
}

FGameplayTag ALMS_TeamProjectCharacter::GetInteractionType_Implementation() const
{
	static const FGameplayTag ReviveType = FGameplayTag::RequestGameplayTag(LMSTags::InteractionRevive);
	return ReviveType;
}

int32 ALMS_TeamProjectCharacter::GetInteractInputID_Implementation() const
{
	return static_cast<int32>(ELMSAbilityInputID::Interact_Revive);
}

void ALMS_TeamProjectCharacter::OnInteractTargetChanged(AActor* NewTarget, FGameplayTag InteractionType)
{
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

	if (!NewTarget)
	{
		CombatHUDPresenter->HideInteractionPrompt();
		return;
	}

	static const FGameplayTag RescueAllType = FGameplayTag::RequestGameplayTag(LMSTags::InteractionRescueAll);

	const FText InteractionText = InteractionType.MatchesTagExact(RescueAllType)
		? FText::FromString(TEXT("전원 구조하기"))
		: FText::FromString(TEXT("구조하기"));

	CombatHUDPresenter->ShowInteractionPrompt(FText::FromString(TEXT("E")), InteractionText);
}

//////////////////////////////////////////////////////////////////////////
// IIndicatorTargetInterface

ELMSIndicatorType ALMS_TeamProjectCharacter::GetIndicatorType_Implementation() const
{
	return ELMSIndicatorType::Ally;
}

bool ALMS_TeamProjectCharacter::ShouldShowIndicator_Implementation() const
{
	// 로컬 플레이어 자기 자신은 인디케이터로 표시하지 않는다
	if (const APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (LocalPC->GetPawn() == this)
		{
			return false;
		}
	}

	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(LMSTags::StateDead);
	return !AbilitySystemComponent || !AbilitySystemComponent->HasMatchingGameplayTag(DeadTag);
}
