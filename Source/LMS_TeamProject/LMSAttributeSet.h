// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "LMSAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeZero, const FGameplayEffectModCallbackData&);
/**
 * Basic attribute set for LMS_TeamProject characters.
 */
UCLASS()
class LMS_TEAMPROJECT_API ULMSAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	ULMSAttributeSet();

	//체력 0이 될 때 호출할 델리게이트(서버용)
	FOnAttributeZero OnHealthZero;

	//빈사상태 0이 될 때 호출할 델리게이트(서버용)
	FOnAttributeZero OnIncapHealthZero;

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, Shield)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxShield)
	FGameplayAttributeData MaxShield;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, MaxShield)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, MaxStamina)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Speed)
	FGameplayAttributeData Speed;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, Speed)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxSpeed)
	FGameplayAttributeData MaxSpeed;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, MaxSpeed)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Wound)
	FGameplayAttributeData Wound;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, Wound)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxWound)
	FGameplayAttributeData MaxWound;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, MaxWound)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_IncapHealth)
	FGameplayAttributeData IncapHealth;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, IncapHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxIncapHealth)
	FGameplayAttributeData MaxIncapHealth;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, MaxIncapHealth)

		UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(ULMSAttributeSet, Damage)

	//~ Begin UAttributeSet Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	//~ End UAttributeSet Interface

protected:

	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data);

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Shield(const FGameplayAttributeData& OldShield);

	UFUNCTION()
	virtual void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);

	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	UFUNCTION()
	virtual void OnRep_Speed(const FGameplayAttributeData& OldSpeed);

	UFUNCTION()
	virtual void OnRep_MaxSpeed(const FGameplayAttributeData& OldMaxSpeed);

	UFUNCTION()
	void OnRep_Wound(const FGameplayAttributeData& OldWound);

	UFUNCTION()
	void OnRep_MaxWound(const FGameplayAttributeData& OldMaxWound);

	UFUNCTION()
	void OnRep_IncapHealth(const FGameplayAttributeData& OldIncapHealth);

	UFUNCTION()
	void OnRep_MaxIncapHealth(const FGameplayAttributeData& OldMaxIncapHealth);
};
