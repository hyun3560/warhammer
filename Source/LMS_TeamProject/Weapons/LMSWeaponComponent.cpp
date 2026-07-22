#include "LMSWeaponComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "../HitBox_Projectile.h"
#include "LMSWeaponBase.h"
#include "LMSWeaponPrimaryAbility.h"
#include "LMSWeaponSecondaryAbility.h"
#include "LMSWeaponSkillAbility.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "../LMSDamageLibrary.h"
#include "../LMSGameplayAbility.h"

ULMSWeaponComponent::ULMSWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void ULMSWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority() && !DefaultWeaponRowName.IsNone())
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

	CurrentWeapon = SpawnWeaponActor(WeaponData);
	if (!CurrentWeapon)
	{
		return false;
	}

	CurrentWeaponData = WeaponData;
	EquippedWeaponID = CurrentWeaponData.WeaponID;
	CurrentWeapon->SetWeaponData(WeaponData);
	CurrentWeapon->Equip(OwnerCharacter, EquippedSocketName);
	RefreshFirstPersonWeaponVisual();

	AmmoInMagazine = CurrentWeaponData.MagazineSize;
	ReserveAmmo = CurrentWeaponData.MaxReserveAmmo;
	bIsReloading = false;
	bIsBlocking = false;
	bIsAiming = false;
	ResetCombo();
	BroadcastAmmoChanged();
	BroadcastWeaponHUDChanged();

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

	if (FirstPersonWeapon)
	{
		FirstPersonWeapon->Unequip();
		FirstPersonWeapon->Destroy();
		FirstPersonWeapon = nullptr;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ReloadTimerHandle);
		World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);
		World->GetTimerManager().ClearTimer(MeleeDamageBoostTimerHandle);
	}

	CurrentWeaponData = FWeaponData();
	EquippedWeaponID = NAME_None;
	AmmoInMagazine = 0;
	ReserveAmmo = 0;
	bIsReloading = false;
	bIsBlocking = false;
	bIsAiming = false;
	ResetCombo();
	ClearMeleeDamageBoost();
	SkillCooldownEndTime = 0.f;
	SkillCooldownDuration = 0.f;
	BroadcastAmmoChanged();
	BroadcastWeaponHUDChanged();
	BroadcastSkillCooldownChanged(0.f, 0.f);
}

void ULMSWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ULMSWeaponComponent, CurrentWeapon);
	DOREPLIFETIME(ULMSWeaponComponent, EquippedWeaponID);
	DOREPLIFETIME(ULMSWeaponComponent, AmmoInMagazine);
	DOREPLIFETIME(ULMSWeaponComponent, ReserveAmmo);
	DOREPLIFETIME(ULMSWeaponComponent, SkillCooldownEndTime);
	DOREPLIFETIME(ULMSWeaponComponent, SkillCooldownDuration);
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

void ULMSWeaponComponent::BroadcastWeaponHUDChanged()
{
	OnWeaponHUDChanged.Broadcast(ResolveWeaponHUDIcon(), ShouldShowAmmoOnHUD());
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

void ULMSWeaponComponent::CacheWeaponDataByID(FName WeaponID)
{
	if (!WeaponDataTable || WeaponID.IsNone())
	{
		CurrentWeaponData = FWeaponData();
		return;
	}

	if (const FWeaponData* WeaponData = WeaponDataTable->FindRow<FWeaponData>(WeaponID, TEXT("CacheWeaponDataByID"), false))
	{
		CurrentWeaponData = *WeaponData;
		return;
	}

	TArray<FWeaponData*> WeaponRows;
	WeaponDataTable->GetAllRows<FWeaponData>(TEXT("CacheWeaponDataByID"), WeaponRows);

	for (const FWeaponData* WeaponData : WeaponRows)
	{
		if (WeaponData && WeaponData->WeaponID == WeaponID)
		{
			CurrentWeaponData = *WeaponData;
			return;
		}
	}

	CurrentWeaponData = FWeaponData();
}

void ULMSWeaponComponent::RestartReplicatedSkillCooldownTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(SkillCooldownTimerHandle);

	const float CurrentCooldown = FMath::Max(0.f, SkillCooldownEndTime - World->GetTimeSeconds());
	BroadcastSkillCooldownChanged(CurrentCooldown, SkillCooldownDuration);

	if (CurrentCooldown > 0.f && SkillCooldownDuration > 0.f)
	{
		World->GetTimerManager().SetTimer(
			SkillCooldownTimerHandle,
			this,
			&ULMSWeaponComponent::UpdateSkillCooldown,
			0.05f,
			true);
	}
}

