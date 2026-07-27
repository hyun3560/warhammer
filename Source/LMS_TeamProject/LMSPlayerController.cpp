#include "LMSPlayerController.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LMSMenuFlowSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "UI/LMSCombatHUDPresenterComponent.h"
#include "UI/UIManagerComponent.h"
#include "UI/IndicatorManagerComponent.h"
#include "UI/LMSTeamStatusComponent.h"
#include "UI/LMSMainMenuWidget.h"
#include "LMS_TeamProjectCharacter.h"
#include "LMS_TeamProjectPlayerState.h"
#include "Weapons/LMSWeaponComponent.h"

namespace
{
	const FName LMSMainMenuHostSessionName(TEXT("LMSMainMenuHostSession"));
	const FName LMSMainMenuJoinSessionName(TEXT("LMSMainMenuJoinSession"));

	FGameplayTag GetIncapacitatedTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("state.Incapacitated")));
	}

	FGameplayTag GetDeadTag()
	{
		return FGameplayTag::RequestGameplayTag(FName(TEXT("state.Dead")));
	}

	bool TextContainsAnyFirstPersonToken(const FString& Text)
	{
		return Text.Contains(TEXT("FirstPerson"), ESearchCase::IgnoreCase)
			|| Text.Contains(TEXT("First Person"), ESearchCase::IgnoreCase)
			|| Text.Contains(TEXT("FP_"), ESearchCase::IgnoreCase)
			|| Text.Contains(TEXT("_FP"), ESearchCase::IgnoreCase)
			|| Text.Contains(TEXT("1P"), ESearchCase::IgnoreCase)
			|| Text.Contains(TEXT("Arms"), ESearchCase::IgnoreCase)
			|| Text.Contains(TEXT("Arm"), ESearchCase::IgnoreCase);
	}
}

ALMSPlayerController::ALMSPlayerController()
{
	// PlayerController가 생성될 때 UI 관련 컴포넌트들도 함께 생성합니다.
	UIManagerComponent = CreateDefaultSubobject<UUIManagerComponent>(TEXT("UIManagerComponent"));
	CombatHUDPresenterComponent = CreateDefaultSubobject<ULMSCombatHUDPresenterComponent>(TEXT("CombatHUDPresenterComponent"));
	IndicatorManagerComponent = CreateDefaultSubobject<UIndicatorManagerComponent>(TEXT("IndicatorManagerComponent"));
	TeamStatusComponent = CreateDefaultSubobject<ULMSTeamStatusComponent>(TEXT("TeamStatusComponent"));
}

void ALMSPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 멀티플레이에서 로컬 플레이어 Controller만 UI를 생성/표시합니다.
	if (!IsLocalController())
	{
		return;
	}

	ULMSMenuFlowSubsystem* MenuFlowSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULMSMenuFlowSubsystem>()
		: nullptr;

	if (MenuFlowSubsystem && MenuFlowSubsystem->ConsumeGameplayIntroRequest())
	{
		BeginGameplayIntro();
		return;
	}

	if (MenuFlowSubsystem && MenuFlowSubsystem->ShouldShowStartupMenu(GetWorld()))
	{
		ShowStartupMenu();
		return;
	}

	InitializeGameplayUI();
	return;

}

void ALMSPlayerController::InitializeGameplayUI()
{
	if (UIManagerComponent)
	{
		UIManagerComponent->ShowUI(ELMSUIType::Combat);
	}

	if (CombatHUDPresenterComponent)
	{
		CombatHUDPresenterComponent->InitializeCombatHUD();
	}

	if (TeamStatusComponent)
	{
		TeamStatusComponent->InitializeTeamStatus();
	}

	BindGroggyCameraState();

	//ShowWeaponSelectionUI();
}

void ALMSPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 클라이언트에서는 PlayerState가 BeginPlay보다 늦게 복제될 수 있습니다.
	// 그 경우 PlayerState가 준비된 뒤 다시 HUD 연결을 시도합니다.
	if (CombatHUDPresenterComponent)
	{
		CombatHUDPresenterComponent->InitializeCombatHUD();
	}

	if (TeamStatusComponent)
	{
		TeamStatusComponent->InitializeTeamStatus();
	}

	BindGroggyCameraState();
}

void ALMSPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	if (!IsLocalController())
	{
		return;
	}

	BindGroggyCameraState();
	UpdateGroggyCameraState();
}

void ALMSPlayerController::ShowStartupMenu()
{
	if (!IsLocalController())
	{
		return;
	}

	if (UIManagerComponent)
	{
		UIManagerComponent->HideUI(ELMSUIType::Combat);
	}

	ACameraActor* MenuCamera = FindOrSpawnMenuCamera();
	if (MenuCamera)
	{
		SetViewTarget(MenuCamera);
	}

	if (!MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<ULMSMainMenuWidget>(this, ULMSMainMenuWidget::StaticClass());
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport(100);
			MainMenuWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
			MainMenuWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
			MainMenuWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
			MainMenuWidget->SetDesiredSizeInViewport(FVector2D(1920.f, 1080.f));
		}
	}

	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Visible);
	}

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ALMSPlayerController::HideStartupMenu()
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ALMSPlayerController::BeginGameplayIntro()
{
	if (!IsLocalController())
	{
		return;
	}

	HideStartupMenu();

	if (UIManagerComponent)
	{
		UIManagerComponent->HideUI(ELMSUIType::Combat);
	}

	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	if (ACameraActor* MenuCamera = FindOrSpawnMenuCamera())
	{
		SetViewTarget(MenuCamera);
	}

	GameplayIntroRetryCount = 0;
	GetWorldTimerManager().SetTimer(
		GameplayIntroRetryTimerHandle,
		this,
		&ThisClass::TryBlendFromMenuCameraToPawn,
		0.1f,
		true);
}

void ALMSPlayerController::TryBlendFromMenuCameraToPawn()
{
	++GameplayIntroRetryCount;

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		if (GameplayIntroRetryCount > 60)
		{
			GetWorldTimerManager().ClearTimer(GameplayIntroRetryTimerHandle);
			FinishGameplayIntro();
		}

		return;
	}

	GetWorldTimerManager().ClearTimer(GameplayIntroRetryTimerHandle);

	const ULMSMenuFlowSubsystem* MenuFlowSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULMSMenuFlowSubsystem>()
		: nullptr;
	const float BlendTime = MenuFlowSubsystem ? MenuFlowSubsystem->GetIntroBlendTime() : 2.5f;

	SetViewTargetWithBlend(ControlledPawn, BlendTime, VTBlend_Cubic);

	GetWorldTimerManager().SetTimer(
		GameplayIntroFinishTimerHandle,
		this,
		&ThisClass::FinishGameplayIntro,
		BlendTime,
		false);
}

void ALMSPlayerController::FinishGameplayIntro()
{
	GetWorldTimerManager().ClearTimer(GameplayIntroRetryTimerHandle);
	GetWorldTimerManager().ClearTimer(GameplayIntroFinishTimerHandle);

	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;

	InitializeGameplayUI();
}

void ALMSPlayerController::BindGroggyCameraState()
{
	if (!IsLocalController())
	{
		return;
	}

	ALMS_TeamProjectPlayerState* LMSPlayerState = GetPlayerState<ALMS_TeamProjectPlayerState>();
	if (!LMSPlayerState)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = LMSPlayerState->GetAbilitySystemComponent();
	if (!AbilitySystemComponent)
	{
		return;
	}

	if (CachedGroggyCameraAbilitySystemComponent != AbilitySystemComponent)
	{
		UnbindGroggyCameraState();
		CachedGroggyCameraAbilitySystemComponent = AbilitySystemComponent;
	}

	if (!GroggyCameraTagDelegateHandle.IsValid())
	{
		GroggyCameraTagDelegateHandle = CachedGroggyCameraAbilitySystemComponent->RegisterGameplayTagEvent(
			GetIncapacitatedTag(),
			EGameplayTagEventType::NewOrRemoved
		).AddUObject(this, &ThisClass::HandleGroggyCameraTagChanged);
	}

	UpdateGroggyCameraState();
}

