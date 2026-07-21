#include "LMSDamageLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"

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

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddSourceObject(Source);

	FGameplayEffectSpecHandle Spec =
		SourceASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);

	if (Spec.IsValid())
	{
		static const FGameplayTag DamageTag =
			FGameplayTag::RequestGameplayTag(FName("Data.Damage"));
		Spec.Data->SetSetByCallerMagnitude(DamageTag, Damage);
		SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	}
}