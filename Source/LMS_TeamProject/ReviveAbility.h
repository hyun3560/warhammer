#pragma once

#include "CoreMinimal.h"
#include "LMSGameplayAbility.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "ReviveAbility.generated.h"

class ULMSCombatHUDPresenterComponent;

/**
 * 쓰러진 팀원을 부활시키는 홀드형 어빌리티.
 *
 * 흐름:
 *  1) 캐릭터가 매 틱 트레이스로 부활 대상을 감지해 멤버변수에 보관 (UI 프롬프트용, 로컬)
 *  2) E 입력 → GA 활성화
 *     - 클라: 대상을 TargetData로 포장해 서버로 전송 + 로컬 예측 시작
 *     - 서버: TargetData 수신 대기 → 받으면 거리/태그 검증 → 시작
 *  3) WaitReviveHold 태스크가 홀드 시간 + 유지 조건을 검사
 *  4) 완주 → 서버에서 GE_Revive 적용 (Incap 해제 + Health 25%)
 *
 * 대상 지정이 필요하므로(자기대상 아님) TargetData로 대상을 서버에 전달한다.
 * 요구 설정: ReplicationPolicy = Replicate, NetExecutionPolicy = LocalPredicted.
 */
UCLASS()
class LMS_TEAMPROJECT_API UReviveAbility : public ULMSGameplayAbility
{
	GENERATED_BODY()

public:
	UReviveAbility();

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
	//~ 대상 전달: 서버가 리모트 클라의 TargetData를 수신하는 콜백
	void OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& Data, FGameplayTag ActivationTag);

	//~ 대상 확정 후 클라/서버 공통 진입점 (홀드 태스크 시작 + BeingRevived 적용)
	void StartRevive(AActor* Target);

	//~ 서버 검증 (거리 + 다운 상태 태그)
	bool ValidateTarget(AActor* Target) const;

	//~ 태스크 결과 콜백
	UFUNCTION()
	void OnReviveCompleted();   // 홀드 완주 → 부활

	UFUNCTION()
	void OnReviveCancelled();   // 조건 이탈 / 입력 해제 → 취소

	UFUNCTION()
	void HandleReviveProgress(float Progress);

	// 로컬 플레이어의 HUD Presenter를 찾아 구조 진행바 UI에 접근합니다.
	ULMSCombatHUDPresenterComponent* GetCombatHUDPresenter() const;

protected:
	// 부활에 필요한 홀드 시간(초)
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	float ReviveDuration = 3.0f;

	// 서버 검증용 최대 거리 (캐릭터 트레이스 거리보다 살짝 넉넉하게)
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	float MaxReviveDistance = 300.0f;

	// 부활 완료 시 대상에게 적용할 GE (Incap/BleedOut 제거 + Health 25%)
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	TSubclassOf<UGameplayEffect> ReviveEffect;

	// 홀드 중 대상에게 붙일 GE (state.BeingRevived 부여 → BleedOut 정지)
	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	TSubclassOf<UGameplayEffect> BeingRevivedEffect;

private:
	// 현재 부활 대상 (PerActor 인스턴스라 재활성화 시 ActivateAbility에서 초기화)
	UPROPERTY()
	TObjectPtr<AActor> ReviveTarget = nullptr;

	// BeingRevived 적용 핸들 (EndAbility에서 제거하기 위해 보관)
	FActiveGameplayEffectHandle BeingRevivedHandle;

	// 서버가 TargetData 수신용으로 등록한 델리게이트 핸들 (EndAbility에서 해제)
	FDelegateHandle TargetDataDelegateHandle;
};
