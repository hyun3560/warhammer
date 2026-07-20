#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LMSTeamStatusComponent.generated.h"

class ULMSCombatHUDPresenterComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LMS_TEAMPROJECT_API ULMSTeamStatusComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULMSTeamStatusComponent();

	// 로컬 PlayerController에서 팀원 상태 UI 갱신을 시작합니다.
	void InitializeTeamStatus();

private:
	// PlayerController에 붙어 있는 Combat HUD Presenter를 찾아 캐싱합니다.
	void CacheCombatHUDPresenter();

	// 현재 GameState의 PlayerArray를 기준으로 팀원 슬롯을 다시 구성합니다.
	void RefreshTeamStatus();

	UPROPERTY()
	TObjectPtr<ULMSCombatHUDPresenterComponent> CombatHUDPresenterComponent;

	FTimerHandle RefreshTimerHandle;

	// WBP_CombatHUD에 준비된 팀원 슬롯 개수입니다. 현재 0, 1, 2번 슬롯을 사용합니다.
	static constexpr int32 MaxTeamMemberSlots = 3;

	// 팀원 체력/쉴드 UI를 PlayerState 복제 타이밍에 맞춰 주기적으로 갱신합니다.
	static constexpr float RefreshInterval = 0.25f;
};
