#include "ReviveAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayPrediction.h"
#include "InteractionDetectorComponent.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#include "AbilityTask_WaitReviveHold.h"
#include "LMS_TeamProjectCharacter.h"
#include "UI/LMSCombatHUDPresenterComponent.h"

UReviveAbility::UReviveAbility()
{
}

// ─────────────────────────────────────────────────────────────
// 활성화 : 클라/서버 경로 분기
//  - 클라 : 감지해둔 대상을 TargetData로 서버 전송 + 로컬 예측 시작
//  - 서버 : TargetData 수신 델리게이트 등록 후 대기
// ─────────────────────────────────────────────────────────────
void UReviveAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	ReviveTarget = nullptr;   // PerActor 재사용 대비 초기화

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (ActorInfo->IsLocallyControlled())
	{
		// 캐릭터가 트레이스로 잡아둔 대상 꺼내기
		ALMS_TeamProjectCharacter* Char =
			Cast<ALMS_TeamProjectCharacter>(ActorInfo->AvatarActor.Get());
		AActor* Target = Char ? Char->GetInteractionDetector()->GetCurrentTarget() : nullptr;

		if (!Target)
		{
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}

		// TargetData 포장 (힙 할당 필수 — 스택 할당 시 크래시)
		FGameplayAbilityTargetDataHandle TargetDataHandle;
		FGameplayAbilityTargetData_ActorArray* TData =
			new FGameplayAbilityTargetData_ActorArray();
		TData->TargetActorArray.Add(Target);
		TargetDataHandle.Add(TData);

		// 서버로 복제 전송 (예측 윈도우 안에서 → 예측키에 묶임)
		FScopedPredictionWindow ScopedPrediction(ASC);
		ASC->CallServerSetReplicatedTargetData(
			CurrentSpecHandle,
			CurrentActivationInfo.GetActivationPredictionKey(),
			TargetDataHandle,
			FGameplayTag(),
			ASC->ScopedPredictionKey);

		// 로컬도 즉시 진행 (호스트면 GE까지, 순수 클라면 예측 태스크만)
		StartRevive(Target);
	}
	else
	{
		// 서버는 클라가 보낸 TargetData를 델리게이트로 수신
		TargetDataDelegateHandle =
			ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle,
				CurrentActivationInfo.GetActivationPredictionKey())
			.AddUObject(this, &UReviveAbility::OnTargetDataReceived);

		// 등록 전에 이미 도착한 데이터가 있으면 즉시 처리
		ASC->CallReplicatedTargetDataDelegatesIfSet(
			CurrentSpecHandle,
			CurrentActivationInfo.GetActivationPredictionKey());
	}
}

// ─────────────────────────────────────────────────────────────
// 서버 : TargetData 수신 → 검증 → 시작
// ─────────────────────────────────────────────────────────────
void UReviveAbility::OnTargetDataReceived(
	const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

	if (!ASC)
	{
		return;
	}

	// RPC 버퍼 비우기 (다음 활성화 시 묵은 데이터 재사용 방지)
	ASC->ConsumeClientReplicatedTargetData(
		CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey());

	// TargetData에서 대상 추출 (TargetActorArray는 TWeakObjectPtr)
	AActor* Target = nullptr;
	if (Data.Data.IsValidIndex(0) && Data.Data[0].IsValid())
	{
		const FGameplayAbilityTargetData_ActorArray* ActorData =
			static_cast<const FGameplayAbilityTargetData_ActorArray*>(Data.Data[0].Get());
		if (ActorData && ActorData->TargetActorArray.Num() > 0)
		{
			Target = ActorData->TargetActorArray[0].Get();
		}
	}

	// 서버 권위 검증 — 실패 시 시작하지 않음
	if (!ValidateTarget(Target))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	StartRevive(Target);
}

// ─────────────────────────────────────────────────────────────
// 클라/서버 공통 : 홀드 태스크 시작 + BeingRevived(BleedOut 정지) 적용
// ─────────────────────────────────────────────────────────────
void UReviveAbility::StartRevive(AActor* Target)
{
	ReviveTarget = Target;

	// 서버 권위: 시전자 이동 봉인(Reviving) + 대상 BleedOut 정지(BeingRevived)
	if (K2_HasAuthority())
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
		{
			// [1] 시전자 자신 — Speed Override 0 (홀드 중 이동 봉인)
			if (RevivingEffect)
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				Context.AddSourceObject(this);

				FGameplayEffectSpecHandle Spec =
					ASC->MakeOutgoingSpec(RevivingEffect, 1.f, Context);
				if (Spec.IsValid())
				{
					RevivingHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
				}
			}

			// [2] 대상 — state.BeingRevived 부여 (BleedOut Ongoing 조건 위반 → 출혈 정지)
			UAbilitySystemComponent* TargetASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ReviveTarget);

			if (BeingRevivedEffect && TargetASC)
			{
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				Context.AddSourceObject(this);

				FGameplayEffectSpecHandle Spec =
					ASC->MakeOutgoingSpec(BeingRevivedEffect, 1.f, Context);
				if (Spec.IsValid())
				{
					BeingRevivedHandle =
						ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
				}
			}
		}
	}

	// 홀드 태스크 (클라 = 게이지 예측 / 서버 = 실제 완료 판정)
	UAbilityTask_WaitReviveHold* Task =
		UAbilityTask_WaitReviveHold::WaitReviveHold(
			this, ReviveTarget, ReviveDuration, MaxReviveDistance, 0.1f);

	Task->OnCompleted.AddDynamic(this, &UReviveAbility::OnReviveCompleted);
	Task->OnCancelled.AddDynamic(this, &UReviveAbility::OnReviveCancelled);
	Task->OnProgress.AddDynamic(this, &UReviveAbility::HandleReviveProgress);
	Task->ReadyForActivation();

	// 로컬 시전자 화면에 부활 게이지 표시
	if (ULMSCombatHUDPresenterComponent* CombatHUDPresenter = GetCombatHUDPresenter())
	{
		CombatHUDPresenter->ShowInteractionProgress();
		CombatHUDPresenter->SetInteractionProgressValue(0.f);
	}
}

