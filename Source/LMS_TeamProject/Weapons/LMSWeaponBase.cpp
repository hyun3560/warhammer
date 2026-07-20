#include "LMSWeaponBase.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

ALMSWeaponBase::ALMSWeaponBase()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(SceneRoot);
}

void ALMSWeaponBase::Equip(ACharacter* NewOwnerCharacter, FName AttachSocketName)
{
	if (!NewOwnerCharacter)
	{
		return;
	}

	SetOwner(NewOwnerCharacter);

	if (USkeletalMeshComponent* CharacterMesh = NewOwnerCharacter->GetMesh())
	{
		AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
	}
}

void ALMSWeaponBase::Unequip()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
}

FTransform ALMSWeaponBase::GetAttackOriginTransform() const
{
	if (WeaponMesh && WeaponMesh->DoesSocketExist(AttackOriginSocketName))
	{
		return WeaponMesh->GetSocketTransform(AttackOriginSocketName);
	}

	return GetActorTransform();
}

void ALMSWeaponBase::SetWeaponData(const FWeaponData& NewWeaponData)
{
	WeaponData = NewWeaponData;
}
