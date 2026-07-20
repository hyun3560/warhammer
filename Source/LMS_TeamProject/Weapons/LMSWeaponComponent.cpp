#include "LMSWeaponComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "LMSWeaponBase.h"
#include "LMSWeaponPrimaryAbility.h"
#include "LMSWeaponSecondaryAbility.h"
#include "LMSWeaponSkillAbility.h"
#include "TimerManager.h"
#include "../LMSGameplayAbility.h"

ULMSWeaponComponent::ULMSWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULMSWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!DefaultWeaponRowName.IsNone())
	{
		EquipWeaponByRowName(DefaultWeaponRowName);
	}
}

bool ULMSWeaponComponent::EquipWeaponFromData(const FWeaponData& WeaponData)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !WeaponData.WeaponClass)
	{
		return false;
	}

	UnequipCurrentWeapon();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;

	CurrentWeapon = GetWorld()->SpawnActor<ALMSWeaponBase>(WeaponData.WeaponClass, SpawnParams);
	if (!CurrentWeapon)
	{
		return false;
	}

	CurrentWeaponData = WeaponData;
	CurrentWeapon->SetWeaponData(WeaponData);
	CurrentWeapon->Equip(OwnerCharacter, EquippedSocketName);

	AmmoInMagazine = CurrentWeaponData.MagazineSize;
	ReserveAmmo = CurrentWeaponData.MaxReserveAmmo;
	bIsReloading = false;
	bIsBlocking = false;
	bIsAiming = false;
	BroadcastAmmoChanged();

	GrantCurrentWeaponAbilities();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Equipped weapon: %s GrantedAbilities=%d WeaponSkill=%s"),
		*CurrentWeaponData.WeaponID.ToString(),
		CurrentWeaponData.GrantedAbilities.Num(),
		*GetNameSafe(CurrentWeaponData.WeaponSkill));

	return true;
}

bool ULMSWeaponComponent::EquipWeaponByRowName(FName RowName)
{
	return EquipWeaponByID(RowName);
}

bool ULMSWeaponComponent::EquipWeaponByID(FName WeaponID)
{
	if (!WeaponDataTable || WeaponID.IsNone())
	{
		return false;
	}

	if (const FWeaponData* WeaponData = WeaponDataTable->FindRow<FWeaponData>(WeaponID, TEXT("EquipWeaponByID"), false))
	{
		return EquipWeaponFromData(*WeaponData);
	}

	TArray<FWeaponData*> WeaponRows;
	WeaponDataTable->GetAllRows<FWeaponData>(TEXT("EquipWeaponByID"), WeaponRows);

	for (const FWeaponData* WeaponData : WeaponRows)
	{
		if (WeaponData && WeaponData->WeaponID == WeaponID)
		{
			return EquipWeaponFromData(*WeaponData);
		}
	}

	return false;
}

void ULMSWeaponComponent::UnequipCurrentWeapon()
{
	ClearGrantedWeaponAbilities();

	if (CurrentWeapon)
	{
		CurrentWeapon->Unequip();
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
		World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
	}

	CurrentWeaponData = FWeaponData();
	AmmoInMagazine = 0;
	ReserveAmmo = 0;
	bIsReloading = false;
	bIsBlocking = false;
	bIsAiming = false;
	SkillCooldownEndTime = 0.f;
	SkillCooldownDuration = 0.f;
	BroadcastAmmoChanged();
	BroadcastSkillCooldownChanged(0.f, 0.f);
}

void ULMSWeaponComponent::StartAttack()
{
	if (!CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartAttack ignored because CurrentWeapon is null."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("StartAttack: %s Type=%d"), *CurrentWeaponData.WeaponID.ToString(), static_cast<int32>(CurrentWeaponData.WeaponType));

	switch (CurrentWeaponData.WeaponType)
	{
	case ELMSWeaponType::Melee:
		StartMeleeAttack();
		break;
	case ELMSWeaponType::Ranged:
		StartRangedAttack();
		break;
	default:
		break;
	}
}

void ULMSWeaponComponent::StopAttack()
{
	UE_LOG(LogTemp, Log, TEXT("StopAttack: %s"), *CurrentWeaponData.WeaponID.ToString());
}

void ULMSWeaponComponent::StartSecondaryAction()
{
	if (!CurrentWeapon)
	{
		return;
	}

	switch (CurrentWeaponData.WeaponType)
	{
	case ELMSWeaponType::Melee:
		StartBlock();
		break;
	case ELMSWeaponType::Ranged:
		StartAim();
		break;
	default:
		break;
	}
}

void ULMSWeaponComponent::StopSecondaryAction()
{
	StopBlock();
	StopAim();
}

void ULMSWeaponComponent::Reload()
{
	if (!CanReload())
	{
		return;
	}

	bIsReloading = true;
	UE_LOG(LogTemp, Log, TEXT("Reload started: %s"), *CurrentWeaponData.WeaponID.ToString());

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ReloadTimerHandle, this, &ULMSWeaponComponent::FinishReload, CurrentWeaponData.ReloadTime, false);
	}
}

