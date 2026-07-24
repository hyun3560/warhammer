// Copyright Epic Games, Inc. All Rights Reserved.

#include "LMSAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

ULMSAttributeSet::ULMSAttributeSet()
{
}

void ULMSAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, MaxShield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, Speed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, MaxSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, Wound, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, MaxWound, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, IncapHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ULMSAttributeSet, MaxIncapHealth, COND_None, REPNOTIFY_Always);

}

void ULMSAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxShield());
	}
	else if (Attribute == GetMaxShieldAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetSpeedAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxSpeed());
	}
	else if (Attribute == GetMaxSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetWoundAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxWound());
	}
	else if (Attribute == GetMaxWoundAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetIncapHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxIncapHealth());
	}
	else if (Attribute == GetMaxIncapHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
}

void ULMSAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// 1. 먼저 확정값 clamp (0 밑으로 안 가게)
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

		// 2. Health 0 → 델리게이트 브로드캐스트 (Post라 서버에서만 불림)
		if (GetHealth() <= 0.f)
		{
			OnHealthZero.Broadcast(Data);
		}
	}

	else if (Data.EvaluatedData.Attribute == GetIncapHealthAttribute())
	{
		SetIncapHealth(FMath::Clamp(GetIncapHealth(), 0.f, GetMaxIncapHealth()));
		if (GetIncapHealth() <= 0.f)
			OnIncapHealthZero.Broadcast(Data);
	}

	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}

	else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Clamp(GetShield(), 0.f, GetMaxShield()));
	}

	else if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		const float LocalDamage = GetDamage();
		SetDamage(0.f);

		static const FGameplayTag DeadTag =
			FGameplayTag::RequestGameplayTag(FName("state.Dead"));
		if (Data.Target.HasMatchingGameplayTag(DeadTag))
		{
			return;   // 죽은 대상은 데미지 무시
		}
		if (UAbilitySystemComponent* SourceASC =
			Data.EffectSpec.GetContext().GetOriginalInstigatorAbilitySystemComponent())
		{
			if (SourceASC != &Data.Target)   // 자기 자신은 통과
			{
				static const FGameplayTag TeamPlayerTag =
					FGameplayTag::RequestGameplayTag(FName("Team.Player"));

				if (SourceASC->HasMatchingGameplayTag(TeamPlayerTag) &&
					Data.Target.HasMatchingGameplayTag(TeamPlayerTag))
				{
					return;
				}
			}
		}
		if (LocalDamage <= 0.f) return;

		static const FGameplayTag IncapTag =
			FGameplayTag::RequestGameplayTag(FName("state.Incapacitated"));

		// 다운 중이면 IncapHealth로 직행 (쉴드 무시)
		if (Data.Target.HasMatchingGameplayTag(IncapTag))
		{
			SetIncapHealth(FMath::Clamp(GetIncapHealth() - LocalDamage, 0.f, GetMaxIncapHealth()));
			if (GetIncapHealth() <= 0.f)
			{
				OnIncapHealthZero.Broadcast(Data);
			}
			OnDamaged.Broadcast(Data);
			return;
		}

		// 평상시: 쉴드 → 체력, 넘친 건 버림
		float Remaining = LocalDamage;
		if (GetShield() > 0.f)
		{
			const float Absorbed = FMath::Min(GetShield(), Remaining);
			SetShield(GetShield() - Absorbed);
			Remaining -= Absorbed;
		}
		if (Remaining > 0.f)
		{
			SetHealth(FMath::Clamp(GetHealth() - Remaining, 0.f, GetMaxHealth()));
			if (GetHealth() <= 0.f)
			{
				OnHealthZero.Broadcast(Data);
			}
		}
		OnDamaged.Broadcast(Data);
	}

	else if (Data.EvaluatedData.Attribute == GetHealAttribute())
	{
		const float LocalHeal = GetHeal();
		SetHeal(0.f);                    // 메타값 즉시 소비

		if (LocalHeal > 0.f)
		{
			float Remaining = LocalHeal;

			// 1) HP 먼저 채움 (Max까지)
			const float HealthSpace = GetMaxHealth() - GetHealth();
			if (HealthSpace > 0.f)
			{
				const float Applied = FMath::Min(HealthSpace, Remaining);
				SetHealth(GetHealth() + Applied);
				Remaining -= Applied;
			}

			// 2) 넘친 만큼 쉴드로 (Max까지)
			if (Remaining > 0.f)
			{
				SetShield(FMath::Clamp(GetShield() + Remaining, 0.f, GetMaxShield()));
			}
		}
	}
}

void ULMSAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, Health, OldHealth);
}

void ULMSAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, MaxHealth, OldMaxHealth);
}

void ULMSAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, Shield, OldShield);
}

void ULMSAttributeSet::OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, MaxShield, OldMaxShield);
}

void ULMSAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, Stamina, OldStamina);
}

void ULMSAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, MaxStamina, OldMaxStamina);
}

void ULMSAttributeSet::OnRep_Speed(const FGameplayAttributeData& OldSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, Speed, OldSpeed);
}

void ULMSAttributeSet::OnRep_MaxSpeed(const FGameplayAttributeData& OldMaxSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, MaxSpeed, OldMaxSpeed);
}

void ULMSAttributeSet::OnRep_Wound(const FGameplayAttributeData& OldWound)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, Wound, OldWound);
}

void ULMSAttributeSet::OnRep_MaxWound(const FGameplayAttributeData& OldMaxWound)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, MaxWound, OldMaxWound);
}

void ULMSAttributeSet::OnRep_IncapHealth(const FGameplayAttributeData& OldIncapHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, IncapHealth, OldIncapHealth);
}

void ULMSAttributeSet::OnRep_MaxIncapHealth(const FGameplayAttributeData& OldMaxIncapHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ULMSAttributeSet, MaxIncapHealth, OldMaxIncapHealth);
}
