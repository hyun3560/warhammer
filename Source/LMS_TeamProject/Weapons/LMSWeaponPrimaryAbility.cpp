#include "LMSWeaponPrimaryAbility.h"

#include "LMSWeaponComponent.h"

ULMSWeaponPrimaryAbility::ULMSWeaponPrimaryAbility()
{
	AbilityInputID = ELMSAbilityInputID::PrimaryAttack;
}

void ULMSWeaponPrimaryAbility::ActivateAbility(
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
		WeaponComponent->StartAttack();
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
