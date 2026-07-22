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

		// 실제로 재장전이 시작된 경우에만 블루프린트에서 몽타주를 재생한다.
		if (WeaponComponent->IsReloading())
		{
			PlayReloadMontage();
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