void ULMSWeaponComponent::RefreshGrantedAbilities()
{
	if (CurrentWeapon)
	{
		GrantCurrentWeaponAbilities();
	}
}

bool ULMSWeaponComponent::TryConsumeAmmo(int32 AmmoCost, bool bReloadIfEmpty)
{
	if (bIsReloading)
	{
		return false;
	}

	if (AmmoCost <= 0)
	{
		return true;
	}

	if (AmmoInMagazine < AmmoCost)
	{
		if (bReloadIfEmpty && AmmoInMagazine <= 0)
		{
			Reload();
		}

		return false;
	}

	AmmoInMagazine -= AmmoCost;
	BroadcastAmmoChanged();
	return true;
}

void ULMSWeaponComponent::BroadcastAmmoChanged()
{
	OnAmmoChanged.Broadcast(AmmoInMagazine, ReserveAmmo);
}

void ULMSWeaponComponent::StartSkillCooldown(float CurrentCooldown, float MaxCooldown)
{
	UWorld* World = GetWorld();
	if (!World || CurrentCooldown <= 0.f || MaxCooldown <= 0.f)
	{
		if (World)
		{
			World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		}

		SkillCooldownEndTime = 0.f;
		SkillCooldownDuration = 0.f;
		BroadcastSkillCooldownChanged(0.f, FMath::Max(MaxCooldown, 0.f));
		return;
	}

	SkillCooldownDuration = MaxCooldown;
	SkillCooldownEndTime = World->GetTimeSeconds() + CurrentCooldown;
	BroadcastSkillCooldownChanged(CurrentCooldown, SkillCooldownDuration);

	World->GetTimerManager().SetTimer(
		SkillCooldownTimerHandle,
		this,
		&ULMSWeaponComponent::UpdateSkillCooldown,
		0.05f,
		true);
}

void ULMSWeaponComponent::UpdateSkillCooldown()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentCooldown = FMath::Max(0.f, SkillCooldownEndTime - World->GetTimeSeconds());
	BroadcastSkillCooldownChanged(CurrentCooldown, SkillCooldownDuration);

	if (CurrentCooldown <= 0.f)
	{
		World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		SkillCooldownEndTime = 0.f;
		SkillCooldownDuration = 0.f;
	}
}

void ULMSWeaponComponent::BroadcastSkillCooldownChanged(float CurrentCooldown, float MaxCooldown)
{
	OnSkillCooldownChanged.Broadcast(CurrentCooldown, MaxCooldown);
}

ACharacter* ULMSWeaponComponent::GetOwnerCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

UCameraComponent* ULMSWeaponComponent::GetOwnerCameraComponent() const
{
	AActor* OwnerActor = GetOwner();
	return OwnerActor ? OwnerActor->FindComponentByClass<UCameraComponent>() : nullptr;
}

UAbilitySystemComponent* ULMSWeaponComponent::GetOwnerAbilitySystemComponent() const
{
	const IAbilitySystemInterface* AbilitySystemOwner = Cast<IAbilitySystemInterface>(GetOwner());
	return AbilitySystemOwner ? AbilitySystemOwner->GetAbilitySystemComponent() : nullptr;
}

void ULMSWeaponComponent::GrantCurrentWeaponAbilities()
{
	ClearGrantedWeaponAbilities();

	for (const TSubclassOf<UGameplayAbility>& AbilityClass : CurrentWeaponData.GrantedAbilities)
	{
		GrantWeaponAbility(AbilityClass);
	}

	GrantWeaponAbility(CurrentWeaponData.WeaponSkill);
}

void ULMSWeaponComponent::ClearGrantedWeaponAbilities()
{
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (!AbilitySystemComponent || GrantedAbilityHandles.IsEmpty())
	{
		GrantedAbilityHandles.Reset();
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& AbilityHandle : GrantedAbilityHandles)
	{
		if (AbilityHandle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(AbilityHandle);
		}
	}

	GrantedAbilityHandles.Reset();
}

