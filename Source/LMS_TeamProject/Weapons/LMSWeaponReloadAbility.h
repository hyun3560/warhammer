#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponGameplayAbility.h"
#include "LMSWeaponReloadAbility.generated.h"

UCLASS(Blueprintable)
class LMS_TEAMPROJECT_API ULMSWeaponReloadAbility : public ULMSWeaponGameplayAbility
{
	GENERATED_BODY()

public:
	ULMSWeaponReloadAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
