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
class UInputMappingContext;
class UInputAction;
class UAbilitySystemComponent;
class ULMSAttributeSet;
class UInteractionDetectorComponent;
class UGameplayAbility;
class ULMSWeaponComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config = Game)
class ALMS_TeamProjectCharacter : public ACharacter, public IAbilitySystemInterface, public ILMSInteractableInterface, public IIndicatorTargetInterface
{
	GENERATED_BODY()

	/** Ability System Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	UAbilitySystemComponent* AbilitySystemComponent;

	/** Attribute Set */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	ULMSAttributeSet* AttributeSet;

	/** Abilities granted to this character on possession */
	UPROPERTY(EditDefaultsOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TArray<TSubclassOf<UGameplayEffect>> DefaultEffects;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> HealEffect;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> HealBlockEffect;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> IncapacitatedEffect;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> BleedOutEffect;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DeadEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Interaction, meta = (AllowPrivateAccess = "true"))
	UInteractionDetectorComponent* InteractionDetector;

	UPROPERTY()
	ELMSAbilityInputID CachedInteractInputID = ELMSAbilityInputID::None;

	UPROPERTY(EditDefaultsOnly, Category = "Spectator")
	TSubclassOf<class ALMSSpectatorPawn> SpectatorPawnClass;

	/** 죽을 때 소유했던 PC — 언포제스 후 PlayerState가 null이 되므로 별도 보관 (서버 전용) */
	UPROPERTY()
	TObjectPtr<APlayerController> CachedOwnerPC;

	/** 죽음 상태 — 복제되어 각 클라에서 래그돌 시작 */
	UPROPERTY(ReplicatedUsing = OnRep_IsDead, BlueprintReadOnly, Category = "Death", meta = (AllowPrivateAccess = "true"))
	bool bIsDead = false;

	/** 래그돌 유지 시간 (이후 시체 숨김) */
	UPROPERTY(EditDefaultsOnly, Category = "Death", meta = (AllowPrivateAccess = "true"))
	float CorpseRagdollDuration = 3.f;

	/** 구조물 부활 시 적용할 GE (죽음/다운 GE 제거 + 체력 복구) */
	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> RescuedEffect;

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Handles equipped weapon data, spawned weapon actor, and weapon actions */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	ULMSWeaponComponent* WeaponComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SprintAction;

	/** Dash Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* DashAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PrimaryAction;

	/** Ping Input Action (마우스 휠 클릭) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PingAction;

	/** 핑 마커로 스폰할 액터 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Ping", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<class APingMarker> PingMarkerClass;

	/** 핑 라인트레이스 최대 거리 */
	UPROPERTY(EditDefaultsOnly, Category = "Ping", meta = (AllowPrivateAccess = "true"))
	float PingTraceDistance = 5000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* SecondaryAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* WeaponSkillAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;

	UPROPERTY(EditDefaultsOnly, Category = "Coherency")
	float CoherencyDistance = 500.f;

public:
	ALMS_TeamProjectCharacter();

	//~ Begin IAbilitySystemInterface Interface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface Interface

	//~ Begin IIndicatorTargetInterface
	virtual ELMSIndicatorType GetIndicatorType_Implementation() const override;
	virtual bool ShouldShowIndicator_Implementation() const override;
	//~ End IIndicatorTargetInterface

protected:
	/** Called for movement input */
	void Input_Jump();

	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Grants DefaultAbilities to the AbilitySystemComponent. Server only. */
	void GiveDefaultAbilities();
	/** Applies DefaultEffects to self. Server only. */
	void ApplyDefaultEffects();

	void OnSpeedChanged(const struct FOnAttributeChangeData& Data);

	void OnAbilityInputPressed(ELMSAbilityInputID InputID);
	void OnAbilityInputReleased(ELMSAbilityInputID InputID);

	void HandleDamaged(const FGameplayEffectModCallbackData& Data);
	void HandleHealthZero(const FGameplayEffectModCallbackData& Data);
	void HandleIncapHealthZero(const FGameplayEffectModCallbackData& Data);

	UFUNCTION()
	void OnRep_IsDead();

	/** 래그돌 시작 + 정리 타이머 예약 (서버/클라 각자 실행) */
	void StartRagdoll();

	/** 시체 상태 해제 — 물리 끄고 메시 복구 + 표시 (서버/클라 각자) */
	void RestoreFromCorpse();

	/** 타이머 만료 — 숨김 → 물리 해제 → 메시 재부착 순서 */
	void FinishCorpseCleanup();

	/** InteractionDetector가 대상 변경을 알릴 때 호출 — HUD 프롬프트 표시/숨김 */
	UFUNCTION()
	void OnInteractTargetChanged(AActor* NewTarget, FGameplayTag InteractionType);

	AActor* FindFirstLivingAlly() const;

	/** 마우스 휠 클릭 입력 처리: 카메라 중앙(크로스헤어) 기준 라인트레이스로 핑 위치 계산 */
	void RequestPing(const FInputActionValue& Value);

	UFUNCTION(Server, Reliable)
	void Server_RequestPing(FVector_NetQuantize PingLocation);

	/** 서버/클라 공통 ASC 초기화 */
	void InitAbilityActorInfo();

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	FTimerHandle CoherencyTimerHandle;

	FTimerHandle CorpseTimerHandle;

	/** 래그돌 전 메시의 원래 상대 트랜스폼 (부활 시 복구용) */
	FTransform MeshRelativeTransform;

	/** 재possess 시 DefaultAbilities/Effects 중복 적용 방지 */
	bool bDefaultsInitialized = false;

public:
	UFUNCTION(BlueprintCallable)
	void TakeDamage(float Damage);

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ULMSWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	//~ Begin ILMSInteractableInterface
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FGameplayTag GetInteractionType_Implementation() const override;
	virtual int32 GetInteractInputID_Implementation() const override;
	//~ End ILMSInteractableInterface

	UInteractionDetectorComponent* GetInteractionDetector() const { return InteractionDetector; }

	void RescueFromDeath(const FVector& ReviveLocation, const FRotator& ReviveRotation);

	bool IsDead() const { return bIsDead; }

	void CheckCoherency();

};