void ULMSWeaponComponent::OnRep_EquippedWeaponID()
{
	CacheWeaponDataByID(EquippedWeaponID);

	if (CurrentWeapon)
	{
		CurrentWeapon->SetWeaponData(CurrentWeaponData);
	}

	BroadcastAmmoChanged();
	BroadcastWeaponHUDChanged();
	BroadcastSkillCooldownChanged(0.f, 0.f);
}

void ULMSWeaponComponent::OnRep_CurrentWeapon()
{
	if (!CurrentWeapon)
	{
		if (FirstPersonWeapon)
		{
			FirstPersonWeapon->Unequip();
			FirstPersonWeapon->Destroy();
			FirstPersonWeapon = nullptr;
		}

		BroadcastWeaponHUDChanged();
		return;
	}

	CacheWeaponDataByID(EquippedWeaponID);
	CurrentWeapon->SetWeaponData(CurrentWeaponData);

	if (ACharacter* OwnerCharacter = GetOwnerCharacter())
	{
		CurrentWeapon->Equip(OwnerCharacter, EquippedSocketName);
	}

	RefreshFirstPersonWeaponVisual();
	BroadcastWeaponHUDChanged();
}

void ULMSWeaponComponent::OnRep_Ammo()
{
	BroadcastAmmoChanged();
}

UTexture2D* ULMSWeaponComponent::ResolveWeaponHUDIcon() const
{
	if (CurrentWeaponData.HUDIcon)
	{
		return CurrentWeaponData.HUDIcon;
	}

	const FString WeaponIDString = CurrentWeaponData.WeaponID.ToString();
	const TCHAR* FallbackPath = nullptr;

	if (WeaponIDString.Equals(TEXT("Rifle"), ESearchCase::IgnoreCase))
	{
		FallbackPath = TEXT("/Game/LJH/Image/Gun.Gun");
	}
	else if (WeaponIDString.Equals(TEXT("Hammer"), ESearchCase::IgnoreCase))
	{
		FallbackPath = TEXT("/Game/LJH/Image/Hammer.Hammer");
	}

	return FallbackPath ? LoadObject<UTexture2D>(nullptr, FallbackPath) : nullptr;
}

bool ULMSWeaponComponent::ShouldShowAmmoOnHUD() const
{
	return CurrentWeaponData.bShowAmmoOnHUD || CurrentWeaponData.WeaponType == ELMSWeaponType::Ranged;
}

void ULMSWeaponComponent::OnRep_SkillCooldown()
{
	RestartReplicatedSkillCooldownTimer();
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

ALMSWeaponBase* ULMSWeaponComponent::SpawnWeaponActor(const FWeaponData& WeaponData, TSubclassOf<ALMSWeaponBase> OverrideWeaponClass) const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	TSubclassOf<ALMSWeaponBase> WeaponClassToSpawn = OverrideWeaponClass ? OverrideWeaponClass : WeaponData.WeaponClass;
	if (!OwnerCharacter || !World || !WeaponClassToSpawn)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;

	return World->SpawnActor<ALMSWeaponBase>(WeaponClassToSpawn, SpawnParams);
}

