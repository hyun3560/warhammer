#include "RescueAllAbility.h"
#include "LMSRescueStation.h"
#include "LMS_TeamProjectCharacter.h"
#include "InteractionDetectorComponent.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "TimerManager.h"

URescueAllAbility::URescueAllAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityInputID = ELMSAbilityInputID::Interact_RescueAll;
}

void URescueAllAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{

	UE_LOG(LogTemp, Warning, TEXT("[RescueAll] ActivateAbility 진입"));
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ElapsedHold = 0.f;

	// 입력 떼면 취소
	if (UAbilityTask_WaitInputRelease* ReleaseTask =
		UAbilityTask_WaitInputRelease::WaitInputRelease(this, true))
	{
		ReleaseTask->OnRelease.AddDynamic(this, &URescueAllAbility::OnInputReleased);
		ReleaseTask->ReadyForActivation();
	}

	if (IsLocallyControlled())
	{
		// 클라: 디텍터가 잡은 구조물을 서버에 알림
		ALMS_TeamProjectCharacter* Char =
			Cast<ALMS_TeamProjectCharacter>(GetAvatarActorFromActorInfo());

		AActor* Detected = (Char && Char->GetInteractionDetector())
			? Char->GetInteractionDetector()->GetCurrentTarget()
			: nullptr;

		TargetStation = Cast<ALMSRescueStation>(Detected);

		if (!TargetStation)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		FGameplayAbilityTargetData_ActorArray* NewData =
			new FGameplayAbilityTargetData_ActorArray();
		NewData->TargetActorArray.Add(TargetStation);

		FGameplayAbilityTargetDataHandle DataHandle;
		DataHandle.Add(NewData);

		FScopedPredictionWindow ScopedPrediction(
			ActorInfo->AbilitySystemComponent.Get(),
			IsPredictingClient());

		ActorInfo->AbilitySystemComponent->CallServerSetReplicatedTargetData(
			Handle,
			ActivationInfo.GetActivationPredictionKey(),
			DataHandle,
			FGameplayTag(),
			ActorInfo->AbilitySystemComponent->ScopedPredictionKey);

		StartHold();   // 로컬은 즉시 시작 (진행도 UI용)
	}
	else
	{
		// 서버: TargetData 도착을 기다림
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

		TargetDataDelegateHandle =
			ASC->AbilityTargetDataSetDelegate(
				Handle, ActivationInfo.GetActivationPredictionKey())
			.AddUObject(this, &URescueAllAbility::OnTargetDataReceived);

		ASC->CallReplicatedTargetDataDelegatesIfSet(
			Handle, ActivationInfo.GetActivationPredictionKey());
	}
}

void URescueAllAbility::OnTargetDataReceived(
	const FGameplayAbilityTargetDataHandle& Data, FGameplayTag Tag)
{
	UE_LOG(LogTemp, Warning, TEXT("[RescueAll] TargetData 수신, Station=%s"), *GetNameSafe(TargetStation));
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		// 필수 — 소비 안 하면 다음 활성화 때 낡은 데이터가 남음
		ASC->ConsumeClientReplicatedTargetData(
			GetCurrentAbilitySpecHandle(),
			GetCurrentActivationInfo().GetActivationPredictionKey());
	}

	if (Data.Num() > 0)
	{
		const TArray<TWeakObjectPtr<AActor>> Actors = Data.Get(0)->GetActors();
		if (Actors.Num() > 0)
		{
			TargetStation = Cast<ALMSRescueStation>(Actors[0].Get());
		}
	}

	if (!TargetStation)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	StartHold();
}

void URescueAllAbility::StartHold()
{
	UE_LOG(LogTemp, Warning, TEXT("[RescueAll] StartHold, Authority=%d"), HasAuthority(&CurrentActivationInfo));
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	Avatar->GetWorldTimerManager().SetTimer(
		HoldTimerHandle, this, &URescueAllAbility::TickHold,
		ValidationInterval, true);
}

void URescueAllAbility::TickHold()
{
	UE_LOG(LogTemp, Warning, TEXT("[RescueAll] Tick %.1f"), ElapsedHold);
	AActor* Avatar = GetAvatarActorFromActorInfo();

	// 구조물이 사라졌거나 조건이 깨지면 취소
	if (!Avatar || !TargetStation ||
		!ILMSInteractableInterface::Execute_CanInteract(TargetStation, Avatar))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	// 거리 이탈 시 취소
	const float DistSq = FVector::DistSquared(
		Avatar->GetActorLocation(), TargetStation->GetActorLocation());

	if (DistSq > FMath::Square(MaxInteractDistance))
	{
		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
		return;
	}

	ElapsedHold += ValidationInterval;

	const bool bServer = HasAuthority(&CurrentActivationInfo);
	const float Threshold = bServer
		? (HoldDuration - ValidationInterval * 1.5f)
		: HoldDuration;

	if (ElapsedHold >= Threshold)
	{
		Avatar->GetWorldTimerManager().ClearTimer(HoldTimerHandle);

		if (bServer && TargetStation)
		{
			TargetStation->PerformRescueAll();
		}

		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void URescueAllAbility::OnInputReleased(float TimeHeld)
{
	CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
}

void URescueAllAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (AActor* Avatar = GetAvatarActorFromActorInfo())
	{
		Avatar->GetWorldTimerManager().ClearTimer(HoldTimerHandle);
	}

	// 서버: TargetData 델리게이트 정리
	if (TargetDataDelegateHandle.IsValid() && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->AbilityTargetDataSetDelegate(
			Handle, ActivationInfo.GetActivationPredictionKey())
			.Remove(TargetDataDelegateHandle);

		TargetDataDelegateHandle.Reset();
	}

	TargetStation = nullptr;
	ElapsedHold = 0.f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}