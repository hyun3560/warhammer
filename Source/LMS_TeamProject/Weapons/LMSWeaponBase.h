#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LMSWeaponTypes.h"
#include "LMSWeaponBase.generated.h"

class ACharacter;
class USceneComponent;
class USkeletalMeshComponent;

UCLASS()
class LMS_TEAMPROJECT_API ALMSWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	ALMSWeaponBase();

	virtual void Equip(ACharacter* NewOwnerCharacter, FName AttachSocketName);
	virtual void Unequip();

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FTransform GetAttackOriginTransform() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	const FWeaponData& GetWeaponData() const { return WeaponData; }

	void SetWeaponData(const FWeaponData& NewWeaponData);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	FName AttackOriginSocketName = TEXT("Muzzle");

	UPROPERTY(BlueprintReadOnly, Category = "Weapon")
	FWeaponData WeaponData;
};
