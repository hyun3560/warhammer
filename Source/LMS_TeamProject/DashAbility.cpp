// Fill out your copyright notice in the Description page of Project Settings.


#include "DashAbility.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionConstantForce.h"
#include "GameFramework/Character.h"


UDashAbility::UDashAbility()
{
}

void UDashAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	//방향 결정
	ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FVector Direction = Character->GetLastMovementInputVector();
	if (Direction.IsNearlyZero())
	{
		Direction = Character->GetActorForwardVector();
	}
	Direction = Direction.GetSafeNormal2D();
	UAbilityTask_ApplyRootMotionConstantForce* Task =
	UAbilityTask_ApplyRootMotionConstantForce::ApplyRootMotionConstantForce(
		this,
		NAME_None,
		Direction,
		DashStrength,
		DashDuration,
		false,                                          // bIsAdditive
		nullptr,                                        // StrengthOverTime
		ERootMotionFinishVelocityMode::ClampVelocity,   // 끝나면 속도 제한
		FVector::ZeroVector,
		0.f,
		false);
	if (!Task)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	Task->OnFinish.AddDynamic(this, &UDashAbility::OnDashFinished);
	Task->ReadyForActivation();

}

void UDashAbility::OnDashFinished()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