void ULMSWeaponComponent::GrantWeaponAbility(TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!AbilityClass)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* AbilitySystemComponent = GetOwnerAbilitySystemComponent();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !AbilitySystemComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("GrantWeaponAbility failed. Owner=%s HasAuthority=%d ASC=%s AbilityClass=%s"),
			*GetNameSafe(OwnerActor),
			OwnerActor ? OwnerActor->HasAuthority() : false,
			*GetNameSafe(AbilitySystemComponent),
			*GetNameSafe(AbilityClass));
		return;
	}

	const ULMSGameplayAbility* AbilityCDO = Cast<ULMSGameplayAbility>(AbilityClass->GetDefaultObject());
	const int32 InputID = AbilityCDO ? static_cast<int32>(AbilityCDO->AbilityInputID) : INDEX_NONE;
	int32 ResolvedInputID = InputID;

	if (Cast<ULMSWeaponPrimaryAbility>(AbilityCDO))
	{
		ResolvedInputID = static_cast<int32>(ELMSAbilityInputID::PrimaryAttack);
	}
	else if (Cast<ULMSWeaponSecondaryAbility>(AbilityCDO))
	{
		ResolvedInputID = static_cast<int32>(ELMSAbilityInputID::SecondaryAttack);
	}
	else if (Cast<ULMSWeaponSkillAbility>(AbilityCDO))
	{
		ResolvedInputID = static_cast<int32>(ELMSAbilityInputID::WeaponSkill);
	}

	FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, ResolvedInputID, CurrentWeapon);
	const FGameplayAbilitySpecHandle AbilityHandle = AbilitySystemComponent->GiveAbility(AbilitySpec);
	if (AbilityHandle.IsValid())
	{
		GrantedAbilityHandles.Add(AbilityHandle);
		UE_LOG(LogTemp, Log, TEXT("Granted weapon ability: %s InputID=%d"), *GetNameSafe(AbilityClass), ResolvedInputID);
	}
}

void ULMSWeaponComponent::StartMeleeAttack()
{
	if (bIsReloading || bIsBlocking)
	{
		return;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World)
	{
		return;
	}

	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const FVector Start = OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, 50.f) + Forward * 50.f;
	const FVector End = Start + Forward * CurrentWeaponData.Range;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LMSMeleeAttack), false);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(CurrentWeapon);

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(MeleeTraceRadius),
		QueryParams);

	TSet<AActor*> DamagedActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == OwnerCharacter || HitActor == CurrentWeapon || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);

		if (OwnerCharacter->HasAuthority())
		{
			UGameplayStatics::ApplyDamage(
				HitActor,
				CurrentWeaponData.Damage,
				OwnerCharacter->GetController(),
				CurrentWeapon ? Cast<AActor>(CurrentWeapon) : Cast<AActor>(OwnerCharacter),
				UDamageType::StaticClass());
		}
	}

	if (bDrawDebugMeleeTrace)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(World, Start, End, DebugColor, false, 1.5f, 0, 2.f);
		DrawDebugSphere(World, End, MeleeTraceRadius, 16, DebugColor, false, 1.5f);

		for (const FHitResult& Hit : Hits)
		{
			DrawDebugSphere(World, Hit.ImpactPoint, 16.f, 8, FColor::Yellow, false, 1.5f);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee attack: %s Hits=%d Damage=%.1f"),
		*CurrentWeaponData.WeaponID.ToString(),
		DamagedActors.Num(),
		CurrentWeaponData.Damage);
}

void ULMSWeaponComponent::StartRangedAttack()
{
	if (!TryConsumeAmmo(1, true))
	{
		return;
	}

	FireRangedShot(1.f, 1.f, bDrawDebugRangedTrace);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Ranged attack: %s Ammo=%d/%d"),
		*CurrentWeaponData.WeaponID.ToString(),
		AmmoInMagazine,
		ReserveAmmo);
}

void ULMSWeaponComponent::PerformMeleeSkillSweep(float DamageMultiplier, float RangeMultiplier, float TraceRadius, bool bDrawDebugTrace)
{
	if (bIsReloading || bIsBlocking)
	{
		return;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World)
	{
		return;
	}

	const FVector Forward = OwnerCharacter->GetActorForwardVector();
	const FVector Start = OwnerCharacter->GetActorLocation() + FVector(0.f, 0.f, 50.f) + Forward * 50.f;
	const FVector End = Start + Forward * CurrentWeaponData.Range * RangeMultiplier;
	const float SkillDamage = CurrentWeaponData.Damage * DamageMultiplier;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LMSMeleeSkill), false);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(CurrentWeapon);

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits,
		Start,
		End,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(TraceRadius),
		QueryParams);

	TSet<AActor*> DamagedActors;
	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor || HitActor == OwnerCharacter || HitActor == CurrentWeapon || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		DamagedActors.Add(HitActor);

		if (OwnerCharacter->HasAuthority())
		{
			UGameplayStatics::ApplyDamage(
				HitActor,
				SkillDamage,
				OwnerCharacter->GetController(),
				CurrentWeapon ? Cast<AActor>(CurrentWeapon) : Cast<AActor>(OwnerCharacter),
				UDamageType::StaticClass());
		}
	}

	if (bDrawDebugTrace)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(World, Start, End, DebugColor, false, 1.5f, 0, 3.f);
		DrawDebugSphere(World, End, TraceRadius, 20, DebugColor, false, 1.5f);

		for (const FHitResult& Hit : Hits)
		{
			DrawDebugSphere(World, Hit.ImpactPoint, 20.f, 10, FColor::Yellow, false, 1.5f);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee skill: %s Hits=%d Damage=%.1f"),
		*CurrentWeaponData.WeaponID.ToString(),
		DamagedActors.Num(),
		SkillDamage);
}

