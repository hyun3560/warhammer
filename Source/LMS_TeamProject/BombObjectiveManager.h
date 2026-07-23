#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BombObjectiveManager.generated.h"

UENUM(BlueprintType)
enum class EBombObjectiveState : uint8 // 정해진 선택지 목록
{
	Idle, // 아직 폭탄을 작동시키지 않은 상태
	ReturnCountdown, // 60초 안에 시작점으로 돌아가야 하는 상태
	ExplosionSequence, // 폭발 레벨시퀀스 재생 중
	Completed, // 성공 완료
	Failed // 실패 완료
};

UCLASS()
class LMS_TEAMPROJECT_API ABombObjectiveManager : public AActor
{
	GENERATED_BODY()

public:
	ABombObjectiveManager();

	// 불발탄을 수동 작동했을 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Bomb Objective")
	void StartBombObjective();

	// 플레이어가 시작점에 돌아왔을 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Bomb Objective")
	void NotifyPlayerReturnedToStart();

	// 폭발 레벨시퀀스가 끝났을 때 호출할 함수
	UFUNCTION(BlueprintCallable, Category = "Bomb Objective")
	void FinishExplosionSequence();

protected:
	// 게임이 시작되고 Actor가 레벨에 준비되면 호출
	virtual void BeginPlay() override;

	// 60초 타이머가 1초마다 호출할 함수
	void TickReturnCountdown();

	// 성공 처리 함수
	void HandleSuccess();

	// 실패 처리 함수
	void HandleFailure();

	// 폭발 레벨시퀀스 시작 함수
	void StartExplosionSequence(bool bSuccess);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bomb Objective")
	int32 ReturnTimeLimit = 60; // 플레이어가 시작점까지 돌아와야 하는 제한 시간

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomb Objective")
	int32 RemainingTime = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomb Objective")
	EBombObjectiveState CurrentState = EBombObjectiveState::Idle;

	bool bExplosionWasSuccess = false;

	FTimerHandle ReturnCountdownTimerHandle;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb Objective|UI")
	void OnReturnCountdownChanged(int32 NewRemainingTime);

	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb Objective|UI")
	void OnCountdownHidden();

	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb Objective|Sequence")
	void OnPlayExplosionSequence(bool bSuccess);

	UFUNCTION(BlueprintImplementableEvent, Category = "Bomb Objective|UI")
	void OnShowResultScreen(bool bSuccess);
};