#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Components/ActorComponent.h"
#include "LMSWeaponTypes.h"
#include "LMSWeaponComponent.generated.h"

class ALMSWeaponBase;
class ACharacter;
class UAbilitySystemComponent;
class UCameraComponent;
class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponAmmoChanged, int32, AmmoInMagazine, int32, ReserveAmmo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWeaponSkillCooldownChanged, float, CurrentCooldown, float, MaxCooldown);

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

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Ammo")
	FOnWeaponAmmoChanged OnAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "Weapon|Skill")
	FOnWeaponSkillCooldownChanged OnSkillCooldownChanged;

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ammo")
	bool TryConsumeAmmo(int32 AmmoCost = 1, bool bReloadIfEmpty = true);

	void StartSkillCooldown(float CurrentCooldown, float MaxCooldown);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Skill")
	void PerformMeleeSkillSweep(float DamageMultiplier, float RangeMultiplier, float TraceRadius, bool bDrawDebugTrace);

	UFUNCTION(BlueprintCallable, Category = "Weapon|Ranged")
	void FireRangedShot(float DamageMultiplier, float RangeMultiplier, bool bDrawDebugTrace);

protected:
	virtual void BeginPlay() override;

	ACharacter* GetOwnerCharacter() const;
	UCameraComponent* GetOwnerCameraComponent() const;
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	void GrantCurrentWeaponAbilities();
	void ClearGrantedWeaponAbilities();
	void GrantWeaponAbility(TSubclassOf<UGameplayAbility> AbilityClass);
	void StartMeleeAttack();
	void StartRangedAttack();
	void StartBlock();
	void StopBlock();
	void StartAim();
	void StopAim();
	bool CanReload() const;
	void FinishReload();
	void BroadcastAmmoChanged();
	void UpdateSkillCooldown();
	void BroadcastSkillCooldownChanged(float CurrentCooldown, float MaxCooldown);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	UDataTable* WeaponDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName DefaultWeaponRowName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName EquippedSocketName = TEXT("hand_rSocket");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	float MeleeTraceRadius = 80.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Aim")
	float AimFOV = 65.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Aim")
	float DefaultFOV = 90.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Debug")
	bool bDrawDebugMeleeTrace = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Debug")
	bool bDrawDebugRangedTrace = true;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	ALMSWeaponBase* CurrentWeapon;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon")
	FWeaponData CurrentWeaponData;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 AmmoInMagazine = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 ReserveAmmo = 0;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	bool bIsReloading = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|State")
	bool bIsBlocking = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|State")
	bool bIsAiming = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Weapon|GAS")
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	FTimerHandle ReloadTimerHandle;
	FTimerHandle SkillCooldownTimerHandle;

	float SkillCooldownEndTime = 0.f;
	float SkillCooldownDuration = 0.f;
};