USceneComponent* ULMSWeaponComponent::FindFirstPersonWeaponAttachComponent() const
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || FirstPersonWeaponAttachComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<USceneComponent*> SceneComponents;
	OwnerCharacter->GetComponents<USceneComponent>(SceneComponents);

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (!SceneComponent || SceneComponent == OwnerCharacter->GetMesh())
		{
			continue;
		}

		const FString ComponentName = SceneComponent->GetName();
		const FString TargetName = FirstPersonWeaponAttachComponentName.ToString();

		if (SceneComponent->GetFName() == FirstPersonWeaponAttachComponentName || ComponentName.StartsWith(TargetName))
		{
			return SceneComponent;
		}

		for (const FName& ComponentTag : SceneComponent->ComponentTags)
		{
			if (ComponentTag == FirstPersonWeaponAttachComponentName)
			{
				return SceneComponent;
			}
		}
	}

	UE_LOG(
		LogTemp,
		Warning,
		TEXT("First-person weapon attach component '%s' was not found on %s."),
		*FirstPersonWeaponAttachComponentName.ToString(),
		*GetNameSafe(OwnerCharacter));

	return nullptr;
}

void ULMSWeaponComponent::RefreshFirstPersonWeaponVisual()
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!OwnerCharacter || !CurrentWeapon)
	{
		return;
	}

	if (FirstPersonWeapon)
	{
		FirstPersonWeapon->Unequip();
		FirstPersonWeapon->Destroy();
		FirstPersonWeapon = nullptr;
	}

	if (!OwnerCharacter->IsLocallyControlled() || !bSpawnFirstPersonWeaponVisual)
	{
		CurrentWeapon->SetOwnerVisibilityRules(false, false, true);
		return;
	}

	USceneComponent* FirstPersonAttachComponent = FindFirstPersonWeaponAttachComponent();
	if (!FirstPersonAttachComponent)
	{
		CurrentWeapon->SetOwnerVisibilityRules(false, false, true);
		return;
	}

	if (!FirstPersonEquippedSocketName.IsNone() && !FirstPersonAttachComponent->DoesSocketExist(FirstPersonEquippedSocketName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("First-person weapon socket '%s' does not exist on component '%s'."),
			*FirstPersonEquippedSocketName.ToString(),
			*GetNameSafe(FirstPersonAttachComponent));

		CurrentWeapon->SetOwnerVisibilityRules(false, false, true);
		return;
	}

	FirstPersonWeapon = SpawnWeaponActor(CurrentWeaponData, CurrentWeaponData.FirstPersonWeaponClass);
	if (!FirstPersonWeapon)
	{
		CurrentWeapon->SetOwnerVisibilityRules(false, false, true);
		return;
	}

	FirstPersonWeapon->SetReplicates(false);
	FirstPersonWeapon->SetWeaponData(CurrentWeaponData);
	FirstPersonWeapon->EquipToComponent(OwnerCharacter, FirstPersonAttachComponent, FirstPersonEquippedSocketName);
	FirstPersonWeapon->SetOwnerVisibilityRules(true, false, false);
	FirstPersonWeapon->SetActorEnableCollision(false);

	CurrentWeapon->SetOwnerVisibilityRules(false, true, true);
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

	RegisterComboInput();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee attack requested: %s. Damage is handled by weapon trace notify states."),
		*CurrentWeaponData.WeaponID.ToString());
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

	BeginWeaponTrace(DefaultTraceStartSocketName, DefaultTraceEndSocketName, TraceRadius, DamageMultiplier, bDrawDebugTrace);
	TickWeaponTrace(0.f);
	EndWeaponTrace();

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee skill trace requested: %s DamageMultiplier=%.2f RangeMultiplier=%.2f"),
		*CurrentWeaponData.WeaponID.ToString(),
		DamageMultiplier,
		RangeMultiplier);
}

void ULMSWeaponComponent::ActivateMeleeDamageBoost(float DamageMultiplier, float Duration, int32 BoostedTraceCount)
{
	if (CurrentWeaponData.WeaponType != ELMSWeaponType::Melee || DamageMultiplier <= 1.f || BoostedTraceCount <= 0)
	{
		return;
	}

	MeleeDamageBoostMultiplier = DamageMultiplier;
	RemainingBoostedWeaponTraces = BoostedTraceCount;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MeleeDamageBoostTimerHandle);
		if (Duration > 0.f)
		{
			World->GetTimerManager().SetTimer(MeleeDamageBoostTimerHandle, this, &ULMSWeaponComponent::ClearMeleeDamageBoost, Duration, false);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee damage boost activated: %s BaseDamage=%.1f Multiplier=%.2f BoostedTraces=%d Duration=%.2f"),
		*CurrentWeaponData.WeaponID.ToString(),
		CurrentWeaponData.Damage,
		MeleeDamageBoostMultiplier,
		RemainingBoostedWeaponTraces,
		Duration);
}

