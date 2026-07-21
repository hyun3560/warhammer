#include "LMSPlayerController.h"

#include "UI/LMSCombatHUDPresenterComponent.h"
#include "UI/UIManagerComponent.h"
#include "UI/IndicatorManagerComponent.h"
#include "UI/LMSTeamStatusComponent.h"
#include "LMS_TeamProjectCharacter.h"
#include "LMS_TeamProjectPlayerState.h"
#include "Weapons/LMSWeaponComponent.h"

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

	ShowWeaponSelectionUI();
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