void ALMSPlayerController::UnbindGroggyCameraState()
{
	if (CachedGroggyCameraAbilitySystemComponent && GroggyCameraTagDelegateHandle.IsValid())
	{
		CachedGroggyCameraAbilitySystemComponent->RegisterGameplayTagEvent(
			GetIncapacitatedTag(),
			EGameplayTagEventType::NewOrRemoved
		).Remove(GroggyCameraTagDelegateHandle);
	}

	GroggyCameraTagDelegateHandle.Reset();
	CachedGroggyCameraAbilitySystemComponent = nullptr;
}

void ALMSPlayerController::UpdateGroggyCameraState()
{
	if (!CachedGroggyCameraAbilitySystemComponent)
	{
		return;
	}

	if (CachedGroggyCameraAbilitySystemComponent->HasMatchingGameplayTag(GetIncapacitatedTag()))
	{
		EnterGroggyCamera();
		return;
	}

	if (CachedGroggyCameraAbilitySystemComponent->HasMatchingGameplayTag(GetDeadTag()))
	{
		return;
	}

	ExitGroggyCamera();
}

void ALMSPlayerController::HandleGroggyCameraTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	UpdateGroggyCameraState();
}

void ALMSPlayerController::EnterGroggyCamera()
{
	if (bUsingGroggyCamera)
	{
		return;
	}

	SetLocalCharacterThirdPersonCameraEnabled(true);
}

void ALMSPlayerController::ExitGroggyCamera()
{
	if (!bUsingGroggyCamera)
	{
		return;
	}

	SetLocalCharacterThirdPersonCameraEnabled(false);
}

void ALMSPlayerController::SetLocalCharacterThirdPersonCameraEnabled(bool bEnabled)
{
	ALMS_TeamProjectCharacter* LMSCharacter = Cast<ALMS_TeamProjectCharacter>(GetPawn());
	if (!LMSCharacter)
	{
		return;
	}

	UCameraComponent* ThirdPersonCamera = LMSCharacter->GetFollowCamera();

	TArray<UCameraComponent*> CameraComponents;
	LMSCharacter->GetComponents<UCameraComponent>(CameraComponents);

	if (bEnabled)
	{
		PreGroggyActiveCameraComponents.Reset();

		for (UCameraComponent* CameraComponent : CameraComponents)
		{
			if (!CameraComponent)
			{
				continue;
			}

			if (CameraComponent->IsActive() && CameraComponent != ThirdPersonCamera)
			{
				PreGroggyActiveCameraComponents.Add(CameraComponent);
			}

			CameraComponent->Deactivate();
		}

		if (ThirdPersonCamera)
		{
			ThirdPersonCamera->Activate(true);
		}

		if (ULMSWeaponComponent* WeaponComponent = LMSCharacter->GetWeaponComponent())
		{
			WeaponComponent->SetLocalFirstPersonWeaponViewEnabled(false);
		}

		ApplyGroggyMeshVisibility(LMSCharacter);
		SetViewTargetWithBlend(LMSCharacter, 0.25f, VTBlend_Cubic);
		bUsingGroggyCamera = true;
		return;
	}

	for (UCameraComponent* CameraComponent : CameraComponents)
	{
		if (CameraComponent)
		{
			CameraComponent->Deactivate();
		}
	}

	bool bRestoredCamera = false;
	for (UCameraComponent* CameraComponent : PreGroggyActiveCameraComponents)
	{
		if (CameraComponent)
		{
			CameraComponent->Activate(true);
			bRestoredCamera = true;
		}
	}

	if (!bRestoredCamera)
	{
		for (UCameraComponent* CameraComponent : CameraComponents)
		{
			if (CameraComponent && CameraComponent != ThirdPersonCamera)
			{
				CameraComponent->Activate(true);
				bRestoredCamera = true;
				break;
			}
		}
	}

	if (!bRestoredCamera && ThirdPersonCamera)
	{
		ThirdPersonCamera->Activate(true);
	}

	PreGroggyActiveCameraComponents.Reset();

	if (ULMSWeaponComponent* WeaponComponent = LMSCharacter->GetWeaponComponent())
	{
		WeaponComponent->SetLocalFirstPersonWeaponViewEnabled(true);
	}

	RestorePreGroggyMeshVisibility();
	SetViewTargetWithBlend(LMSCharacter, 0.25f, VTBlend_Cubic);
	bUsingGroggyCamera = false;
}

