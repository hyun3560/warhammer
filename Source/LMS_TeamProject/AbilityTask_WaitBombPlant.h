#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitBombPlant.generated.h"

class ABombSite;
class UGameplayAbility;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBombPlantProgress, float, Progress); // 진행률을 알려주는 이벤트 타입
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBombPlantEvent); // 완료/취소처럼 값 없이 발생만 알리는 이벤트 타입

UCLASS()
class LMS_TEAMPROJECT_API UAbilityTask_WaitBombPlant : public UAbilityTask
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Ability|Tasks",
        meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
    static UAbilityTask_WaitBombPlant* WaitBombPlant(
        UGameplayAbility* OwningAbility, // 이 태스크를 소유하는 어빌리티
        ABombSite* BombSite, // 설치할 폭탄 지점
        float Duration, // 설치에 필요한 시간
        float MaxDistance, // 설치 중 유지해야 하는 최대 거리
        float CheckInterval = 0.1f); // 몇 초마다 조건을 검사할지

    virtual void Activate() override; // 태스크가 실제로 시작될 때 호출되는 함수

    // 설치가 끝까지 완료되면 호출할 이벤트
    UPROPERTY(BlueprintAssignable)
    FBombPlantEvent OnCompleted;

    // 설치가 중간에 취소되면 호출할 이벤트
    UPROPERTY(BlueprintAssignable)
    FBombPlantEvent OnCancelled;

    // 설치 진행률을 알려주는 이벤트
    UPROPERTY(BlueprintAssignable)
    FBombPlantProgress OnProgress;

protected:
    virtual void OnDestroy(bool bInOwnerFinished) override;

private:
    void CheckTick(); // 0.1초마다 실행될 함수
    bool ValidateConditions() const; // 설치 유지 조건을 검사하는 함수

private:
    // 현재 설치 중인 폭탄 지점
    UPROPERTY()
    TObjectPtr<ABombSite> TargetBombSite = nullptr; 

    float HoldDuration = 5.0f; // 설치에 필요한 총 시간
    float MaxDist = 300.0f; // 설치 중 유지해야 하는 최대 거리
    float Interval = 0.1f; // 조건 검사 주기
    float Elapsed = 0.0f; // 지금까지 누적된 설치 시간

    FTimerHandle CheckTimerHandle;
};