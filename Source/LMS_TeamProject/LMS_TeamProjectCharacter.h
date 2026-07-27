// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "LMSInteractableInterface.h"
#include "LMSGameplayAbility.h"
#include "UI/IndicatorTargetInterface.h"
#include "LMS_TeamProjectCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UCameraShakeBase;
class UInputMappingContext;
class UInputAction;
class UAbilitySystemComponent;
class ULMSAttributeSet;
class UInteractionDetectorComponent;
class UGameplayAbility;
class ULMSWeaponComponent;
class ALMSSpectatorPawn;
class APingMarker;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * 플레이어 캐릭터.
 *
 * ASC는 PlayerState가 소유하고, 이 캐릭터는 Avatar 역할만 한다.
 * (서버: PossessedBy / 클라: OnRep_PlayerState 에서 InitAbilityActorInfo 호출)
 */
UCLASS(config = Game)
class ALMS_TeamProjectCharacter : public ACharacter,
	public IAbilitySystemInterface,
	public ILMSInteractableInterface,
	public IIndicatorTargetInterface
{
	GENERATED_BODY()

public:
	ALMS_TeamProjectCharacter();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	//~ IIndicatorTargetInterface
	virtual ELMSIndicatorType GetIndicatorType_Implementation() const override;
	virtual bool ShouldShowIndicator_Implementation() const override;

	//~ ILMSInteractableInterface — 다운된 자신이 곧 부활 대상이 된다
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FGameplayTag GetInteractionType_Implementation() const override;
	virtual int32 GetInteractInputID_Implementation() const override;

	/** 구조물 부활 — 지정 위치로 이동 후 재possess + GE_Rescued 적용 (서버 전용) */
	void RescueFromDeath(const FVector& ReviveLocation, const FRotator& ReviveRotation);

	/** 주변 아군 수만큼 실드 회복 (Coherency 타이머에서 주기 호출, 서버 전용) */
	void CheckCoherency();

	UFUNCTION(BlueprintCallable)
		void TakeDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
		ULMSWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UInteractionDetectorComponent* GetInteractionDetector() const { return InteractionDetector; }

	bool IsDead() const { return bIsDead; }
	TSubclassOf<UCameraShakeBase> GetDamageCameraShakeClass() const { return DamageCameraShake; }
	float GetDamageCameraShakeScale() const { return DamageCameraShakeScale; }

protected:
	//~ AActor / APawn
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── GAS 초기화 ────────────────────────────────────────────────
	/** 서버/클라 공통 ASC 초기화. PlayerState 준비 시점에 호출된다. */
	void InitAbilityActorInfo();

	/** DefaultAbilities 부여 (서버 전용) */
	void GiveDefaultAbilities();

	/** DefaultEffects 자기 자신에게 적용 (서버 전용) */
	void ApplyDefaultEffects();

	// ── 입력 ─────────────────────────────────────────────────────
	void Input_Jump();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void OnAbilityInputPressed(ELMSAbilityInputID InputID);
	void OnAbilityInputReleased(ELMSAbilityInputID InputID);

	/** 카메라 중앙(크로스헤어) 기준 라인트레이스로 핑 위치를 구해 서버에 요청 */
	void RequestPing(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
		void Server_RequestPing(FVector_NetQuantize PingLocation);

	// ── GAS 콜백 ─────────────────────────────────────────────────
	void OnSpeedChanged(const struct FOnAttributeChangeData& Data);

	void HandleDamaged(const FGameplayEffectModCallbackData& Data);
	void HandleHealthZero(const FGameplayEffectModCallbackData& Data);
	void HandleIncapHealthZero(const FGameplayEffectModCallbackData& Data);

	UFUNCTION(Client, Unreliable)
	void ClientPlayDamageCameraShake();

	// ── 죽음 / 래그돌 ─────────────────────────────────────────────
	UFUNCTION()
		void OnRep_IsDead();

	/** 래그돌 시작 + 정리 타이머 예약 (서버/클라 각자 실행) */
	void StartRagdoll();

	/** 타이머 만료 — 숨김 → 물리 해제 → 메시 재부착 순서 */
	void FinishCorpseCleanup();

	/** 시체 상태 해제 — 물리 끄고 메시 복구 + 표시 (서버/클라 각자) */
	void RestoreFromCorpse();

	/** 스펙테이트 대상으로 쓸, 살아있는 아군 하나 */
	AActor* FindFirstLivingAlly() const;

	// ── 상호작용 ─────────────────────────────────────────────────
	/** InteractionDetector가 대상 변경을 알릴 때 호출 — HUD 프롬프트 표시/숨김 */
	UFUNCTION()
		void OnInteractTargetChanged(AActor* NewTarget, FGameplayTag InteractionType);

private:
	// ── 컴포넌트 ─────────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		UCameraComponent* FollowCamera;

	/** 장착 무기 데이터 / 스폰된 무기 액터 / 무기 액션 담당 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
		ULMSWeaponComponent* WeaponComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
		UInteractionDetectorComponent* InteractionDetector;

	// ── GAS ──────────────────────────────────────────────────────
	/** PlayerState 소유 ASC의 캐시 (이 캐릭터가 소유하지 않는다) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
		UAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
		ULMSAttributeSet* AttributeSet;

	UPROPERTY(EditDefaultsOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
		TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
		TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

	/** Coherency 실드 회복 */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<UGameplayEffect> HealEffect;

	/** 피격 시 회복 차단 */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<UGameplayEffect> HealBlockEffect;

	/** Health 0 → 다운 */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<UGameplayEffect> IncapacitatedEffect;

	/** 다운 중 IncapHealth 지속 감소 */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<UGameplayEffect> BleedOutEffect;

	/** IncapHealth 0 → 사망 */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<UGameplayEffect> DeadEffect;

	/** 구조물 부활 시 적용 (죽음/다운 GE 제거 + 체력 복구) */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
		TSubclassOf<UGameplayEffect> RescuedEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Shake|Damage", meta = (AllowPrivateAccess = "true"))
		TSubclassOf<UCameraShakeBase> DamageCameraShake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera Shake|Damage", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
		float DamageCameraShakeScale = 1.f;

	// ── 입력 ─────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* PrimaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* SecondaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* WeaponSkillAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* ReloadAction;

	/** 마우스 휠 클릭 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
		UInputAction* PingAction;

	/** Interact(E) 키가 Pressed 때 실제로 켠 InputID — Released 라우팅에 사용 */
	UPROPERTY()
		ELMSAbilityInputID CachedInteractInputID = ELMSAbilityInputID::None;

	// ── 핑 ───────────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Ping", meta = (AllowPrivateAccess = "true"))
		TSubclassOf<APingMarker> PingMarkerClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ping", meta = (AllowPrivateAccess = "true"))
		float PingTraceDistance = 5000.f;

	// ── 죽음 / 스펙테이터 ─────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Spectator", meta = (AllowPrivateAccess = "true"))
		TSubclassOf<ALMSSpectatorPawn> SpectatorPawnClass;

	/** 죽을 때 소유했던 PC — 언포제스 후 PlayerState가 null이 되므로 별도 보관 (서버 전용) */
	UPROPERTY()
		TObjectPtr<APlayerController> CachedOwnerPC;

	/** 죽음 상태 — 복제되어 각 클라에서 래그돌 시작 */
	UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "Death", meta = (AllowPrivateAccess = "true"))
		bool bIsDead = false;

	/** 래그돌 유지 시간 (이후 시체 숨김) */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (AllowPrivateAccess = "true"))
		float CorpseRagdollDuration = 3.f;

	// ── Coherency ────────────────────────────────────────────────
	UPROPERTY(EditDefaultsOnly, Category = "Coherency", meta = (AllowPrivateAccess = "true"))
		float CoherencyDistance = 500.f;

	// ── 런타임 상태 (비복제) ──────────────────────────────────────
	FTimerHandle CoherencyTimerHandle;
	FTimerHandle CorpseTimerHandle;

	/** 래그돌 전 메시의 원래 상대 트랜스폼 (부활 시 복구용) */
	FTransform MeshRelativeTransform;

	/** 재possess 시 DefaultAbilities/Effects 중복 적용 방지 */
	bool bDefaultsInitialized = false;
};
