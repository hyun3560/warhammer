#include "LMSWeaponBase.h"

#include "Components/PrimitiveComponent.h"
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

	if (USkeletalMeshComponent* CharacterMesh = NewOwnerCharacter->GetMesh())
	{
		EquipToComponent(NewOwnerCharacter, CharacterMesh, AttachSocketName);
	}
}

void ALMSWeaponBase::EquipToComponent(ACharacter* NewOwnerCharacter, USceneComponent* AttachParent, FName AttachSocketName)
{
	if (!NewOwnerCharacter || !AttachParent)
	{
		return;
	}

	SetOwner(NewOwnerCharacter);
	AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachSocketName);
}

void ALMSWeaponBase::Unequip()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetOwner(nullptr);
}

void ALMSWeaponBase::SetOwnerVisibilityRules(bool bOnlyOwnerSee, bool bOwnerNoSee, bool bCastShadow)
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;
	GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!PrimitiveComponent)
		{
			continue;
		}

		PrimitiveComponent->SetOnlyOwnerSee(bOnlyOwnerSee);
		PrimitiveComponent->SetOwnerNoSee(bOwnerNoSee);
		PrimitiveComponent->SetCastShadow(bCastShadow);
	}
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
