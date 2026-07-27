#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "LMSPlayerController.generated.h"


class UUIManagerComponent;
class ULMSCombatHUDPresenterComponent;
class UIndicatorManagerComponent;
class ULMSTeamStatusComponent;
class ULMSMainMenuWidget;
class UAbilitySystemComponent;
class UCameraComponent;
class UPrimitiveComponent;
class ACameraActor;
struct FGameplayTag;

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

	UFUNCTION(BlueprintCallable, Category = "LMS|Menu")
	void HostGameFromMainMenu();

	UFUNCTION(BlueprintCallable, Category = "LMS|Menu")
	void JoinGameFromMainMenu(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "LMS|Menu")
	void FindGameFromMainMenu();

	UFUNCTION(BlueprintCallable, Category = "LMS|Menu")
	void QuitGameFromMainMenu();

protected:
	virtual void BeginPlay() override;
	virtual void OnRep_PlayerState() override;
	virtual void AcknowledgePossession(APawn* P) override;

	void InitializeGameplayUI();
	void ShowStartupMenu();
	void HideStartupMenu();
	void BeginGameplayIntro();
	void TryBlendFromMenuCameraToPawn();
	void FinishGameplayIntro();
	ACameraActor* FindOrSpawnMenuCamera();
	void BindGroggyCameraState();
	void UnbindGroggyCameraState();
	void UpdateGroggyCameraState();
	void HandleGroggyCameraTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void EnterGroggyCamera();
	void ExitGroggyCamera();
	void SetLocalCharacterThirdPersonCameraEnabled(bool bEnabled);
	void ApplyGroggyMeshVisibility(class ALMS_TeamProjectCharacter* LMSCharacter);
	void RestorePreGroggyMeshVisibility();
	bool IsFirstPersonViewPrimitiveComponent(const UPrimitiveComponent* PrimitiveComponent, const UPrimitiveComponent* ThirdPersonMesh) const;
	IOnlineSessionPtr GetOnlineSessionInterface() const;
	void CreateMainMenuSession();
	void OpenMainMenuListenLevel() const;
	void OnCreateMainMenuSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnDestroyMainMenuSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindMainMenuSessionsComplete(bool bWasSuccessful);
	void OnJoinMainMenuSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void JoinFirstFoundMainMenuSession();
	FString NormalizeMainMenuConnectString(const FString& ConnectString) const;
	void SetMainMenuStatusMessage(const FText& Message) const;

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

	UPROPERTY()
	TObjectPtr<ULMSMainMenuWidget> MainMenuWidget;

	UPROPERTY()
	TObjectPtr<ACameraActor> RuntimeMenuCamera;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedGroggyCameraAbilitySystemComponent;

	UPROPERTY()
	TArray<TObjectPtr<UCameraComponent>> PreGroggyActiveCameraComponents;

	struct FGroggyPrimitiveVisibilityState
	{
		TWeakObjectPtr<UPrimitiveComponent> PrimitiveComponent;
		bool bHiddenInGame = false;
		bool bOwnerNoSee = false;
		bool bOnlyOwnerSee = false;
	};

	TArray<FGroggyPrimitiveVisibilityState> PreGroggyPrimitiveVisibilityStates;

	FTimerHandle GameplayIntroRetryTimerHandle;
	FTimerHandle GameplayIntroFinishTimerHandle;
	FDelegateHandle GroggyCameraTagDelegateHandle;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FOnlineSessionSearchResult PendingJoinSessionSearchResult;

	int32 GameplayIntroRetryCount = 0;
	bool bUsingGroggyCamera = false;
	bool bHasPendingJoinSessionSearchResult = false;
};
