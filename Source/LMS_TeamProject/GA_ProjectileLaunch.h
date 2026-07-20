// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "LMSGameplayAbility.h"
#include "GA_ProjectileLaunch.generated.h"

/**
 * 포물선 발사체를 Spawn하고 목표 지점으로 발사하는 어빌리티
 */
UCLASS()
class LMS_TEAMPROJECT_API UGA_ProjectileLaunch : public ULMSGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ProjectileLaunch();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 발사할 발사체 클래스 (블루프린트에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<class AHitBox_Projectile> ProjectileClass;

	// 발사 시작 소켓 이름 (Mesh 소켓, 없으면 캐릭터 위치 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FName MuzzleSocketName;

	// 타겟 Actor (없으면 커서/조준 방향 사용)
	UPROPERTY(BlueprintReadWrite, Category = "Projectile")
	TObjectPtr<AActor> TargetActor;

	// Send Gameplay Event To Actor로 이 태그가 들어오면 발사체 생성 (예: 애님 노티파이)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	FGameplayTag SpawnEventTag;

};
