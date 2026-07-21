#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponGameplayAbility.h"
#include "LMSWeaponSecondaryAbility.generated.h"

class ULMSWeaponComponent;

UCLASS(Blueprintable)
class LMS_TEAMPROJECT_API ULMSWeaponSecondaryAbility : public ULMSWeaponGameplayAbility
{
	GENERATED_BODY()

public:
	ULMSWeaponSecondaryAbility();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Ability")
	void OnSecondaryStarted(ULMSWeaponComponent* WeaponComponent);

	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Ability")
	void OnSecondaryEnded(ULMSWeaponComponent* WeaponComponent, bool bWasCancelled);
};
