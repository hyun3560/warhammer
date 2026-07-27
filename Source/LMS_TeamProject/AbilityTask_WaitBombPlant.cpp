#include "AbilityTask_WaitBombPlant.h"

#include "Objectives/BombSite.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

// 태스크 객체 생성, 필요한 값 저장, 태스크 반환
UAbilityTask_WaitBombPlant* UAbilityTask_WaitBombPlant::WaitBombPlant(
    UGameplayAbility* OwningAbility,
    ABombSite* BombSite,
    float Duration,
    float MaxDistance,
    float CheckInterval)
{
    UAbilityTask_WaitBombPlant* Task =
        NewAbilityTask<UAbilityTask_WaitBombPlant>(OwningAbility);

    Task->TargetBombSite = BombSite; // 설치할 폭탄 지점
    Task->HoldDuration = Duration; // 설치에 필요한 총 시간
    Task->MaxDist = MaxDistance; // 유지해야 하는 거리
    Task->Interval = CheckInterval; // 검사 주기
    Task->Elapsed = 0.0f; // 현재까지 누적 시간

    return Task;
}

void UAbilityTask_WaitBombPlant::Activate()
{
    if (!TargetBombSite || !Ability) // 시작 전 유효성 검사
    {
        OnCancelled.Broadcast();
        EndTask();
        return;
    }

    if (UWorld* World = GetWorld())
    {
        // 0.1초마다 CheckTick을 반복 실행
        World->GetTimerManager().SetTimer(
            CheckTimerHandle,
            this,
            &UAbilityTask_WaitBombPlant::CheckTick,
            Interval,
            true); 
    }
}

void UAbilityTask_WaitBombPlant::CheckTick()
{
    if (!ValidateConditions())
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnCancelled.Broadcast();
        }

        EndTask();
        return;
    }

    Elapsed += Interval;

    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnProgress.Broadcast(FMath::Clamp(Elapsed / HoldDuration, 0.0f, 1.0f)); // 값이 0보다 작거나 1보다 커지지 않게 제한
    }

    // 서버 보정
    // 클라이언트는 입력하자마자 바로 시작, 서버는 RPC/TargetData를 받고 시작
    // 서버 시작이 조금더 늦음
    // 해결방법 : 서버는 완료기준을 앞당긴다.
    // 결과 : HoldDuration = 5.0, Interval = 0.1, 서버 완료 기준 = 4.85
    const bool bServer = Ability && Ability->K2_HasAuthority();
    const float Threshold = bServer ? (HoldDuration - Interval * 1.5f) : HoldDuration;

    if (Elapsed >= Threshold)
    {
        if (ShouldBroadcastAbilityTaskDelegates())
        {
            OnCompleted.Broadcast();
        }

        EndTask();
    }
}

bool UAbilityTask_WaitBombPlant::ValidateConditions() const
{
    if (!TargetBombSite || !Ability)
    {
        return false;
    }

    const FGameplayAbilityActorInfo* Info = Ability->GetCurrentActorInfo();
    AActor* Installer = Info ? Info->AvatarActor.Get() : nullptr; // Installer = 폭탄을 설치하는 캐릭터
    if (!Installer)
    {
        return false;
    }

    if (TargetBombSite->IsBombPlanted())
    {
        return false;
    }

    const float Dist = FVector::Dist(
        Installer->GetActorLocation(),
        TargetBombSite->GetActorLocation());

    return Dist <= MaxDist;
}

// 태스크가 끝날 때 호출
void UAbilityTask_WaitBombPlant::OnDestroy(bool bInOwnerFinished)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CheckTimerHandle);
    }

    Super::OnDestroy(bInOwnerFinished);
}