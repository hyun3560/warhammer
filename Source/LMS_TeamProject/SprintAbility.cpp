#include "SprintAbility.h"
#include "AbilitySystemComponent.h"
#include "LMSAttributeSet.h"

USprintAbility::USprintAbility()
{
	// 부모(LMSGameplayAbility)에서 이미 InstancedPerActor + LocalPredicted 세팅됨
}

void USprintAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// 코스트/쿨다운 통과 못 하면 여기서 자동 종료
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC || !SprintEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// GE 적용 (서버에서만 실제 적용됨 — Speed는 복제로 클라 반영)
	if (HasAuthority(&ActivationInfo))
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);

		FGameplayEffectSpecHandle SpecHandle =
			ASC->MakeOutgoingSpec(SprintEffectClass, 1.f, Context);

		if (SpecHandle.IsValid())
		{
			ActiveSprintHandle =
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}

		// ② 스태미나 소모
		if (SprintCostEffectClass)
		{
			FGameplayEffectSpecHandle CostSpec =
				ASC->MakeOutgoingSpec(SprintCostEffectClass, 1.f, Context);
			if (CostSpec.IsValid())
			{
				ActiveCostHandle = ASC->ApplyGameplayEffectSpecToSelf(*CostSpec.Data.Get());
			}
		}
	}

	// ③ 스태미나 감시 구독 (서버/클라 모두)
	if (const ULMSAttributeSet* AS = ASC->GetSet<ULMSAttributeSet>())
	{
		StaminaChangedDelegateHandle =
			ASC->GetGameplayAttributeValueChangeDelegate(AS->GetStaminaAttribute())
			.AddUObject(this, &USprintAbility::OnStaminaChanged);
	}

	
}

void USprintAbility::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	// 키 떼면 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void USprintAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	// 스태미나 구독 해제
	if (ASC && StaminaChangedDelegateHandle.IsValid())
	{
		if (const ULMSAttributeSet* AS = ASC->GetSet<ULMSAttributeSet>())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(AS->GetStaminaAttribute())
				.Remove(StaminaChangedDelegateHandle);
		}
		StaminaChangedDelegateHandle.Reset();
	}
	// GE 제거 (서버에서 적용했으니 서버에서 제거)
	if (HasAuthority(&ActivationInfo) && ASC)
	{
		if (ActiveSprintHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveSprintHandle);
			ActiveSprintHandle = FActiveGameplayEffectHandle();
		}
		if (ActiveCostHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(ActiveCostHandle);
			ActiveCostHandle = FActiveGameplayEffectHandle();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USprintAbility::OnStaminaChanged(const FOnAttributeChangeData& Data)
{
	// 스태미나 0 이하 → Sprint 종료
	if (Data.NewValue <= 0.f)
	{
		if (IsActive())
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		}
	}
}