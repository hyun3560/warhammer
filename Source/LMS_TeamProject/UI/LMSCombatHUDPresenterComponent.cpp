#include "LMSCombatHUDPresenterComponent.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "../Weapons/LMSWeaponComponent.h"
#include "../LMSAttributeSet.h"
#include "../LMS_TeamProjectPlayerState.h"
#include "LMSCombatHUDWidget.h"
#include "UIManagerComponent.h"
#include "GameplayTagContainer.h"

ULMSCombatHUDPresenterComponent::ULMSCombatHUDPresenterComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULMSCombatHUDPresenterComponent::InitializeCombatHUD()
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	CacheUIManagerComponent();
	CacheCombatHUDWidget();
	CacheAttributeSource();
	CacheWeaponSource();

	BindAttributeDelegates();
	BindWeaponDelegates();
	BindStateTagDelegates();

	UpdateAllCombatHUD();
}

void ULMSCombatHUDPresenterComponent::CacheUIManagerComponent()
{
	if (UIManagerComponent)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController)
	{
		return;
	}

	UIManagerComponent = PlayerController->FindComponentByClass<UUIManagerComponent>();
}

void ULMSCombatHUDPresenterComponent::CacheCombatHUDWidget()
{
	if (CombatHUDWidget || !UIManagerComponent)
	{
		return;
	}

	// UIManagerComponent가 Combat 타입으로 생성해 둔 위젯을 가져옵니다.
	// 이 위젯은 WBP_CombatHUD이고, 부모 클래스가 ULMSCombatHUDWidget이어야 합니다.
	CombatHUDWidget = Cast<ULMSCombatHUDWidget>(UIManagerComponent->GetUI(ELMSUIType::Combat));
}

void ULMSCombatHUDPresenterComponent::CacheAttributeSource()
{
	if (CachedAbilitySystemComponent && CachedAttributeSet)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController)
	{
		return;
	}

	ALMS_TeamProjectPlayerState* LMSPlayerState = PlayerController->GetPlayerState<ALMS_TeamProjectPlayerState>();
	if (!LMSPlayerState)
	{
		return;
	}

	CachedAbilitySystemComponent = LMSPlayerState->GetAbilitySystemComponent();
	CachedAttributeSet = LMSPlayerState->GetAttributeSet();
}

void ULMSCombatHUDPresenterComponent::CacheWeaponSource()
{
	if (CachedWeaponComponent)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController)
	{
		return;
	}

	const APawn* ControlledPawn = PlayerController->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	CachedWeaponComponent = ControlledPawn->FindComponentByClass<ULMSWeaponComponent>();
}

void ULMSCombatHUDPresenterComponent::BindWeaponDelegates()
{
	if (bWeaponDelegatesBound || !CachedWeaponComponent)
	{
		return;
	}

	CachedWeaponComponent->OnAmmoChanged.AddDynamic(
		this,
		&ULMSCombatHUDPresenterComponent::HandleAmmoChanged
	);

	CachedWeaponComponent->OnSkillCooldownChanged.AddDynamic(
		this,
		&ULMSCombatHUDPresenterComponent::HandleSkillCooldownChanged
	);

	bWeaponDelegatesBound = true;
}

void ULMSCombatHUDPresenterComponent::HandleAmmoChanged(int32 CurrentAmmo, int32 ReserveAmmo)
{
	UpdateAmmoUI(CurrentAmmo, ReserveAmmo);
}

// 무기 스킬 쿨타임이 바뀌면 HUD 스킬 쿨타임 UI를 갱신합니다.
void ULMSCombatHUDPresenterComponent::HandleSkillCooldownChanged(float CurrentCooldown, float MaxCooldown)
{
	UpdateSkillCooldownUI(CurrentCooldown, MaxCooldown);
}

void ULMSCombatHUDPresenterComponent::BindAttributeDelegates()
{
	if (bAttributeDelegatesBound || !CachedAbilitySystemComponent || !CachedAttributeSet)
	{
		return;
	}

	// GAS가 제공하는 Attribute 변경 델리게이트에 UI 갱신 함수를 연결합니다.
	// 값이 바뀔 때만 HUD를 갱신하므로 Tick으로 매 프레임 읽는 방식보다 부담이 적습니다.
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetHealthAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleHealthChanged);
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetMaxHealthAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleMaxHealthChanged);
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetShieldAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleShieldChanged);
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetMaxShieldAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleMaxShieldChanged);
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetStaminaAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleStaminaChanged);
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetMaxStaminaAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleMaxStaminaChanged);
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetIncapHealthAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleIncapHealthChanged);
	CachedAbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(CachedAttributeSet->GetMaxIncapHealthAttribute())
		.AddUObject(this, &ULMSCombatHUDPresenterComponent::HandleMaxIncapHealthChanged);

	bAttributeDelegatesBound = true;
}