void ULMSWeaponComponent::BeginWeaponTrace(FName StartSocketName, FName EndSocketName, float TraceRadius, float DamageMultiplier, bool bDrawDebugTrace)
{
	if (CurrentWeaponData.WeaponType != ELMSWeaponType::Melee)
	{
		return;
	}

	ActiveTraceStartSocketName = StartSocketName.IsNone() ? DefaultTraceStartSocketName : StartSocketName;
	ActiveTraceEndSocketName = EndSocketName.IsNone() ? DefaultTraceEndSocketName : EndSocketName;
	ActiveWeaponTraceRadius = TraceRadius > 0.f ? TraceRadius : WeaponTraceRadius;
	ActiveWeaponTraceDamageMultiplier = DamageMultiplier;
	if (RemainingBoostedWeaponTraces > 0 && MeleeDamageBoostMultiplier > 1.f)
	{
		ActiveWeaponTraceDamageMultiplier *= MeleeDamageBoostMultiplier;
		--RemainingBoostedWeaponTraces;
		if (RemainingBoostedWeaponTraces <= 0)
		{
			ClearMeleeDamageBoost();
		}
	}
	bDrawDebugWeaponTrace = bDrawDebugTrace;
	WeaponTraceHitActors.Reset();

	if (!GetWeaponTraceSocketLocations(PreviousTraceStart, PreviousTraceEnd))
	{
		bIsWeaponTracing = false;
		return;
	}

	bIsWeaponTracing = true;
}

void ULMSWeaponComponent::TickWeaponTrace(float DeltaTime)
{
	if (!bIsWeaponTracing)
	{
		return;
	}

	FVector CurrentTraceStart = FVector::ZeroVector;
	FVector CurrentTraceEnd = FVector::ZeroVector;
	if (!GetWeaponTraceSocketLocations(CurrentTraceStart, CurrentTraceEnd))
	{
		EndWeaponTrace();
		return;
	}

	ACharacter* OwnerCharacter = GetOwnerCharacter();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LMSWeaponTrace), false);
	QueryParams.AddIgnoredActor(OwnerCharacter);
	QueryParams.AddIgnoredActor(CurrentWeapon);
	QueryParams.AddIgnoredActor(FirstPersonWeapon);

	const int32 SampleCount = FMath::Max(WeaponTraceSampleCount, 2);
	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const float Alpha = SampleCount == 1 ? 0.f : static_cast<float>(SampleIndex) / static_cast<float>(SampleCount - 1);
		const FVector PreviousPoint = FMath::Lerp(PreviousTraceStart, PreviousTraceEnd, Alpha);
		const FVector CurrentPoint = FMath::Lerp(CurrentTraceStart, CurrentTraceEnd, Alpha);
		TraceWeaponSegment(PreviousPoint, CurrentPoint, QueryParams);
	}

	PreviousTraceStart = CurrentTraceStart;
	PreviousTraceEnd = CurrentTraceEnd;
}

void ULMSWeaponComponent::EndWeaponTrace()
{
	bIsWeaponTracing = false;
	bDrawDebugWeaponTrace = false;
	ActiveWeaponTraceRadius = WeaponTraceRadius;
	ActiveWeaponTraceDamageMultiplier = 1.f;
	ActiveTraceStartSocketName = NAME_None;
	ActiveTraceEndSocketName = NAME_None;
	PreviousTraceStart = FVector::ZeroVector;
	PreviousTraceEnd = FVector::ZeroVector;
	WeaponTraceHitActors.Reset();
}

ALMSWeaponBase* ULMSWeaponComponent::GetWeaponTraceActor() const
{
	return FirstPersonWeapon ? FirstPersonWeapon : CurrentWeapon;
}

