#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LMSWeaponTypes.generated.h"

class ALMSWeaponBase;
class UGameplayAbility;
class UGameplayEffect;
class UTexture2D;
class UAnimInstance;
class AHitBox_Projectile;

UENUM(BlueprintType)
enum class ELMSWeaponType : uint8
{
	Melee,
	Ranged
};

USTRUCT(BlueprintType)
struct FWeaponData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName WeaponID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	ELMSWeaponType WeaponType = ELMSWeaponType::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<ALMSWeaponBase> WeaponClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|First Person")
	TSubclassOf<ALMSWeaponBase> FirstPersonWeaponClass;

	// 이 무기 장착 시 3인칭 몸(CharacterMesh0)에 적용할 AnimBP. 비워두면 기본 AnimBP 유지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimClass;

	// 이 무기 장착 시 1인칭 팔(SK_Murdock_FP_Arms)에 적용할 AnimBP. 로컬 플레이어만. 비워두면 기본 유지.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimClass;

	// 설정 시 이 클래스를 Spawn하여 조준 방향으로 직선 발사 (라이플 Projectile 발사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	TSubclassOf<AHitBox_Projectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	float ProjectileSpeed = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	float ProjectileRadius = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|HUD")
	UTexture2D* HUDIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|HUD")
	bool bShowAmmoOnHUD = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|GAS")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float AttackRate = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Melee")
	bool bCanBlock = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	bool bCanAim = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	float Range = 1000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	int32 MagazineSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	int32 MaxReserveAmmo = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	float ReloadTime = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|GAS")
	TSubclassOf<UGameplayAbility> WeaponSkill;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|GAS")
	TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|GAS")
	TSubclassOf<UGameplayEffect> PassiveEffect;
};