void ALMSPlayerController::ApplyGroggyMeshVisibility(ALMS_TeamProjectCharacter* LMSCharacter)
{
	if (!LMSCharacter)
	{
		return;
	}

	PreGroggyPrimitiveVisibilityStates.Reset();

	auto SaveVisibilityState = [this](UPrimitiveComponent* PrimitiveComponent)
		{
			if (!PrimitiveComponent)
			{
				return;
			}

			for (const FGroggyPrimitiveVisibilityState& ExistingState : PreGroggyPrimitiveVisibilityStates)
			{
				if (ExistingState.PrimitiveComponent.Get() == PrimitiveComponent)
				{
					return;
				}
			}

			FGroggyPrimitiveVisibilityState NewState;
			NewState.PrimitiveComponent = PrimitiveComponent;
			NewState.bHiddenInGame = PrimitiveComponent->bHiddenInGame;
			NewState.bOwnerNoSee = PrimitiveComponent->bOwnerNoSee;
			NewState.bOnlyOwnerSee = PrimitiveComponent->bOnlyOwnerSee;
			PreGroggyPrimitiveVisibilityStates.Add(NewState);
		};

	USkeletalMeshComponent* ThirdPersonMesh = LMSCharacter->GetMesh();
	if (ThirdPersonMesh)
	{
		SaveVisibilityState(ThirdPersonMesh);
		ThirdPersonMesh->SetHiddenInGame(false);
		ThirdPersonMesh->SetOwnerNoSee(false);
		ThirdPersonMesh->SetOnlyOwnerSee(false);
	}

	TArray<UPrimitiveComponent*> PrimitiveComponents;
	LMSCharacter->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsFirstPersonViewPrimitiveComponent(PrimitiveComponent, ThirdPersonMesh))
		{
			continue;
		}

		SaveVisibilityState(PrimitiveComponent);
		PrimitiveComponent->SetHiddenInGame(true);
		PrimitiveComponent->SetOwnerNoSee(false);
		PrimitiveComponent->SetOnlyOwnerSee(false);
	}
}

void ALMSPlayerController::RestorePreGroggyMeshVisibility()
{
	for (const FGroggyPrimitiveVisibilityState& VisibilityState : PreGroggyPrimitiveVisibilityStates)
	{
		UPrimitiveComponent* PrimitiveComponent = VisibilityState.PrimitiveComponent.Get();
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetHiddenInGame(VisibilityState.bHiddenInGame);
		PrimitiveComponent->SetOwnerNoSee(VisibilityState.bOwnerNoSee);
		PrimitiveComponent->SetOnlyOwnerSee(VisibilityState.bOnlyOwnerSee);
	}

	PreGroggyPrimitiveVisibilityStates.Reset();
}

bool ALMSPlayerController::IsFirstPersonViewPrimitiveComponent(
	const UPrimitiveComponent* PrimitiveComponent,
	const UPrimitiveComponent* ThirdPersonMesh) const
{
	if (!PrimitiveComponent || PrimitiveComponent == ThirdPersonMesh)
	{
		return false;
	}

	if (PrimitiveComponent->bOnlyOwnerSee)
	{
		return true;
	}

	if (TextContainsAnyFirstPersonToken(PrimitiveComponent->GetName()))
	{
		return true;
	}

	for (const FName& ComponentTag : PrimitiveComponent->ComponentTags)
	{
		if (TextContainsAnyFirstPersonToken(ComponentTag.ToString()))
		{
			return true;
		}
	}

	return false;
}

