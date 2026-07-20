#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponSkillAbility.h"
#include "LMSRifleSkillAbility.generated.h"

UCLASS(Blueprintable)
class LMS_TEAMPROJECT_API ULMSRifleSkillAbility : public ULMSWeaponSkillAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecuteWeaponSkill_Implementation(ULMSWeaponComponent* WeaponComponent) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	float DamageMultiplier = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	float RangeMultiplier = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	int32 AmmoCost = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	bool bDrawDebugTrace = true;
};