void ULMSCombatHUDPresenterComponent::BindStateTagDelegates()
{
	if (bStateTagDelegatesBound || !CachedAbilitySystemComponent)
	{
		return;
	}

	const FGameplayTag IncapacitatedTag =
		FGameplayTag::RequestGameplayTag(TEXT("state.Incapacitated"));

	CachedAbilitySystemComponent->RegisterGameplayTagEvent(
		IncapacitatedTag,
		EGameplayTagEventType::NewOrRemoved
	).AddUObject(
		this,
		&ULMSCombatHUDPresenterComponent::HandleIncapacitatedTagChanged
	);

	bStateTagDelegatesBound = true;
}

void ULMSCombatHUDPresenterComponent::UpdateGroggyStateUI() const
{
	if (!CombatHUDWidget || !CachedAbilitySystemComponent)
	{
		return;
	}

	const FGameplayTag IncapacitatedTag =
		FGameplayTag::RequestGameplayTag(TEXT("state.Incapacitated"));

	const bool bIsGroggy =
		CachedAbilitySystemComponent->HasMatchingGameplayTag(IncapacitatedTag);

	CombatHUDWidget->SetGroggyState(bIsGroggy);
}

void ULMSCombatHUDPresenterComponent::HandleIncapacitatedTagChanged(
	const FGameplayTag CallbackTag,
	int32 NewCount)
{
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetGroggyState(NewCount > 0);
	UpdateHealthUI();
}

void ULMSCombatHUDPresenterComponent::UpdateAllCombatHUD()
{
	UpdateHealthUI();
	UpdateShieldUI();
	UpdateStaminaUI();
	UpdateGroggyStateUI();

	if (CachedWeaponComponent)
	{
		UpdateAmmoUI(
			CachedWeaponComponent->GetAmmoInMagazine(),
			CachedWeaponComponent->GetReserveAmmo()
		);
	}
}

void ULMSCombatHUDPresenterComponent::UpdateHealthUI()
{
	if (!CombatHUDWidget || !CachedAttributeSet)
	{
		return;
	}

	bool bIsGroggy = false;

	if (CachedAbilitySystemComponent)
	{
		const FGameplayTag IncapacitatedTag =
			FGameplayTag::RequestGameplayTag(TEXT("state.Incapacitated"));

		bIsGroggy = CachedAbilitySystemComponent->HasMatchingGameplayTag(IncapacitatedTag);
	}

	if (bIsGroggy)
	{
		CombatHUDWidget->SetHealth(
			CachedAttributeSet->GetIncapHealth(),
			CachedAttributeSet->GetMaxIncapHealth()
		);
		return;
	}

	CombatHUDWidget->SetHealth(
		CachedAttributeSet->GetHealth(),
		CachedAttributeSet->GetMaxHealth()
	);
}

void ULMSCombatHUDPresenterComponent::UpdateShieldUI()
{
	if (!CombatHUDWidget || !CachedAttributeSet)
	{
		return;
	}

	CombatHUDWidget->SetShield(CachedAttributeSet->GetShield(), CachedAttributeSet->GetMaxShield());
}

void ULMSCombatHUDPresenterComponent::UpdateStaminaUI()
{
	if (!CombatHUDWidget || !CachedAttributeSet)
	{
		return;
	}

	CombatHUDWidget->SetStamina(CachedAttributeSet->GetStamina(), CachedAttributeSet->GetMaxStamina());
}

void ULMSCombatHUDPresenterComponent::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthUI();
}

void ULMSCombatHUDPresenterComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthUI();
}

void ULMSCombatHUDPresenterComponent::HandleIncapHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthUI();
}

void ULMSCombatHUDPresenterComponent::HandleMaxIncapHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthUI();
}

void ULMSCombatHUDPresenterComponent::HandleShieldChanged(const FOnAttributeChangeData& Data)
{
	UpdateShieldUI();
}

void ULMSCombatHUDPresenterComponent::HandleMaxShieldChanged(const FOnAttributeChangeData& Data)
{
	UpdateShieldUI();
}

void ULMSCombatHUDPresenterComponent::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
	UpdateStaminaUI();
}

void ULMSCombatHUDPresenterComponent::HandleMaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	UpdateStaminaUI();
}

