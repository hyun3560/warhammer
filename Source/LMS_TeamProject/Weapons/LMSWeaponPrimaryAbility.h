#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponGameplayAbility.h"
#include "LMSWeaponPrimaryAbility.generated.h"

class ULMSWeaponComponent;

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

	UFUNCTION(BlueprintNativeEvent, Category = "Weapon|Ability")
	bool ExecutePrimaryAttack(ULMSWeaponComponent* WeaponComponent);
	virtual bool ExecutePrimaryAttack_Implementation(ULMSWeaponComponent* WeaponComponent);
};
