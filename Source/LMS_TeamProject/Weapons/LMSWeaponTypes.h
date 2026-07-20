#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LMSWeaponTypes.generated.h"

class ALMSWeaponBase;
class UGameplayAbility;
class UGameplayEffect;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	float Damage = 10.f;

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
