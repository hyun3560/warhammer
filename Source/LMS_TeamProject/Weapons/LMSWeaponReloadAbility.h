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

	// 재장전 로직(Reload) 실행 직후 호출된다. 블루프린트에서 재장전 몽타주 재생 등을 구현한다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Weapon|Reload")
	void PlayReloadMontage();
};
