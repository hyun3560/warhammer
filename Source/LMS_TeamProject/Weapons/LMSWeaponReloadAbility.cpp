#include "LMSWeaponReloadAbility.h"

#include "LMSWeaponComponent.h"

ULMSWeaponReloadAbility::ULMSWeaponReloadAbility()
{
	AbilityInputID = ELMSAbilityInputID::Reload;
}

void ULMSWeaponReloadAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ULMSWeaponComponent* WeaponComponent = GetWeaponComponentFromActorInfo())
	{
		WeaponComponent->Reload();

		// 재장전이 실제로 시작된 경우에만 블루프린트 몽타주 훅 호출
		if (WeaponComponent->IsReloading())
		{
			OnReloadStarted(WeaponComponent);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
