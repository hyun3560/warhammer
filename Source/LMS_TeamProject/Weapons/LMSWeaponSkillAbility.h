#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponGameplayAbility.h"
#include "LMSWeaponSkillAbility.generated.h"

class ULMSWeaponComponent;

UCLASS(Abstract, Blueprintable)
class LMS_TEAMPROJECT_API ULMSWeaponSkillAbility : public ULMSWeaponGameplayAbility
{
	GENERATED_BODY()

public:
	ULMSWeaponSkillAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Ability")
	bool ExecuteWeaponSkill(ULMSWeaponComponent* WeaponComponent);
	virtual bool ExecuteWeaponSkill_Implementation(ULMSWeaponComponent* WeaponComponent);
};
