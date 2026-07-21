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

	ULMSWeaponComponent* WeaponComponent = GetWeaponComponentFromActorInfo();
	if (!WeaponComponent || !ExecutePrimaryAttack(WeaponComponent))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool ULMSWeaponPrimaryAbility::ExecutePrimaryAttack_Implementation(ULMSWeaponComponent* WeaponComponent)
{
	if (!WeaponComponent)
	{
		return false;
	}

	WeaponComponent->StartAttack();
	return true;
}
