#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitReviveHold.generated.h"

// 진행률 알림 (UI 게이지용), Progress = 0~1
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FReviveHoldProgress, float, Progress);
// 완료 / 취소 알림 (파라미터 없음)
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FReviveHoldEvent);

/**
 * 부활 홀드 태스크.
 *  - 주기(Interval)마다 유지 조건을 재검사하고, 누적 시간이 HoldDuration에 도달하면 완료.
 *  - 조건이 하나라도 깨지면 즉시 취소.
 *  - 클라/서버 양쪽에서 각자 실행된다 (LocalPredicted). 완료 판정은 서버가 권위.
 */
UCLASS()
class LMS_TEAMPROJECT_API UAbilityTask_WaitReviveHold : public UAbilityTask
{
	GENERATED_BODY()

public:
	// GA에서 호출하는 팩토리
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
		meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_WaitReviveHold* WaitReviveHold(
		UGameplayAbility* OwningAbility,
		AActor* ReviveTarget,
		float Duration,
		float MaxDistance,
		float CheckInterval = 0.1f);

	virtual void Activate() override;

	// GA가 결과를 받는 통로
	UPROPERTY(BlueprintAssignable)
	FReviveHoldEvent OnCompleted;    // 홀드 완주

	UPROPERTY(BlueprintAssignable)
	FReviveHoldEvent OnCancelled;    // 조건 이탈

	UPROPERTY(BlueprintAssignable)
	FReviveHoldProgress OnProgress;  // 진행률 (UI용)

protected:
	virtual void OnDestroy(bool bInOwnerFinished) override;

	// 주기 검사 (타이머 콜백)
	void CheckTick();

	// 유지 조건 검사 (거리 / 다운 태그 / 벽 막힘)
	bool ValidateConditions() const;

protected:
	UPROPERTY()
	TObjectPtr<AActor> Target = nullptr;

	float HoldDuration = 3.0f;   // 완주에 필요한 총 시간
	float MaxDist = 300.0f;      // 유지 가능한 최대 거리
	float Interval = 0.1f;       // 검사 주기
	float Elapsed = 0.0f;        // 누적 시간

	FTimerHandle CheckTimerHandle;
};