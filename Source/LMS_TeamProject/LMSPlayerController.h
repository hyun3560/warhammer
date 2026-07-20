#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LMSPlayerController.generated.h"


class UUIManagerComponent;
class ULMSCombatHUDPresenterComponent;
class UIndicatorManagerComponent;
class ULMSTeamStatusComponent;

UCLASS()
class LMS_TEAMPROJECT_API ALMSPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ALMSPlayerController();

	// 무기 선택 UI의 버튼에서 호출하는 함수입니다.
	// 예: Rifle 버튼은 "Rifle", Hammer 버튼은 "Hammer"를 넘깁니다.
	UFUNCTION(BlueprintCallable, Category = "Weapon Selection")
	void RequestWeaponSelection(FName WeaponID);

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;

	// 무기 선택 UI를 화면에 표시하고 마우스 입력을 UI에 사용할 수 있게 합니다.
	void ShowWeaponSelectionUI();

	// 무기 선택 UI를 숨기고 입력을 다시 게임 조작 모드로 되돌립니다.
	void HideWeaponSelectionUI();

	// 클라이언트가 선택한 무기 ID를 서버로 전달합니다.
	// 실제 무기 장착은 반드시 서버에서 처리해야 합니다.
	UFUNCTION(Server, Reliable)
	void ServerRequestWeaponSelection(FName WeaponID);

	// 서버에서 무기 선택 처리 결과를 클라이언트에게 알려줍니다.
	// 성공하면 선택 UI를 닫고, 실패하면 UI를 유지합니다.
	UFUNCTION(Client, Reliable)
	void ClientHandleWeaponSelectionResult(bool bSuccess);

private:
	// 메인 UI, 전투 UI, 리스폰 UI, 일시정지 UI 등의 생성/표시/숨김 처리하는 UI 관리자 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LMS|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UUIManagerComponent> UIManagerComponent;

	// 전투 HUD에 필요한 게임 데이터를 실제 위젯으로 전달하는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LMS|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULMSCombatHUDPresenterComponent> CombatHUDPresenterComponent;

	// 팀원/적/목표/핑 위치의 오프스크린 인디케이터 좌표를 계산하는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LMS|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UIndicatorManagerComponent> IndicatorManagerComponent;

	// 팀원 목록과 팀원 HP/Shield 값을 HUD 팀원 슬롯에 전달하는 컴포넌트입니다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "LMS|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULMSTeamStatusComponent> TeamStatusComponent;
};