void ULMSCombatHUDPresenterComponent::UpdateSkillCooldownUI(float CurrentCooldown, float MaxCooldown) const
{
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetSkillCooldown(CurrentCooldown, MaxCooldown);
}

void ULMSCombatHUDPresenterComponent::UpdateAmmoUI(int32 CurrentAmmo, int32 ReserveAmmo) const
{
	// 무기 컴포넌트에서 받은 탄약 값을 HUD BP 이벤트로 넘깁니다.
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetAmmo(CurrentAmmo, ReserveAmmo);
}

void ULMSCombatHUDPresenterComponent::ShowInteractionPrompt(
	const FText& KeyText,
	const FText& InteractionText) const
{
	UpdateInteractionPrompt(KeyText, InteractionText);
	UpdateInteractionVisible(true);
}

void ULMSCombatHUDPresenterComponent::HideInteractionPrompt() const
{
	UpdateInteractionVisible(false);
}

void ULMSCombatHUDPresenterComponent::ShowInteractionProgress() const
{
	UpdateInteractionProgressVisible(true);
}

void ULMSCombatHUDPresenterComponent::SetInteractionProgressValue(float Progress) const
{
	UpdateInteractionProgress(FMath::Clamp(Progress, 0.f, 1.f));
}

void ULMSCombatHUDPresenterComponent::HideInteractionProgress() const
{
	UpdateInteractionProgressVisible(false);
	UpdateInteractionProgress(0.f);
}

void ULMSCombatHUDPresenterComponent::ShowTeamMemberStatus(
	int32 MemberIndex,
	const FText& Nickname,
	float CurrentHealth,
	float MaxHealth,
	float CurrentShield,
	float MaxShield) const
{
	// 현재 WBP_CombatHUD에는 팀원 슬롯이 0, 1, 2번까지만 준비되어 있습니다.
	// 잘못된 인덱스가 들어오면 BP까지 넘기지 않고 여기서 막습니다.
	if (MemberIndex < 0 || MemberIndex > 2)
	{
		return;
	}

	// 팀원 상태 값은 기존 내부 전달 함수로 HUD BP에 넘깁니다.
	UpdateTeamMemberStatus(
		MemberIndex,
		Nickname,
		CurrentHealth,
		MaxHealth,
		CurrentShield,
		MaxShield
	);

	// 값 갱신 후 해당 팀원 슬롯을 보이게 만듭니다.
	UpdateTeamMemberVisible(MemberIndex, true);
}

void ULMSCombatHUDPresenterComponent::HideTeamMemberStatus(int32 MemberIndex) const
{
	// 현재 WBP_CombatHUD에는 팀원 슬롯이 0, 1, 2번까지만 준비되어 있습니다.
	// 잘못된 인덱스가 들어오면 BP까지 넘기지 않고 여기서 막습니다.
	if (MemberIndex < 0 || MemberIndex > 2)
	{
		return;
	}

	// 팀원이 없거나 정보가 준비되지 않은 슬롯은 숨깁니다.
	UpdateTeamMemberVisible(MemberIndex, false);
}

void ULMSCombatHUDPresenterComponent::UpdateInteractionPrompt(const FText& KeyText, const FText& InteractionText) const
{
	// 상호작용 시스템이 결정한 표시 문구를 HUD BP 이벤트로 넘깁니다.
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetInteractionPrompt(KeyText, InteractionText);
}

void ULMSCombatHUDPresenterComponent::UpdateInteractionVisible(bool bVisible) const
{
	// 상호작용 가능 여부에 따라 HUD의 상호작용 패널을 표시하거나 숨깁니다.
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetInteractionVisible(bVisible);
}

void ULMSCombatHUDPresenterComponent::UpdateInteractionProgress(float Progress) const
{
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetInteractionProgress(Progress);
}

void ULMSCombatHUDPresenterComponent::UpdateInteractionProgressVisible(bool bVisible) const
{
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetInteractionProgressVisible(bVisible);
}

void ULMSCombatHUDPresenterComponent::UpdateTeamMemberStatus(
	int32 MemberIndex,
	const FText& Nickname,
	float CurrentHealth,
	float MaxHealth,
	float CurrentShield,
	float MaxShield) const
{
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetTeamMemberStatus(
		MemberIndex,
		Nickname,
		CurrentHealth,
		MaxHealth,
		CurrentShield,
		MaxShield
	);
}

void ULMSCombatHUDPresenterComponent::UpdateTeamMemberVisible(
	int32 MemberIndex,
	bool bVisible) const
{
	if (!CombatHUDWidget)
	{
		return;
	}

	CombatHUDWidget->SetTeamMemberVisible(MemberIndex, bVisible);
}