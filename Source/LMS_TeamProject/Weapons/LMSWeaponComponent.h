#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "LMSWeaponTypes.h"
#include "LMSWeaponComponent.generated.h"

class ALMSWeaponBase;
class ACharacter;
class UAbilitySystemComponent;
class UAnimInstance;
class UAnimMontage;
class UCameraComponent;
class UDataTable;
class USceneComponent;
class UTexture2D;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, AmmoInMagazine, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSkillCooldownChanged, float, CurrentCooldown, float, MaxCooldown);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponHUDChanged, UTexture2D*, WeaponIcon, bool, bShowAmmo);

USTRUCT(BlueprintType)
struct FReplicatedWeaponMontageState
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAnimMontage> Montage = nullptr;

	UPROPERTY()
	FName SectionName = NAME_None;

	UPROPERTY()
	float PlayRate = 1.f;

	UPROPERTY()
	uint8 Counter = 0;

	UPROPERTY()
	bool bStop = false;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LMS_TEAMPROJECT_API ULMSWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULMSWeaponComponent();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeaponFromData(const FWeaponData& WeaponData);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeaponByID(FName WeaponID);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool EquipWeaponByRowName(FName RowName);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void UnequipCurrentWeapon();

	UFUNCTION(BlueprintCallable, Category = "Weapon|First Person")
	void SetLocalFirstPersonWeaponViewEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartAttack();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopAttack();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StartSecondaryAction();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void StopSecondaryAction();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void Reload();

	UFUNCTION(BlueprintCallable, Category = "Weapon|GAS")
	void RefreshGrantedAbilities();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ALMSWeaponBase* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	const FWeaponData& GetCurrentWeaponData() const { return CurrentWeaponData; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsReloading() const { return bIsReloading; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsBlocking() const { return bIsBlocking; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsAiming() const { return bIsAiming; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Skill")
	float GetSkillCooldownRemaining() const;

	UFUNCTION(BlueprintPure, Category = "Weapon|Skill")
	float GetSkillCooldownDuration() const { return SkillCooldownDuration; }

	UFUNCTION(BlueprintPure, Category = "Weapon|HUD")
	UTexture2D* GetCurrentWeaponHUDIcon() const { return ResolveWeaponHUDIcon(); }

	UFUNCTION(BlueprintPure, Category = "Weapon|HUD")
	bool ShouldDisplayAmmoOnHUD() const { return ShouldShowAmmoOnHUD(); }

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Ammo")
	FOnWeaponAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Skill")
	FOnWeaponSkillCooldownChanged OnSkillCooldownChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|HUD")
	FOnWeaponHUDChanged OnWeaponHUDChanged;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	bool TryConsumeAmmo(int32 AmmoCost = 1, bool bReloadIfEmpty = true);

	void StartSkillCooldown(float CurrentCooldown, float MaxCooldown);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Skill")
	void PerformMeleeSkillSweep(float DamageMultiplier, float RangeMultiplier, float TraceRadius, bool bDrawDebugTrace);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Skill")
	void ActivateMeleeDamageBoost(float DamageMultiplier, float Duration, int32 BoostedTraceCount = 1);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ranged")
	void FireRangedShot(float DamageMultiplier, float RangeMultiplier, bool bDrawDebugTrace);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Animation")
	void PlayReplicatedThirdPersonWeaponMontage(UAnimMontage* Montage, FName SectionName = NAME_None, float PlayRate = 1.f);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Animation")
	void StopReplicatedThirdPersonWeaponMontage(UAnimMontage* Montage, float BlendOutTime = 0.15f);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Trace")
	void BeginWeaponTrace(FName StartSocketName, FName EndSocketName, float TraceRadius, float DamageMultiplier, bool bDrawDebugTrace);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Trace")
	void TickWeaponTrace(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Trace")
	void EndWeaponTrace();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	void StartComboAttack(int32 StartingComboIndex = 1);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	bool HandleMeleeComboInput(UAnimInstance* AnimInstance, UAnimMontage* ComboMontage, const TArray<FName>& ComboSectionNames);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	bool HandleMeleeComboInputLinked(
		UAnimInstance* PrimaryAnimInstance,
		UAnimMontage* PrimaryComboMontage,
		UAnimInstance* LinkedAnimInstance,
		UAnimMontage* LinkedComboMontage,
		const TArray<FName>& ComboSectionNames);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	void NotifyComboSectionBegin(int32 ComboIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	void NotifyComboSectionEnd(int32 ComboIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	bool RegisterComboInput();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	void OpenComboWindow();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	void CloseComboWindow();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	bool QueueComboSection(UAnimInstance* AnimInstance, UAnimMontage* ComboMontage, FName CurrentSection, FName NextSection, int32 NextComboIndex);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	bool ConsumeBufferedComboInput();

	UFUNCTION(BlueprintCallable, Category = "Weapon|Combo")
	void ResetCombo();

	UFUNCTION(BlueprintPure, Category = "Weapon|Combo")
	bool IsComboAttacking() const { return bIsComboAttacking; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Combo")
	bool IsComboWindowOpen() const { return bComboWindowOpen; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Combo")
	bool HasBufferedComboInput() const { return bComboInputBuffered; }

	UFUNCTION(BlueprintPure, Category = "Weapon|Combo")
	int32 GetCurrentComboIndex() const { return CurrentComboIndex; }

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	ACharacter* GetOwnerCharacter() const;
	UCameraComponent* GetOwnerCameraComponent() const;
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	void GrantCurrentWeaponAbilities();
	void ClearGrantedWeaponAbilities();
	void GrantWeaponAbility(TSubclassOf<UGameplayAbility> AbilityClass);
	ALMSWeaponBase* SpawnWeaponActor(const FWeaponData& WeaponData, TSubclassOf<ALMSWeaponBase> OverrideWeaponClass = nullptr) const;
	USceneComponent* FindFirstPersonWeaponAttachComponent() const;
	void RefreshFirstPersonWeaponVisual();
	void ApplyLocalWeaponViewVisibility();
	void StartMeleeAttack();
	void StartRangedAttack();
	void StartBlock();
	void StopBlock();
	void StartAim();
	void StopAim();
	bool CanReload() const;
	void FinishReload();
	void ClearMeleeDamageBoost();
	void BroadcastAmmoChanged();
	void BroadcastWeaponHUDChanged();
	void UpdateSkillCooldown();
	void BroadcastSkillCooldownChanged(float CurrentCooldown, float MaxCooldown);
	float ResolveCurrentWeaponSkillCooldownDuration() const;
	void CacheWeaponDataByID(FName WeaponID);
	UTexture2D* ResolveWeaponHUDIcon() const;
	bool ShouldShowAmmoOnHUD() const;
	void RestartReplicatedSkillCooldownTimer();
	ALMSWeaponBase* GetWeaponTraceActor() const;
	bool GetWeaponTraceSocketLocations(FVector& OutStart, FVector& OutEnd) const;
	void TraceWeaponSegment(const FVector& PreviousPoint, const FVector& CurrentPoint, FCollisionQueryParams& QueryParams);
	void HandleWeaponTraceHit(const FHitResult& Hit, const FVector& TraceStart, const FVector& TraceEnd);
	float ConsumeWeaponTraceDamageMultiplier(float DamageMultiplier);
	bool IsValidClientWeaponTraceHit(AActor* HitActor, const FVector& TraceStart, const FVector& TraceEnd) const;
	void PlayThirdPersonWeaponMontageLocal(UAnimMontage* Montage, FName SectionName, float PlayRate);
	void StopThirdPersonWeaponMontageLocal(UAnimMontage* Montage, float BlendOutTime);
	void ReplicateThirdPersonWeaponMontage(UAnimMontage* Montage, FName SectionName, float PlayRate = 1.f);
	void ReplicateStopThirdPersonWeaponMontage(UAnimMontage* Montage);
	bool HandleMeleeComboInputInternal(
		UAnimInstance* PrimaryAnimInstance,
		UAnimMontage* PrimaryComboMontage,
		UAnimInstance* LinkedAnimInstance,
		UAnimMontage* LinkedComboMontage,
		const TArray<FName>& ComboSectionNames);
	bool QueueBufferedComboSection();
	void StopActiveComboMontage(float BlendOutTime = 0.15f);

	UFUNCTION()
	void OnRep_EquippedWeaponID();

	UFUNCTION()
	void OnRep_CurrentWeapon();

	UFUNCTION()
	void OnRep_Ammo();

	UFUNCTION()
	void OnRep_SkillCooldown();

	UFUNCTION()
	void OnRep_WeaponMontageState();

	UFUNCTION(Server, Reliable)
	void ServerPlayThirdPersonWeaponMontage(UAnimMontage* Montage, FName SectionName, float PlayRate);

	UFUNCTION(Server, Reliable)
	void ServerStopThirdPersonWeaponMontage(UAnimMontage* Montage);

	UFUNCTION(Server, Reliable)
	void ServerBeginFirstPersonWeaponTrace(float TraceRadius, float DamageMultiplier);

	UFUNCTION(Server, Reliable)
	void ServerApplyFirstPersonWeaponTraceHit(AActor* HitActor, FVector TraceStart, FVector TraceEnd);

	UFUNCTION(Server, Reliable)
	void ServerEndFirstPersonWeaponTrace();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UDataTable* WeaponDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName DefaultWeaponRowName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName EquippedSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|First Person")
	bool bSpawnFirstPersonWeaponVisual = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|First Person")
	FName FirstPersonWeaponAttachComponentName = TEXT("SK_Murdock_FP_Arms");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|First Person")
	FName FirstPersonEquippedSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	float MeleeTraceRadius = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Trace")
	FName DefaultTraceStartSocketName = TEXT("TraceStart");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Trace")
	FName DefaultTraceEndSocketName = TEXT("TraceEnd");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Trace")
	float WeaponTraceRadius = 12.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Trace", meta = (ClampMin = "2", ClampMax = "16"))
	int32 WeaponTraceSampleCount = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Trace")
	float ClientTraceValidationMaxDistance = 500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Trace")
	float ClientTraceValidationTolerance = 180.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Aim")
	float AimFOV = 65.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Aim")
	float DefaultFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Debug")
	bool bDrawDebugMeleeTrace = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Debug")
	bool bDrawDebugRangedTrace = true;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	ALMSWeaponBase* CurrentWeapon;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|First Person")
	ALMSWeaponBase* FirstPersonWeapon;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	FWeaponData CurrentWeaponData;

	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeaponID, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	FName EquippedWeaponID = NAME_None;

	UPROPERTY(ReplicatedUsing = OnRep_Ammo, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 AmmoInMagazine = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Ammo, VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 ReserveAmmo = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	bool bIsReloading = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|State")
	bool bIsBlocking = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|State")
	bool bIsAiming = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Combo")
	bool bIsComboAttacking = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Combo")
	bool bComboWindowOpen = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Combo")
	bool bComboInputBuffered = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Combo")
	bool bComboTransitionQueued = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Combo")
	int32 CurrentComboIndex = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|GAS")
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	FTimerHandle ReloadTimerHandle;
	FTimerHandle SkillCooldownTimerHandle;
	FTimerHandle MeleeDamageBoostTimerHandle;

	UPROPERTY(ReplicatedUsing = OnRep_SkillCooldown)
	float SkillCooldownEndTime = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_SkillCooldown)
	float SkillCooldownDuration = 0.f;

	UPROPERTY(ReplicatedUsing = OnRep_WeaponMontageState)
	FReplicatedWeaponMontageState WeaponMontageState;

	float MeleeDamageBoostMultiplier = 1.f;
	int32 RemainingBoostedWeaponTraces = 0;

	bool bIsWeaponTracing = false;
	bool bAcceptClientWeaponTraceHits = false;
	bool bDrawDebugWeaponTrace = false;
	bool bUseLocalFirstPersonWeaponView = true;
	float ActiveWeaponTraceRadius = 12.f;
	float ActiveWeaponTraceDamageMultiplier = 1.f;
	FName ActiveTraceStartSocketName = NAME_None;
	FName ActiveTraceEndSocketName = NAME_None;
	FVector PreviousTraceStart = FVector::ZeroVector;
	FVector PreviousTraceEnd = FVector::ZeroVector;
	TSet<TWeakObjectPtr<AActor>> WeaponTraceHitActors;

	TWeakObjectPtr<UAnimInstance> ActiveComboAnimInstance;
	TWeakObjectPtr<UAnimMontage> ActiveComboMontage;
	TWeakObjectPtr<UAnimInstance> ActiveLinkedComboAnimInstance;
	TWeakObjectPtr<UAnimMontage> ActiveLinkedComboMontage;
	TArray<FName> ActiveComboSectionNames;
};