bool ULMSWeaponComponent::GetWeaponTraceSocketLocations(FVector& OutStart, FVector& OutEnd) const
{
	ALMSWeaponBase* TraceWeapon = GetWeaponTraceActor();
	USkeletalMeshComponent* WeaponMesh = TraceWeapon ? TraceWeapon->GetWeaponMesh() : nullptr;
	if (!WeaponMesh)
	{
		return false;
	}

	if (!WeaponMesh->DoesSocketExist(ActiveTraceStartSocketName) || !WeaponMesh->DoesSocketExist(ActiveTraceEndSocketName))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Weapon trace sockets missing. Weapon=%s StartSocket=%s Exists=%d EndSocket=%s Exists=%d"),
			*GetNameSafe(TraceWeapon),
			*ActiveTraceStartSocketName.ToString(),
			WeaponMesh->DoesSocketExist(ActiveTraceStartSocketName),
			*ActiveTraceEndSocketName.ToString(),
			WeaponMesh->DoesSocketExist(ActiveTraceEndSocketName));
		return false;
	}

	OutStart = WeaponMesh->GetSocketLocation(ActiveTraceStartSocketName);
	OutEnd = WeaponMesh->GetSocketLocation(ActiveTraceEndSocketName);
	return true;
}

void ULMSWeaponComponent::TraceWeaponSegment(const FVector& PreviousPoint, const FVector& CurrentPoint, FCollisionQueryParams& QueryParams)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TArray<FHitResult> Hits;
	const bool bHit = World->SweepMultiByChannel(
		Hits,
		PreviousPoint,
		CurrentPoint,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(ActiveWeaponTraceRadius),
		QueryParams);

	for (const FHitResult& Hit : Hits)
	{
		HandleWeaponTraceHit(Hit);
	}

	if (bDrawDebugWeaponTrace)
	{
		const FColor DebugColor = bHit ? FColor::Red : FColor::Green;
		DrawDebugLine(World, PreviousPoint, CurrentPoint, DebugColor, false, 1.f, 0, 2.f);
		if (bHit)
		{
			for (const FHitResult& Hit : Hits)
			{
				DrawDebugSphere(World, Hit.ImpactPoint, ActiveWeaponTraceRadius, 8, FColor::Yellow, false, 1.f);
			}
		}
	}
}

