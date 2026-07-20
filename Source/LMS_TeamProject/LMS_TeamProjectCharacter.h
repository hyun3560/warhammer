// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "AbilitySystemInterface.h"
#include "LMSGameplayAbility.h"
#include "UI/IndicatorTargetInterface.h"
#include "LMS_TeamProjectCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UAbilitySystemComponent;
class ULMSAttributeSet;
class UGameplayAbility;
class ULMSWeaponComponent;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ALMS_TeamProjectCharacter : public ACharacter, public IAbilitySystemInterface, public IIndicatorTargetInterface
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
	TSubclassOf<UGameplayEffect> IncapacitatedEffect;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> BleedOutEffect;

	UPROPERTY(EditDefaultsOnly, Category = Effects, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DeadEffect;

	UPROPERTY()
	TObjectPtr<AActor> CurrentReviveTarget = nullptr;

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



	UPROPERTY(EditDefaultsOnly, Category = "Revive")
	float ReviveTraceDistance = 250.f;

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

	void HandleHealthZero(const FGameplayEffectModCallbackData& Data);
	void HandleIncapHealthZero(const FGameplayEffectModCallbackData& Data);

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

	private:
		FTimerHandle ReviveTraceTimerHandle;

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

	TObjectPtr<AActor> GetCurrentReviveTarget() const { return CurrentReviveTarget; }

	void TraceForReviveTarget();
};

