#include "LMSWeaponGameplayAbility.h"

#include "GameFramework/Actor.h"
#include "LMSWeaponComponent.h"

ULMSWeaponComponent* ULMSWeaponGameplayAbility::GetWeaponComponentFromActorInfo() const
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->FindComponentByClass<ULMSWeaponComponent>() : nullptr;
}
