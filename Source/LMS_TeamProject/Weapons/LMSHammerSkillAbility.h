#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponSkillAbility.h"
#include "LMSHammerSkillAbility.generated.h"

UCLASS(Blueprintable)
class LMS_TEAMPROJECT_API ULMSHammerSkillAbility : public ULMSWeaponSkillAbility
{
	GENERATED_BODY()

protected:
	virtual bool ExecuteWeaponSkill_Implementation(ULMSWeaponComponent* WeaponComponent) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	float DamageMultiplier = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	float RangeMultiplier = 1.4f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	float TraceRadius = 140.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Skill")
	bool bDrawDebugTrace = true;
};
