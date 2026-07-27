#include "LMSWeaponSecondaryAbility.h"

#include "Animation/AnimMontage.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "LMSWeaponComponent.h"
#include "UObject/UnrealType.h"

ULMSWeaponSecondaryAbility::ULMSWeaponSecondaryAbility()
{
	AbilityInputID = ELMSAbilityInputID::SecondaryAttack;
}

void ULMSWeaponSecondaryAbility::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ULMSWeaponComponent* WeaponComponent = GetWeaponComponentFromActorInfo())
	{
		bDestroyedServerSideNonLocalBlockShieldEffect = false;
		WeaponComponent->StartSecondaryAction();
		OnSecondaryStarted(WeaponComponent);
		DestroyServerSideNonLocalBlockShieldEffect(ActorInfo);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void ULMSWeaponSecondaryAbility::InputReleased(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void ULMSWeaponSecondaryAbility::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ULMSWeaponComponent* WeaponComponent = GetWeaponComponentFromActorInfo())
	{
		WeaponComponent->StopSecondaryAction();
		OnSecondaryEnded(WeaponComponent, bWasCancelled);
		PlayServerSideNonLocalShieldEndMontage(WeaponComponent);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool ULMSWeaponSecondaryAbility::IsServerSideNonLocalAvatar(const FGameplayAbilityActorInfo* ActorInfo) const
{
	const AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : GetAvatarActorFromActorInfo();
	if (!AvatarActor || !AvatarActor->HasAuthority())
	{
		return false;
	}

	const APawn* AvatarPawn = Cast<APawn>(AvatarActor);
	if (AvatarPawn && AvatarPawn->IsLocallyControlled())
	{
		return false;
	}

	return true;
}

void ULMSWeaponSecondaryAbility::DestroyServerSideNonLocalBlockShieldEffect(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!IsServerSideNonLocalAvatar(ActorInfo))
	{
		return;
	}

	FObjectProperty* BlockShieldProperty = FindFProperty<FObjectProperty>(GetClass(), TEXT("BlockShieldPSC"));
	if (!BlockShieldProperty)
	{
		return;
	}

	UActorComponent* BlockShieldComponent = Cast<UActorComponent>(BlockShieldProperty->GetObjectPropertyValue_InContainer(this));
	if (!BlockShieldComponent)
	{
		return;
	}

	BlockShieldComponent->DestroyComponent();
	BlockShieldProperty->SetObjectPropertyValue_InContainer(this, nullptr);
	bDestroyedServerSideNonLocalBlockShieldEffect = true;
}

void ULMSWeaponSecondaryAbility::PlayServerSideNonLocalShieldEndMontage(ULMSWeaponComponent* WeaponComponent)
{
	if (!bDestroyedServerSideNonLocalBlockShieldEffect || !WeaponComponent || !IsServerSideNonLocalAvatar(nullptr))
	{
		return;
	}

	bDestroyedServerSideNonLocalBlockShieldEffect = false;

	FObjectProperty* ShieldMontageProperty = FindFProperty<FObjectProperty>(GetClass(), TEXT("ThirdPersonShieldMontage"));
	if (!ShieldMontageProperty)
	{
		return;
	}

	UAnimMontage* ShieldMontage = Cast<UAnimMontage>(ShieldMontageProperty->GetObjectPropertyValue_InContainer(this));
	if (!ShieldMontage)
	{
		return;
	}

	WeaponComponent->PlayReplicatedThirdPersonWeaponMontage(ShieldMontage, TEXT("shield_end"));
}
