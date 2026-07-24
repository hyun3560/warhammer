#include "RescueAllAbility.h"

#include "LMSRescueStation.h"
#include "LMS_TeamProjectCharacter.h"
#include "InteractionDetectorComponent.h"
#include "AbilityTask_WaitReviveHold.h"
#include "AbilitySystemComponent.h"
#include "GameplayPrediction.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "UI/LMSCombatHUDPresenterComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

URescueAllAbility::URescueAllAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	AbilityInputID = ELMSAbilityInputID::Interact_RescueAll;
}

// ─────────────────────────────────────────────────────────────
// 활성화 : 클라/서버 경로 분기 (Revive와 동일 구조)
// ─────────────────────────────────────────────────────────────
void URescueAllAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	TargetStation = nullptr;   // PerActor 재사용 대비 초기화

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (ActorInfo->IsLocallyControlled())
	{
		// 클라: 디텍터가 잡은 구조물을 서버에 알림
		ALMS_TeamProjectCharacter* Char =
			Cast<ALMS_TeamProjectCharacter>(ActorInfo->AvatarActor.Get());

		AActor* Detected = (Char && Char->GetInteractionDetector())
			? Char->GetInteractionDetector()->GetCurrentTarget()
			: nullptr;

		TargetStation = Cast<ALMSRescueStation>(Detected);

		if (!TargetStation)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		// TargetData 포장 (힙 할당 필수 — 스택 할당 시 크래시)
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		FGameplayAbilityTargetData_ActorArray* TData =
			new FGameplayAbilityTargetData_ActorArray();
		TData->TargetActorArray.Add(TargetStation);
		TargetDataHandle.Add(TData);

		// 서버로 복제 전송 (예측 윈도우 안에서 → 예측키에 묶임)
		FScopedPredictionWindow ScopedPrediction(ASC);
		ASC->CallServerSetReplicatedTargetData(
			CurrentSpecHandle,
			CurrentActivationInfo.GetActivationPredictionKey(),
			TargetDataHandle,
			FGameplayTag(),
			ASC->ScopedPredictionKey);

		// 로컬도 즉시 진행 (게이지 예측용)
		StartHold();
	}
	else
	{
		// 서버는 클라가 보낸 TargetData를 델리게이트로 수신
		TargetDataDelegateHandle =
			ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle,
				CurrentActivationInfo.GetActivationPredictionKey())
			.AddUObject(this, &URescueAllAbility::OnTargetDataReceived);

		// 등록 전에 이미 도착한 데이터가 있으면 즉시 처리
		ASC->CallReplicatedTargetDataDelegatesIfSet(
			CurrentSpecHandle,
			CurrentActivationInfo.GetActivationPredictionKey());
	}
}

// ─────────────────────────────────────────────────────────────
// 서버 : TargetData 수신 → 검증 → 시작
// ─────────────────────────────────────────────────────────────
void URescueAllAbility::OnTargetDataReceived(
	const FGameplayAbilityTargetDataHandle& Data, FGameplayTag Tag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	// RPC 버퍼 비우기 (다음 활성화 시 묵은 데이터 재사용 방지)
	ASC->ConsumeClientReplicatedTargetData(
		CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

	// TargetData에서 구조물 추출
	if (Data.Data.IsValidIndex(0) && Data.Data[0].IsValid())
	{
		const FGameplayAbilityTargetData_ActorArray* ActorData =
			static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data.Data[0].Get());

		if (ActorData && ActorData->TargetActorArray.Num() > 0)
		{
			TargetStation = Cast<ALMSRescueStation>(ActorData->TargetActorArray[0].Get());
		}
	}

	// 서버 권위 검증
	AActor* Rescuer = GetAvatarActorFromActorInfo();
	if (!TargetStation || !Rescuer ||
		FVector::Dist(Rescuer->GetActorLocation(), TargetStation->GetActorLocation()) > MaxInteractDistance ||
		!ILMSInteractableInterface::Execute_CanInteract(TargetStation, Rescuer))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	StartHold();
}

// ─────────────────────────────────────────────────────────────
// 클라/서버 공통 : 홀드 태스크 시작
// ─────────────────────────────────────────────────────────────
void URescueAllAbility::StartHold()
{
	if (!TargetStation)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	// 거리/시야/타이밍은 태스크가 담당.
	// 추가 조건은 주입하지 않음 — 구조물의 CanInteract는 전체 액터 순회라
	// 0.1초 주기 호출이 부담이고, 시작 시점 검증으로 충분하다.
	UAbilityTask_WaitReviveHold* Task =
		UAbilityTask_WaitReviveHold::WaitReviveHold(
			this, TargetStation, HoldDuration, MaxInteractDistance, ValidationInterval);

	Task->OnCompleted.AddDynamic(this, &URescueAllAbility::OnHoldCompleted);
	Task->OnCancelled.AddDynamic(this, &URescueAllAbility::OnHoldCancelled);
	Task->OnProgress.AddDynamic(this, &URescueAllAbility::HandleHoldProgress);
	Task->ReadyForActivation();

	// 로컬 시전자 화면에 진행 게이지 표시
	if (ULMSCombatHUDPresenterComponent* HUD = GetCombatHUDPresenter())
	{
		HUD->ShowInteractionProgress();
		HUD->SetInteractionProgressValue(0.f);
	}
}

// ─────────────────────────────────────────────────────────────
// 홀드 완주 : 서버에서 전원 부활 실행
// ─────────────────────────────────────────────────────────────
void URescueAllAbility::OnHoldCompleted()
{
	if (K2_HasAuthority() && TargetStation)
	{
		TargetStation->PerformRescueAll();
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void URescueAllAbility::OnHoldCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void URescueAllAbility::HandleHoldProgress(float Progress)
{
	if (ULMSCombatHUDPresenterComponent* HUD = GetCombatHUDPresenter())
	{
		HUD->ShowInteractionProgress();
		HUD->SetInteractionProgressValue(Progress);
	}
}

// ─────────────────────────────────────────────────────────────
// 로컬 시전자의 HUD Presenter 얻기
// ─────────────────────────────────────────────────────────────
ULMSCombatHUDPresenterComponent* URescueAllAbility::GetCombatHUDPresenter() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
	return PC ? PC->FindComponentByClass<ULMSCombatHUDPresenterComponent>() : nullptr;
}

void URescueAllAbility::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 손 떼면 취소 (태스크는 EndAbility → OnDestroy로 정리됨)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

// ─────────────────────────────────────────────────────────────
// 종료 : 게이지 숨김 + 델리게이트 해제
// ─────────────────────────────────────────────────────────────
void URescueAllAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ULMSCombatHUDPresenterComponent* HUD = GetCombatHUDPresenter())
	{
		HUD->HideInteractionProgress();
	}

	// 서버가 등록한 TargetData 수신 델리게이트 해제 (재활성화 시 중복 바인딩 방지)
	if (TargetDataDelegateHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey())
				.Remove(TargetDataDelegateHandle);
		}
		TargetDataDelegateHandle.Reset();
	}

	TargetStation = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}