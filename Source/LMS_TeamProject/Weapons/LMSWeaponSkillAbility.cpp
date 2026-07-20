#include "LMSWeaponSkillAbility.h"

#include "LMSWeaponComponent.h"

ULMSWeaponSkillAbility::ULMSWeaponSkillAbility()
{
	AbilityInputID = ELMSAbilityInputID::WeaponSkill;
}

void ULMSWeaponSkillAbility::ActivateAbility(
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
	if (WeaponComponent)
	{
		float CurrentCooldown = 0.f;
		float MaxCooldown = 0.f;
		GetCooldownTimeRemainingAndDuration(Handle, ActorInfo, CurrentCooldown, MaxCooldown);
		WeaponComponent->StartSkillCooldown(CurrentCooldown, MaxCooldown);
	}

	if (!WeaponComponent || !ExecuteWeaponSkill(WeaponComponent))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool ULMSWeaponSkillAbility::ExecuteWeaponSkill_Implementation(ULMSWeaponComponent* WeaponComponent)
{
	return false;
}