void ULMSWeaponComponent::FireRangedShot(float DamageMultiplier, float RangeMultiplier, bool bDrawDebugTrace)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World)
	{
		return;
	}

	const FTransform AttackOrigin = CurrentWeapon ? CurrentWeapon->GetAttackOriginTransform() : FTransform::Identity;
	const FVector Start = CurrentWeapon ? AttackOrigin.GetLocation() : OwnerCharacter->GetPawnViewLocation();
	const FRotator AimRotation = OwnerCharacter->GetControlRotation();
	const FVector Direction = AimRotation.Vector();
	const FVector End = Start + Direction * CurrentWeaponData.Range * RangeMultiplier;
	const float ShotDamage = CurrentWeaponData.Damage * DamageMultiplier;

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LMSRangedShot), false);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(CurrentWeapon);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_Visibility,
		QueryParams);

	if (bHit)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (OwnerCharacter->HasAuthority())
			{
				UGameplayStatics::ApplyDamage(
					HitActor,
					ShotDamage,
					OwnerCharacter->GetController(),
					CurrentWeapon ? Cast<AActor>(CurrentWeapon) : Cast<AActor>(OwnerCharacter),
					UDamageType::StaticClass());
			}
		}
	}

	if (bDrawDebugTrace)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		const FVector TraceEnd = bHit ? Hit.ImpactPoint : End;
		DrawDebugLine(World, Start, TraceEnd, DebugColor, false, 1.5f, 0, 2.f);

		if (bHit)
		{
			DrawDebugSphere(World, Hit.ImpactPoint, 12.f, 8, FColor::Yellow, false, 1.5f);
		}
	}
}

void ULMSWeaponComponent::StartBlock()
{
	if (CurrentWeaponData.WeaponType != ELMSWeaponType::Melee || !CurrentWeaponData.bCanBlock || bIsReloading)
	{
		return;
	}

	bIsBlocking = true;
	UE_LOG(LogTemp, Log, TEXT("Block started: %s"), *CurrentWeaponData.WeaponID.ToString());
}

void ULMSWeaponComponent::StopBlock()
{
	if (!bIsBlocking)
	{
		return;
	}

	bIsBlocking = false;
	UE_LOG(LogTemp, Log, TEXT("Block stopped: %s"), *CurrentWeaponData.WeaponID.ToString());
}

void ULMSWeaponComponent::StartAim()
{
	if (CurrentWeaponData.WeaponType != ELMSWeaponType::Ranged || !CurrentWeaponData.bCanAim || bIsReloading || bIsAiming)
	{
		return;
	}

	if (UCameraComponent* CameraComponent = GetOwnerCameraComponent())
	{
		DefaultFOV = CameraComponent->FieldOfView;
		CameraComponent->SetFieldOfView(AimFOV);
	}

	bIsAiming = true;
	UE_LOG(LogTemp, Log, TEXT("Aim started: %s"), *CurrentWeaponData.WeaponID.ToString());
}

void ULMSWeaponComponent::StopAim()
{
	if (!bIsAiming)
	{
		return;
	}

	if (UCameraComponent* CameraComponent = GetOwnerCameraComponent())
	{
		CameraComponent->SetFieldOfView(DefaultFOV);
	}

	bIsAiming = false;
	UE_LOG(LogTemp, Log, TEXT("Aim stopped: %s"), *CurrentWeaponData.WeaponID.ToString());
}

bool ULMSWeaponComponent::CanReload() const
{
	return CurrentWeapon
		&& CurrentWeaponData.WeaponType == ELMSWeaponType::Ranged
		&& !bIsReloading
		&& !bIsAiming
		&& CurrentWeaponData.MagazineSize > 0
		&& AmmoInMagazine < CurrentWeaponData.MagazineSize
		&& ReserveAmmo > 0;
}

void ULMSWeaponComponent::FinishReload()
{
	if (!bIsReloading)
	{
		return;
	}

	const int32 NeededAmmo = CurrentWeaponData.MagazineSize - AmmoInMagazine;
	const int32 AmmoToLoad = FMath::Min(NeededAmmo, ReserveAmmo);

	AmmoInMagazine += AmmoToLoad;
	ReserveAmmo -= AmmoToLoad;
	bIsReloading = false;
	BroadcastAmmoChanged();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Reload finished: %s Ammo=%d/%d"),
		*CurrentWeaponData.WeaponID.ToString(),
		AmmoInMagazine,
		ReserveAmmo);
}
