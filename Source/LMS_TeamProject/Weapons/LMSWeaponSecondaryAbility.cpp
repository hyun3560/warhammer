#include "LMSWeaponSecondaryAbility.h"

#include "LMSWeaponComponent.h"

ULMSWeaponSecondaryAbility::ULMSWeaponSecondaryAbility()
{
	AbilityInputID = ELMSAbilityInputID::SecondaryAttack;
}

void ULMSWeaponSecondaryAbility::ActivateAbility(
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
		WeaponComponent->StartSecondaryAction();
	}
}

void ULMSWeaponSecondaryAbility::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void ULMSWeaponSecondaryAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ULMSWeaponComponent* WeaponComponent = GetWeaponComponentFromActorInfo())
	{
		WeaponComponent->StopSecondaryAction();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
