#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponGameplayAbility.h"
#include "LMSWeaponPrimaryAbility.generated.h"

UCLASS(Blueprintable)
class LMS_TEAMPROJECT_API ULMSWeaponPrimaryAbility : public ULMSWeaponGameplayAbility
{
	GENERATED_BODY()

public:
	ULMSWeaponPrimaryAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
