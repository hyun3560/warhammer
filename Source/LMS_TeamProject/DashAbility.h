// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LMSGameplayAbility.h"
#include "DashAbility.generated.h"

/**
 * 
 */
UCLASS()
class LMS_TEAMPROJECT_API UDashAbility : public ULMSGameplayAbility
{
	GENERATED_BODY()
	
public:
	UDashAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashStrength = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Dash")
	float DashDuration = 0.2f;

	UFUNCTION()
	void OnDashFinished();
};