// ─────────────────────────────────────────────────────────────
// 서버 검증 : 거리 + 다운 상태 태그
// ─────────────────────────────────────────────────────────────
bool UReviveAbility::ValidateTarget(AActor* Target) const
{
	if (!Target)
	{
		return false;
	}

	AActor* Rescuer = GetAvatarActorFromActorInfo();
	if (!Rescuer)
	{
		return false;
	}

	// 거리 — 서버만 아는 권위 검증
	if (FVector::Dist(Rescuer->GetActorLocation(), Target->GetActorLocation()) > MaxReviveDistance)
	{
		return false;
	}

	// 인터랙터블인가 + 상호작용 가능한 상태인가
	if (!Target->Implements<ULMSInteractableInterface>())
	{
		return false;
	}
	if (!ILMSInteractableInterface::Execute_CanInteract(Target, Rescuer))
	{
		return false;
	}

	//시야 헬퍼 함수 
	if (!UInteractionDetectorComponent::HasLineOfSight(GetWorld(), Rescuer, Target))
	{
		return false;
	}

	return true;
}

// ─────────────────────────────────────────────────────────────
// 홀드 완주 : 서버에서 GE_Revive 적용
// ─────────────────────────────────────────────────────────────
void UReviveAbility::OnReviveCompleted()
{
	if (K2_HasAuthority())
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		UAbilitySystemComponent* TargetASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ReviveTarget);

		if (TargetASC && ReviveEffect)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);

			FGameplayEffectSpecHandle Spec =
				ASC->MakeOutgoingSpec(ReviveEffect, 1.f, Context);
			if (Spec.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

// ─────────────────────────────────────────────────────────────
// 조건 이탈 / 입력 해제 : 취소
// ─────────────────────────────────────────────────────────────
void UReviveAbility::OnReviveCancelled()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

// ─────────────────────────────────────────────────────────────
// 홀드 진행률 → 로컬 시전자 HUD 게이지 (0~1)
// ─────────────────────────────────────────────────────────────
void UReviveAbility::HandleReviveProgress(float Progress)
{
	if (ULMSCombatHUDPresenterComponent* CombatHUDPresenter = GetCombatHUDPresenter())
	{
		CombatHUDPresenter->ShowInteractionProgress();
		CombatHUDPresenter->SetInteractionProgressValue(Progress);
	}
}

// ─────────────────────────────────────────────────────────────
// 로컬 시전자의 HUD Presenter 얻기 (AvatarActor의 로컬 컨트롤러 경유)
// ─────────────────────────────────────────────────────────────
ULMSCombatHUDPresenterComponent* UReviveAbility::GetCombatHUDPresenter() const
{
	APawn* Pawn = Cast<APawn>(GetAvatarActorFromActorInfo());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return nullptr;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	if (!PlayerController)
	{
		return nullptr;
	}

	return PlayerController->FindComponentByClass<ULMSCombatHUDPresenterComponent>();
}

void UReviveAbility::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 손 떼면 부활 취소 (태스크는 EndAbility → OnDestroy로 정리됨)
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}

// ─────────────────────────────────────────────────────────────
// 종료 : 델리게이트 해제 + BeingRevived 제거
// ─────────────────────────────────────────────────────────────
void UReviveAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// 로컬 시전자 게이지 숨김 (완료/취소/중단 모두)
	if (ULMSCombatHUDPresenterComponent* CombatHUDPresenter = GetCombatHUDPresenter())
	{
		CombatHUDPresenter->HideInteractionProgress();
	}

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	// 서버가 등록한 TargetData 수신 델리게이트 해제 (재활성화 시 중복 바인딩 방지)
	if (TargetDataDelegateHandle.IsValid())
	{
		if (ASC)
		{
			ASC->AbilityTargetDataSetDelegate(
				CurrentSpecHandle, CurrentActivationInfo.GetActivationPredictionKey())
				.Remove(TargetDataDelegateHandle);
		}
		TargetDataDelegateHandle.Reset();
	}

	// 홀드 중 붙였던 GE 정리 (취소든 완료든 반드시)
	if (HasAuthority(&ActivationInfo))
	{
		// [1] 시전자 이동 봉인 해제 — 자기 ASC
		if (ASC && RevivingHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(RevivingHandle);
			RevivingHandle = FActiveGameplayEffectHandle();
		}

		// [2] 대상 BeingRevived 해제 — 대상 ASC (안 하면 BleedOut 영구 정지)
		if (BeingRevivedHandle.IsValid())
		{
			if (UAbilitySystemComponent* TargetASC =
				UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ReviveTarget))
			{
				TargetASC->RemoveActiveGameplayEffect(BeingRevivedHandle);
			}
			BeingRevivedHandle = FActiveGameplayEffectHandle();
		}
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}