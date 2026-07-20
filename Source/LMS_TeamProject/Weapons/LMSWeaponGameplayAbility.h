#pragma once

#include "CoreMinimal.h"
#include "../LMSGameplayAbility.h"
#include "LMSWeaponGameplayAbility.generated.h"

class ULMSWeaponComponent;

UCLASS(Abstract, Blueprintable)
class LMS_TEAMPROJECT_API ULMSWeaponGameplayAbility : public ULMSGameplayAbility
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ability")
	ULMSWeaponComponent* GetWeaponComponentFromActorInfo() const;
};
