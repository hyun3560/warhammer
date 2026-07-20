#include "LMSRifleSkillAbility.h"

#include "LMSWeaponComponent.h"

bool ULMSRifleSkillAbility::ExecuteWeaponSkill_Implementation(ULMSWeaponComponent* WeaponComponent)
{
	if (!WeaponComponent || !WeaponComponent->TryConsumeAmmo(AmmoCost, true))
	{
		return false;
	}

	WeaponComponent->FireRangedShot(DamageMultiplier, RangeMultiplier, bDrawDebugTrace);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Rifle skill: Ammo=%d/%d DamageMultiplier=%.1f"),
		WeaponComponent->GetAmmoInMagazine(),
		WeaponComponent->GetReserveAmmo(),
		DamageMultiplier);

	return true;
}
