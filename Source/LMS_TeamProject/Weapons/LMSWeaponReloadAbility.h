#pragma once

#include "CoreMinimal.h"
#include "LMSWeaponGameplayAbility.h"
#include "LMSWeaponReloadAbility.generated.h"

class ULMSWeaponComponent;

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

	// 재장전 시작 시 블루프린트에서 1인칭/3인칭 몽타주를 재생하도록 하는 훅
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Ability")
	void OnReloadStarted(ULMSWeaponComponent* WeaponComponent);
};