ACameraActor* ALMSPlayerController::FindOrSpawnMenuCamera()
{
	if (RuntimeMenuCamera)
	{
		return RuntimeMenuCamera;
	}

	const ULMSMenuFlowSubsystem* MenuFlowSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULMSMenuFlowSubsystem>()
		: nullptr;
	const FName MenuCameraTag = MenuFlowSubsystem ? MenuFlowSubsystem->GetMenuCameraTag() : FName(TEXT("MenuCamera"));

	for (TActorIterator<ACameraActor> It(GetWorld()); It; ++It)
	{
		ACameraActor* CameraActor = *It;
		if (CameraActor && CameraActor->ActorHasTag(MenuCameraTag))
		{
			RuntimeMenuCamera = CameraActor;
			return RuntimeMenuCamera;
		}
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector FocusLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
	const FVector CameraLocation = FocusLocation + FVector(-1200.f, 0.f, 7200.f);
	const FRotator CameraRotation = (FocusLocation - CameraLocation).Rotation();

	RuntimeMenuCamera = World->SpawnActor<ACameraActor>(CameraLocation, CameraRotation);
	if (RuntimeMenuCamera)
	{
		RuntimeMenuCamera->Tags.Add(MenuCameraTag);
		RuntimeMenuCamera->GetCameraComponent()->SetFieldOfView(42.f);
	}

	return RuntimeMenuCamera;
}

void ALMSPlayerController::HostGameFromMainMenu()
{
	SetMainMenuStatusMessage(FText::FromString(TEXT("Creating LAN session...")));

	IOnlineSessionPtr Sessions = GetOnlineSessionInterface();
	if (!Sessions.IsValid())
	{
		SetMainMenuStatusMessage(FText::FromString(TEXT("Session system unavailable. Starting listen server directly.")));
		OpenMainMenuListenLevel();
		return;
	}

	if (Sessions->GetNamedSession(LMSMainMenuHostSessionName))
	{
		DestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyMainMenuSessionComplete));
		Sessions->DestroySession(LMSMainMenuHostSessionName);
		return;
	}

	CreateMainMenuSession();
}

void ALMSPlayerController::JoinGameFromMainMenu(const FString& Address)
{
	FindGameFromMainMenu();
}

void ALMSPlayerController::FindGameFromMainMenu()
{
	SetMainMenuStatusMessage(FText::FromString(TEXT("Searching for LAN sessions...")));

	IOnlineSessionPtr Sessions = GetOnlineSessionInterface();
	if (!Sessions.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Online session interface is not available."));
		SetMainMenuStatusMessage(FText::FromString(TEXT("Session system unavailable.")));
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 20;
	SessionSearch->PingBucketSize = 50;

	FindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindMainMenuSessionsComplete));

	if (!Sessions->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
		UE_LOG(LogTemp, Warning, TEXT("FindSessions did not start."));
		SetMainMenuStatusMessage(FText::FromString(TEXT("Could not start session search.")));
	}
}

IOnlineSessionPtr ALMSPlayerController::GetOnlineSessionInterface() const
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	return OnlineSubsystem ? OnlineSubsystem->GetSessionInterface() : nullptr;
}

void ALMSPlayerController::CreateMainMenuSession()
{
	IOnlineSessionPtr Sessions = GetOnlineSessionInterface();
	if (!Sessions.IsValid())
	{
		OpenMainMenuListenLevel();
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = true;
	SessionSettings.NumPublicConnections = 4;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = false;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = false;
	SessionSettings.bUseLobbiesIfAvailable = false;
	SessionSettings.Set(FName(TEXT("MAPNAME")), FString(TEXT("L_showcase_level")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	CreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateMainMenuSessionComplete));

	if (!Sessions->CreateSession(0, LMSMainMenuHostSessionName, SessionSettings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
		UE_LOG(LogTemp, Warning, TEXT("CreateSession did not start. Falling back to listen travel."));
		SetMainMenuStatusMessage(FText::FromString(TEXT("Could not create session. Starting listen server directly.")));
		OpenMainMenuListenLevel();
	}
}

void ALMSPlayerController::OpenMainMenuListenLevel() const
{
	if (ULMSMenuFlowSubsystem* MenuFlowSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULMSMenuFlowSubsystem>()
		: nullptr)
	{
		MenuFlowSubsystem->RequestGameplayIntro();
		UGameplayStatics::OpenLevel(this, MenuFlowSubsystem->GetGameMapName(), true, TEXT("listen"));
	}
}

void ALMSPlayerController::OnCreateMainMenuSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetOnlineSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
	}
	CreateSessionCompleteDelegateHandle.Reset();

	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("CreateSession failed. Falling back to listen travel."));
		SetMainMenuStatusMessage(FText::FromString(TEXT("Session creation failed. Starting listen server directly.")));
	}
	else
	{
		SetMainMenuStatusMessage(FText::FromString(TEXT("Session created. Loading game...")));
	}

	OpenMainMenuListenLevel();
}

