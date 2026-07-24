#pragma once

#include "CoreMinimal.h"
#include "LMSGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "RescueAllAbility.generated.h"

class ALMSRescueStation;
class ULMSCombatHUDPresenterComponent;

UCLASS()
class LMS_TEAMPROJECT_API URescueAllAbility : public ULMSGameplayAbility
{
	GENERATED_BODY()

public:
	URescueAllAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void InputReleased(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	/** 홀드 유지 시간 */
	UPROPERTY(EditDefaultsOnly, Category = "Rescue")
	float HoldDuration = 5.f;

	/** 유효성 검사 주기 */
	UPROPERTY(EditDefaultsOnly, Category = "Rescue")
	float ValidationInterval = 0.1f;

	/** 구조물에서 벗어나면 취소되는 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "Rescue")
	float MaxInteractDistance = 300.f;

	UPROPERTY()
	TObjectPtr<ALMSRescueStation> TargetStation;

	/** 서버: 클라가 보낸 TargetData 수신 */
	void OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag Tag);

	/** 클라/서버 공통 — 홀드 태스크 시작 */
	void StartHold();

	UFUNCTION()
	void OnHoldCompleted();

	UFUNCTION()
	void OnHoldCancelled();

	UFUNCTION()
	void HandleHoldProgress(float Progress);

	ULMSCombatHUDPresenterComponent* GetCombatHUDPresenter() const;

	FDelegateHandle TargetDataDelegateHandle;
};