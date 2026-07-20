#include "AbilityTask_WaitReviveHold.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Actor.h"

UAbilityTask_WaitReviveHold* UAbilityTask_WaitReviveHold::WaitReviveHold(
	UGameplayAbility* OwningAbility,
	AActor* ReviveTarget,
	float Duration,
	float MaxDistance,
	float CheckInterval)
{
	UAbilityTask_WaitReviveHold* Task =
		NewAbilityTask<UAbilityTask_WaitReviveHold>(OwningAbility);
	Task->Target = ReviveTarget;
	Task->HoldDuration = Duration;
	Task->MaxDist = MaxDistance;
	Task->Interval = CheckInterval;
	Task->Elapsed = 0.f;
	return Task;
}

void UAbilityTask_WaitReviveHold::Activate()
{
	// 시작 시점에 대상이 유효하지 않으면 즉시 취소
	if (!Target || !Ability)
	{
		OnCancelled.Broadcast();
		EndTask();
		return;
	}

	// Interval 주기로 CheckTick 반복
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CheckTimerHandle, this,
			&UAbilityTask_WaitReviveHold::CheckTick,
			Interval, true);
	}
}

void UAbilityTask_WaitReviveHold::CheckTick()
{
	// 1) 유지 조건 검사 — 하나라도 깨지면 취소
	if (!ValidateConditions())
	{
		if (ShouldBroadcastAbilityTaskDelegates())
		{
			OnCancelled.Broadcast();
		}
		EndTask();
		return;
	}

	// 2) 시간 누적
	Elapsed += Interval;

	// 3) 진행률 알림 (UI용)
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnProgress.Broadcast(FMath::Clamp(Elapsed / HoldDuration, 0.f, 1.f));
	}

	// 4) 완료 판정
	//    서버 태스크는 대상 RPC를 기다렸다 시작하므로 클라보다 한 틱 늦게 출발한다.
	//    그 시차 때문에 클라가 먼저 완주 → 손 뗌(EndAbility)이 서버로 전파되면
	//    서버 태스크가 완주 직전에 종료되어 부활이 안 되는 문제가 있었다.
	//    → 서버만 완료 결승선을 한 틱 반 앞당겨(HoldDuration - Interval*1.5) 클라를 따라잡게 한다.
	//    클라는 정확히 HoldDuration으로 판정해 게이지가 100%까지 차도록 둔다.
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

bool UAbilityTask_WaitReviveHold::ValidateConditions() const
{
	if (!Target || !Ability)
	{
		return false;
	}

	// 시전자(Rescuer) 위치
	const FGameplayAbilityActorInfo* Info = Ability->GetCurrentActorInfo();
	AActor* Rescuer = Info ? Info->AvatarActor.Get() : nullptr;
	if (!Rescuer)
	{
		return false;
	}

	// [1] 거리
	const float Dist = FVector::Dist(
		Rescuer->GetActorLocation(), Target->GetActorLocation());
	if (Dist > MaxDist)
	{
		return false;
	}

	// [2] 대상이 여전히 다운 상태(state.Incapacitated)인가
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC || !TargetASC->HasMatchingGameplayTag(
		FGameplayTag::RequestGameplayTag("state.Incapacitated")))
	{
		return false;
	}

	// [3] 벽 막힘 — 시전자와 대상 사이가 가려지면 취소 (불필요하면 이 블록만 제거)
	if (UWorld* World = GetWorld())
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(Rescuer);   // 대상은 무시하지 않음(맞아야 하므로)

		const bool bBlocked = World->LineTraceSingleByChannel(
			Hit,
			Rescuer->GetActorLocation(),
			Target->GetActorLocation(),
			ECC_Visibility, Params);

		// 뭔가에 막혔는데 그게 대상이 아니면 → 사이에 벽
		if (bBlocked && Hit.GetActor() != Target)
		{
			return false;
		}
	}

	return true;
}

void UAbilityTask_WaitReviveHold::OnDestroy(bool bInOwnerFinished)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);
	}
	Super::OnDestroy(bInOwnerFinished);
}