void ALMSPlayerController::OnDestroyMainMenuSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSessionPtr Sessions = GetOnlineSessionInterface())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}
	DestroySessionCompleteDelegateHandle.Reset();

	if (SessionName == LMSMainMenuHostSessionName)
	{
		CreateMainMenuSession();
		return;
	}

	if (SessionName == LMSMainMenuJoinSessionName && bHasPendingJoinSessionSearchResult)
	{
		JoinFirstFoundMainMenuSession();
	}
}

void ALMSPlayerController::OnFindMainMenuSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr Sessions = GetOnlineSessionInterface();
	if (Sessions.IsValid())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
	}
	FindSessionsCompleteDelegateHandle.Reset();

	if (!bWasSuccessful || !SessionSearch.IsValid() || SessionSearch->SearchResults.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("No LMS LAN sessions found."));
		SetMainMenuStatusMessage(FText::FromString(TEXT("No LAN sessions found. Start HOST GAME in another window first.")));
		return;
	}

	SetMainMenuStatusMessage(FText::FromString(TEXT("Session found. Joining...")));

	PendingJoinSessionSearchResult = SessionSearch->SearchResults[0];
	bHasPendingJoinSessionSearchResult = true;
	JoinFirstFoundMainMenuSession();
}

void ALMSPlayerController::JoinFirstFoundMainMenuSession()
{
	IOnlineSessionPtr Sessions = GetOnlineSessionInterface();
	if (!Sessions.IsValid() || !bHasPendingJoinSessionSearchResult)
	{
		SetMainMenuStatusMessage(FText::FromString(TEXT("Could not prepare session join.")));
		return;
	}

	if (Sessions->GetNamedSession(LMSMainMenuJoinSessionName))
	{
		SetMainMenuStatusMessage(FText::FromString(TEXT("Cleaning up previous join attempt...")));
		DestroySessionCompleteDelegateHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroyMainMenuSessionComplete));

		if (!Sessions->DestroySession(LMSMainMenuJoinSessionName))
		{
			Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			DestroySessionCompleteDelegateHandle.Reset();
			SetMainMenuStatusMessage(FText::FromString(TEXT("Could not clean up previous join attempt.")));
		}

		return;
	}

	SetMainMenuStatusMessage(FText::FromString(TEXT("Joining found session...")));
	JoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinMainMenuSessionComplete));

	if (!Sessions->JoinSession(0, LMSMainMenuJoinSessionName, PendingJoinSessionSearchResult))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
		bHasPendingJoinSessionSearchResult = false;
		UE_LOG(LogTemp, Warning, TEXT("JoinSession did not start."));
		SetMainMenuStatusMessage(FText::FromString(TEXT("Could not start joining. Try a separate Standalone window.")));
	}
}

