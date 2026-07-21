#include "LMSPlayerController.h"

#include "UI/LMSCombatHUDPresenterComponent.h"
#include "UI/UIManagerComponent.h"
#include "UI/IndicatorManagerComponent.h"

ALMSPlayerController::ALMSPlayerController()
{
	// PlayerController가 생성될 때 UI 관련 컴포넌트들도 함께 생성합니다.
	UIManagerComponent = CreateDefaultSubobject<UUIManagerComponent>(TEXT("UIManagerComponent"));
	CombatHUDPresenterComponent = CreateDefaultSubobject<ULMSCombatHUDPresenterComponent>(TEXT("CombatHUDPresenterComponent"));
	IndicatorManagerComponent = CreateDefaultSubobject<UIndicatorManagerComponent>(TEXT("IndicatorManagerComponent"));
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
}
