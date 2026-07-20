// Copyright Epic Games, Inc. All Rights Reserved.

#include "LMSGameplayAbility.h"

ULMSGameplayAbility::ULMSGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}