void ULMSWeaponComponent::HandleWeaponTraceHit(const FHitResult& Hit)
{
	AActor* HitActor = Hit.GetActor();
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	if (!HitActor || HitActor == OwnerCharacter || HitActor == CurrentWeapon || HitActor == FirstPersonWeapon)
	{
		return;
	}

	const TWeakObjectPtr<AActor> HitActorPtr(HitActor);
	if (WeaponTraceHitActors.Contains(HitActorPtr))
	{
		return;
	}

	WeaponTraceHitActors.Add(HitActorPtr);

	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	const float TraceDamage = CurrentWeaponData.Damage * ActiveWeaponTraceDamageMultiplier;
	ULMSDamageLibrary::ApplyDamageEffect(OwnerCharacter, HitActor, TraceDamage, CurrentWeaponData.DamageEffect);
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
	QueryParams.AddIgnoredActor(FirstPersonWeapon);

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

void ULMSWeaponComponent::FireRifleProjectile(float DamageMultiplier)
{
	ACharacter* OwnerCharacter = GetOwnerCharacter();
	UWorld* World = GetWorld();
	if (!OwnerCharacter || !World || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	if (!CurrentWeaponData.ProjectileClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("FireRifleProjectile failed: ProjectileClass not set for %s"), *CurrentWeaponData.WeaponID.ToString());
		return;
	}

	const FTransform AttackOrigin = CurrentWeapon ? CurrentWeapon->GetAttackOriginTransform() : FTransform::Identity;
	const FVector SpawnLocation = CurrentWeapon ? AttackOrigin.GetLocation() : OwnerCharacter->GetPawnViewLocation();
	const FRotator AimRotation = OwnerCharacter->GetControlRotation();
	const FVector Direction = AimRotation.Vector();

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = OwnerCharacter;
	SpawnParams.Instigator = OwnerCharacter;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AHitBox_Projectile* Projectile = World->SpawnActor<AHitBox_Projectile>(
		CurrentWeaponData.ProjectileClass,
		SpawnLocation,
		AimRotation,
		SpawnParams);

	if (!Projectile)
	{
		return;
	}

	// 쏜 캐릭터/무기 자신과의 즉시 충돌을 방지 (총구가 몸 안에서 스폰될 때 자기 몸에 맞는 문제)
	Projectile->SetOwner(OwnerCharacter);
	if (UPrimitiveComponent* ProjectileRoot = Cast<UPrimitiveComponent>(Projectile->GetRootComponent()))
	{
		ProjectileRoot->IgnoreActorWhenMoving(OwnerCharacter, true);
		if (CurrentWeapon)
		{
			ProjectileRoot->IgnoreActorWhenMoving(CurrentWeapon, true);
		}
		if (FirstPersonWeapon)
		{
			ProjectileRoot->IgnoreActorWhenMoving(FirstPersonWeapon, true);
		}
	}

	const float ShotDamage = CurrentWeaponData.Damage * DamageMultiplier;
	Projectile->InitializeProjectile(CurrentWeaponData.ProjectileRadius, ShotDamage, CurrentWeaponData.ProjectileSpeed, CurrentWeaponData.ProjectileSpeed);
	Projectile->LaunchStraight(Direction, CurrentWeaponData.ProjectileSpeed);
}

void ULMSWeaponComponent::StartComboAttack(int32 StartingComboIndex)
{
	bIsComboAttacking = true;
	bComboWindowOpen = false;
	bComboInputBuffered = false;
	bComboTransitionQueued = false;
	CurrentComboIndex = FMath::Max(StartingComboIndex, 1);
}

bool ULMSWeaponComponent::HandleMeleeComboInput(UAnimInstance* AnimInstance, UAnimMontage* ComboMontage, const TArray<FName>& ComboSectionNames)
{
	return HandleMeleeComboInputInternal(AnimInstance, ComboMontage, nullptr, nullptr, ComboSectionNames);
}

bool ULMSWeaponComponent::HandleMeleeComboInputLinked(
	UAnimInstance* PrimaryAnimInstance,
	UAnimMontage* PrimaryComboMontage,
	UAnimInstance* LinkedAnimInstance,
	UAnimMontage* LinkedComboMontage,
	const TArray<FName>& ComboSectionNames)
{
	return HandleMeleeComboInputInternal(
		PrimaryAnimInstance,
		PrimaryComboMontage,
		LinkedAnimInstance,
		LinkedComboMontage,
		ComboSectionNames);
}

bool ULMSWeaponComponent::HandleMeleeComboInputInternal(
	UAnimInstance* PrimaryAnimInstance,
	UAnimMontage* PrimaryComboMontage,
	UAnimInstance* LinkedAnimInstance,
	UAnimMontage* LinkedComboMontage,
	const TArray<FName>& ComboSectionNames)
{
	if (!PrimaryAnimInstance || !PrimaryComboMontage || ComboSectionNames.IsEmpty())
	{
		return false;
	}

	if (CurrentWeaponData.WeaponType != ELMSWeaponType::Melee)
	{
		return false;
	}

	if (bIsReloading || bIsBlocking)
	{
		return false;
	}

	if (bIsComboAttacking)
	{
		UAnimInstance* ActiveAnimInstance = ActiveComboAnimInstance.Get();
		UAnimMontage* ActiveMontage = ActiveComboMontage.Get();
		if (ActiveAnimInstance && ActiveMontage && !ActiveAnimInstance->Montage_IsPlaying(ActiveMontage))
		{
			ResetCombo();
		}
	}

	if (!bIsComboAttacking)
	{
		const FName FirstSectionName = ComboSectionNames[0];
		if (FirstSectionName.IsNone())
		{
			return false;
		}

		const float MontageLength = PrimaryAnimInstance->Montage_Play(PrimaryComboMontage, 1.f);
		if (MontageLength <= 0.f)
		{
			return false;
		}

		ActiveComboAnimInstance = PrimaryAnimInstance;
		ActiveComboMontage = PrimaryComboMontage;
		ActiveLinkedComboAnimInstance.Reset();
		ActiveLinkedComboMontage.Reset();
		ActiveComboSectionNames = ComboSectionNames;

		StartComboAttack(1);

		PrimaryAnimInstance->Montage_JumpToSection(FirstSectionName, PrimaryComboMontage);

		if (LinkedAnimInstance && LinkedComboMontage)
		{
			const float LinkedMontageLength = LinkedAnimInstance->Montage_Play(LinkedComboMontage, 1.f);
			if (LinkedMontageLength > 0.f)
			{
				ActiveLinkedComboAnimInstance = LinkedAnimInstance;
				ActiveLinkedComboMontage = LinkedComboMontage;
				LinkedAnimInstance->Montage_JumpToSection(FirstSectionName, LinkedComboMontage);
			}
		}

		return true;
	}

	if (!ActiveComboAnimInstance.IsValid() || !ActiveComboMontage.IsValid())
	{
		ResetCombo();
		return false;
	}

	const int32 CurrentSectionIndex = CurrentComboIndex - 1;
	const int32 NextSectionIndex = CurrentSectionIndex + 1;
	if (!ActiveComboSectionNames.IsValidIndex(CurrentSectionIndex) || !ActiveComboSectionNames.IsValidIndex(NextSectionIndex))
	{
		return true;
	}

	bComboInputBuffered = true;

	if (bComboWindowOpen && !bComboTransitionQueued)
	{
		QueueBufferedComboSection();
	}

	return true;
}

bool ULMSWeaponComponent::QueueBufferedComboSection()
{
	if (!bIsComboAttacking || !bComboWindowOpen || !bComboInputBuffered || bComboTransitionQueued)
	{
		return false;
	}

	UAnimInstance* ActiveAnimInstance = ActiveComboAnimInstance.Get();
	UAnimMontage* ActiveMontage = ActiveComboMontage.Get();
	if (!ActiveAnimInstance || !ActiveMontage)
	{
		ResetCombo();
		return false;
	}

	const int32 CurrentSectionIndex = CurrentComboIndex - 1;
	const int32 NextSectionIndex = CurrentSectionIndex + 1;
	if (!ActiveComboSectionNames.IsValidIndex(CurrentSectionIndex) || !ActiveComboSectionNames.IsValidIndex(NextSectionIndex))
	{
		return true;
	}

	const FName CurrentSectionName = ActiveComboSectionNames[CurrentSectionIndex];
	const FName NextSectionName = ActiveComboSectionNames[NextSectionIndex];
	if (CurrentSectionName.IsNone() || NextSectionName.IsNone())
	{
		return true;
	}

	ActiveAnimInstance->Montage_SetNextSection(CurrentSectionName, NextSectionName, ActiveMontage);

	if (UAnimInstance* LinkedAnimInstance = ActiveLinkedComboAnimInstance.Get())
	{
		if (UAnimMontage* LinkedMontage = ActiveLinkedComboMontage.Get())
		{
			if (LinkedAnimInstance->Montage_IsPlaying(LinkedMontage))
			{
				LinkedAnimInstance->Montage_SetNextSection(CurrentSectionName, NextSectionName, LinkedMontage);
			}
		}
	}

	bComboTransitionQueued = true;
	return true;
}

void ULMSWeaponComponent::StopActiveComboMontage(float BlendOutTime)
{
	UAnimInstance* ActiveAnimInstance = ActiveComboAnimInstance.Get();
	UAnimMontage* ActiveMontage = ActiveComboMontage.Get();
	if (ActiveAnimInstance && ActiveMontage && ActiveAnimInstance->Montage_IsPlaying(ActiveMontage))
	{
		ActiveAnimInstance->Montage_Stop(BlendOutTime, ActiveMontage);
	}

	UAnimInstance* LinkedAnimInstance = ActiveLinkedComboAnimInstance.Get();
	UAnimMontage* LinkedMontage = ActiveLinkedComboMontage.Get();
	if (LinkedAnimInstance && LinkedMontage && LinkedAnimInstance->Montage_IsPlaying(LinkedMontage))
	{
		LinkedAnimInstance->Montage_Stop(BlendOutTime, LinkedMontage);
	}
}

void ULMSWeaponComponent::NotifyComboSectionBegin(int32 ComboIndex)
{
	if (ComboIndex <= 0)
	{
		return;
	}

	const bool bEnteringNewSection = ComboIndex != CurrentComboIndex;

	bIsComboAttacking = true;
	bComboWindowOpen = false;
	if (bEnteringNewSection)
	{
		bComboInputBuffered = false;
		bComboTransitionQueued = false;
	}
	CurrentComboIndex = ComboIndex;
}

void ULMSWeaponComponent::NotifyComboSectionEnd(int32 ComboIndex)
{
	if (!bIsComboAttacking)
	{
		return;
	}

	if (ComboIndex != CurrentComboIndex)
	{
		return;
	}

	const int32 MaxComboIndex = ActiveComboSectionNames.Num();
	const bool bLastComboSection = MaxComboIndex > 0 && ComboIndex >= MaxComboIndex;
	if (bLastComboSection || !bComboTransitionQueued)
	{
		StopActiveComboMontage();
		ResetCombo();
	}
}

bool ULMSWeaponComponent::RegisterComboInput()
{
	if (!bIsComboAttacking)
	{
		StartComboAttack(1);
		return true;
	}

	if (!bComboWindowOpen)
	{
		return false;
	}

	bComboInputBuffered = true;
	QueueBufferedComboSection();
	return true;
}

void ULMSWeaponComponent::OpenComboWindow()
{
	if (bIsComboAttacking)
	{
		bComboWindowOpen = true;
		QueueBufferedComboSection();
	}
}

void ULMSWeaponComponent::CloseComboWindow()
{
	bComboWindowOpen = false;
}

bool ULMSWeaponComponent::QueueComboSection(UAnimInstance* AnimInstance, UAnimMontage* ComboMontage, FName CurrentSection, FName NextSection, int32 NextComboIndex)
{
	if (!bIsComboAttacking || !bComboWindowOpen || !AnimInstance || !ComboMontage || CurrentSection.IsNone() || NextSection.IsNone())
	{
		return false;
	}

	AnimInstance->Montage_SetNextSection(CurrentSection, NextSection, ComboMontage);
	if (UAnimInstance* LinkedAnimInstance = ActiveLinkedComboAnimInstance.Get())
	{
		if (UAnimMontage* LinkedMontage = ActiveLinkedComboMontage.Get())
		{
			if (LinkedAnimInstance->Montage_IsPlaying(LinkedMontage))
			{
				LinkedAnimInstance->Montage_SetNextSection(CurrentSection, NextSection, LinkedMontage);
			}
		}
	}

	bComboInputBuffered = true;
	bComboTransitionQueued = true;
	CurrentComboIndex = FMath::Max(NextComboIndex, CurrentComboIndex + 1);
	return true;
}

bool ULMSWeaponComponent::ConsumeBufferedComboInput()
{
	const bool bHadBufferedInput = bComboInputBuffered;
	bComboInputBuffered = false;
	return bHadBufferedInput;
}

void ULMSWeaponComponent::ResetCombo()
{
	bIsComboAttacking = false;
	bComboWindowOpen = false;
	bComboInputBuffered = false;
	bComboTransitionQueued = false;
	CurrentComboIndex = 0;
	ActiveComboAnimInstance.Reset();
	ActiveComboMontage.Reset();
	ActiveLinkedComboAnimInstance.Reset();
	ActiveLinkedComboMontage.Reset();
	ActiveComboSectionNames.Reset();
}

void ULMSWeaponComponent::ClearMeleeDamageBoost()
{
	MeleeDamageBoostMultiplier = 1.f;
	RemainingBoostedWeaponTraces = 0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MeleeDamageBoostTimerHandle);
	}
}

void ULMSWeaponComponent::StartBlock()
{
	if (CurrentWeaponData.WeaponType != ELMSWeaponType::Melee || !CurrentWeaponData.bCanBlock || bIsReloading)
	{
		return;
	}

	if (bIsBlocking)
	{
		return;
	}

	StopActiveComboMontage();
	ResetCombo();
	EndWeaponTrace();

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
