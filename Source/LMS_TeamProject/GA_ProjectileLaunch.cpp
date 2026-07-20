// Fill out your copyright notice in the Description page of Project Settings.

#include "GA_ProjectileLaunch.h"
#include "HitBox_Projectile.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Engine/World.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UGA_ProjectileLaunch::UGA_ProjectileLaunch()
{
	MuzzleSocketName = TEXT("hand_r");
}

void UGA_ProjectileLaunch::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// SpawnEventTag가 Send Gameplay Event To Actor로 들어올 때까지 대기 (예: 애님 노티파이 타이밍)
	UAbilityTask_WaitGameplayEvent* WaitEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, SpawnEventTag);
}