void ALMSPlayerController::OnJoinMainMenuSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Sessions = GetOnlineSessionInterface();
	if (!Sessions.IsValid())
	{
		SetMainMenuStatusMessage(FText::FromString(TEXT("Session system disappeared while joining.")));
		return;
	}

	Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
	JoinSessionCompleteDelegateHandle.Reset();
	bHasPendingJoinSessionSearchResult = false;

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession failed with result %d."), static_cast<int32>(Result));
		SetMainMenuStatusMessage(FText::FromString(TEXT("Join failed.")));
		return;
	}

	FString ConnectString;
	if (!Sessions->GetResolvedConnectString(SessionName, ConnectString))
	{
		UE_LOG(LogTemp, Warning, TEXT("Could not resolve session connect string."));
		SetMainMenuStatusMessage(FText::FromString(TEXT("Could not resolve session address.")));
		return;
	}

	ConnectString = NormalizeMainMenuConnectString(ConnectString);
	if (ConnectString.IsEmpty())
	{
		SetMainMenuStatusMessage(FText::FromString(TEXT("Resolved session address was empty.")));
		return;
	}

	if (ULMSMenuFlowSubsystem* MenuFlowSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<ULMSMenuFlowSubsystem>()
		: nullptr)
	{
		MenuFlowSubsystem->RequestGameplayIntro();
	}

	UE_LOG(LogTemp, Log, TEXT("Joining LMS session using connect string: %s"), *ConnectString);
	SetMainMenuStatusMessage(FText::Format(FText::FromString(TEXT("Joining {0}...")), FText::FromString(ConnectString)));
	ClientTravel(ConnectString, TRAVEL_Absolute);
}

FString ALMSPlayerController::NormalizeMainMenuConnectString(const FString& ConnectString) const
{
	FString Normalized = ConnectString.TrimStartAndEnd();

	int32 SlashIndex = INDEX_NONE;
	if (Normalized.FindChar(TEXT('/'), SlashIndex))
	{
		Normalized = Normalized.Left(SlashIndex);
	}

	Normalized.TrimStartAndEndInline();
	if (Normalized.IsEmpty())
	{
		return Normalized;
	}

	int32 ColonIndex = INDEX_NONE;
	if (!Normalized.FindChar(TEXT(':'), ColonIndex))
	{
		Normalized += TEXT(":7777");
		return Normalized;
	}

	const FString Host = Normalized.Left(ColonIndex);
	FString Port = Normalized.Mid(ColonIndex + 1);
	Port.TrimStartAndEndInline();

	if (Port.IsEmpty() || Port == TEXT("0"))
	{
		Normalized = Host + TEXT(":7777");
	}

	return Normalized;
}

void ALMSPlayerController::SetMainMenuStatusMessage(const FText& Message) const
{
	if (MainMenuWidget)
	{
		MainMenuWidget->SetStatusMessage(Message);
	}
}

void ALMSPlayerController::QuitGameFromMainMenu()
{
	UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, true);
}

void ALMSPlayerController::ShowWeaponSelectionUI()
{
	if (!IsLocalController() || !UIManagerComponent)
	{
		return;
	}

	UIManagerComponent->ShowUI(ELMSUIType::WeaponSelect);

	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ALMSPlayerController::HideWeaponSelectionUI()
{
	if (!IsLocalController() || !UIManagerComponent)
	{
		return;
	}

	UIManagerComponent->HideUI(ELMSUIType::WeaponSelect);

	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void ALMSPlayerController::RequestWeaponSelection(FName WeaponID)
{
	if (WeaponID.IsNone())
	{
		return;
	}

	ServerRequestWeaponSelection(WeaponID);
}

void ALMSPlayerController::ServerRequestWeaponSelection_Implementation(FName WeaponID)
{
	bool bSuccess = false;

	if (!WeaponID.IsNone())
	{
		if (ALMS_TeamProjectPlayerState* LMSPlayerState = GetPlayerState<ALMS_TeamProjectPlayerState>())
		{
			LMSPlayerState->SetSelectedWeaponID(WeaponID);
		}

		ALMS_TeamProjectCharacter* LMSCharacter = Cast<ALMS_TeamProjectCharacter>(GetPawn());
		if (LMSCharacter)
		{
			if (ULMSWeaponComponent* WeaponComponent = LMSCharacter->GetWeaponComponent())
			{
				bSuccess = WeaponComponent->EquipWeaponByID(WeaponID);
			}
		}
	}

	ClientHandleWeaponSelectionResult(bSuccess);
}

void ALMSPlayerController::ClientHandleWeaponSelectionResult_Implementation(bool bSuccess)
{
	if (bSuccess)
	{
		HideWeaponSelectionUI();
	}
}
