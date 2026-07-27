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
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
