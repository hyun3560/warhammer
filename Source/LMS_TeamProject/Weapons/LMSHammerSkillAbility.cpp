#include "LMSHammerSkillAbility.h"

#include "LMSWeaponComponent.h"

bool ULMSHammerSkillAbility::ExecuteWeaponSkill_Implementation(ULMSWeaponComponent* WeaponComponent)
{
	if (!WeaponComponent)
	{
		return false;
	}

	WeaponComponent->ActivateMeleeDamageBoost(DamageMultiplier, BoostDuration, BoostedTraceCount);
	return true;
}
