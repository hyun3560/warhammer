#include "LMSDamageLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "GameFramework/Controller.h"
#include "LMS_TeamProjectCharacter.h"
#include "Weapons/LMSWeaponComponent.h"

namespace
{
constexpr float BlockingDamageMultiplier = 0.2f;
constexpr float BlockingFacingDotThreshold = 0.5f;

bool IsBlockedByFacingShield(const ALMS_TeamProjectCharacter* TargetCharacter, const AActor* DamageSource)
{
	if (!TargetCharacter || !DamageSource)
	{
		return false;
	}

	const ULMSWeaponComponent* WeaponComponent = TargetCharacter->GetWeaponComponent();
	if (!WeaponComponent || !WeaponComponent->IsBlocking())
	{
		return false;
	}

	const FVector ToDamageSource = (DamageSource->GetActorLocation() - TargetCharacter->GetActorLocation()).GetSafeNormal2D();
	if (ToDamageSource.IsNearlyZero())
	{
		return false;
	}

	const AController* Controller = TargetCharacter->GetController();
	const FRotator FacingRotation = Controller ? Controller->GetControlRotation() : TargetCharacter->GetActorRotation();
	const FVector FacingDirection = FacingRotation.Vector().GetSafeNormal2D();
	const float FacingDot = FVector::DotProduct(FacingDirection, ToDamageSource);
	return FacingDot >= BlockingFacingDotThreshold;
}
}

void ULMSDamageLibrary::ApplyDamageEffect(
	AActor* Source, AActor* Target, float Damage, TSubclassOf<UGameplayEffect> DamageEffect)
{
	if (!Source || !Target || !DamageEffect)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Source);
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

	if (!SourceASC || !TargetASC)
	{
		return;
	}

	float FinalDamage = Damage;
	if (const ALMS_TeamProjectCharacter* TargetCharacter = Cast<ALMS_TeamProjectCharacter>(Target))
	{
		if (IsBlockedByFacingShield(TargetCharacter, Source))
		{
			FinalDamage *= BlockingDamageMultiplier;
		}
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(Source);

	FGameplayEffectSpecHandle Spec =
		SourceASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);

	if (Spec.IsValid())
	{
		static const FGameplayTag DamageTag =
			FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
		Spec.Data->SetSetByCallerMagnitude(DamageTag, FinalDamage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}
