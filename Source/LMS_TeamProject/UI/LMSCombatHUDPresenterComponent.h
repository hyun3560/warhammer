#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LMSCombatHUDPresenterComponent.generated.h"

class ULMSWeaponComponent;
class UAbilitySystemComponent;
class ULMSAttributeSet;
class ULMSCombatHUDWidget;
class UUIManagerComponent;
struct FOnAttributeChangeData;
struct FGameplayTag;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LMS_TEAMPROJECT_API ULMSCombatHUDPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULMSCombatHUDPresenterComponent();

	// 전투 HUD를 찾고, PlayerState의 Attribute 값을 HUD로 전달할 준비를 합니다.
	void InitializeCombatHUD();

	// =========================
	// Interaction UI Public API
	// =========================
	// 외부 시스템(캐릭터, Ability, 상호작용 액터)이 호출하는 상호작용 UI 명령입니다.
	// 외부 코드는 HUD 내부 위젯 구조나 표시 순서를 알 필요 없이,
	// "상호작용 안내를 보여줘/숨겨줘", "진행바를 보여줘/갱신해줘" 같은 의미 단위로만 호출합니다.

	// 상호작용 안내 UI를 표시합니다.
	// 내부적으로 문구를 갱신하고, 안내 UI를 보이게 만듭니다.
	void ShowInteractionPrompt(const FText& KeyText, const FText& InteractionText) const;

	// 상호작용 안내 UI를 숨깁니다.
	void HideInteractionPrompt() const;

	// 상호작용 진행바를 표시합니다.
	void ShowInteractionProgress() const;

	// 상호작용 진행률을 HUD에 전달합니다. Progress는 0~1 범위 값입니다.
	void SetInteractionProgressValue(float Progress) const;

	// 상호작용 진행바를 숨기고 진행률을 0으로 초기화합니다.
	void HideInteractionProgress() const;

	// =========================
	// Team Member UI Public API
	// =========================
	// 외부 팀/파티/PlayerState 시스템이 호출하는 팀원 상태 UI 명령입니다.
	// 외부 코드는 HUD 내부 위젯 구조를 알 필요 없이,
	// 지정한 팀원 슬롯을 보여주거나 숨기는 의미 단위로만 호출합니다.

	// 지정한 팀원 슬롯에 닉네임, 체력, 쉴드 값을 표시하고 슬롯을 보이게 만듭니다.
	// MemberIndex는 WBP_CombatHUD에 만들어둔 팀원 슬롯 번호입니다. 현재 0~2번 슬롯을 사용합니다.
	void ShowTeamMemberStatus(
		int32 MemberIndex,
		const FText& Nickname,
		float CurrentHealth,
		float MaxHealth,
		float CurrentShield,
		float MaxShield
	) const;

	// 지정한 팀원 슬롯을 숨깁니다.
	// 팀원이 없거나, 팀원 정보가 아직 준비되지 않았을 때 사용합니다.
	void HideTeamMemberStatus(int32 MemberIndex) const;

private:
	// Owner PlayerController가 가진 UIManagerComponent를 찾아 캐싱합니다.
	void CacheUIManagerComponent();

	// UIManagerComponent가 생성한 전투 HUD를 C++ 부모 클래스 타입으로 캐싱합니다.
	void CacheCombatHUDWidget();

	// PlayerState가 가진 AbilitySystemComponent와 AttributeSet을 캐싱합니다.
	void CacheAttributeSource();

	// 현재 조종 중인 캐릭터가 가진 무기 컴포넌트입니다.
	// 이 컴포넌트에서 현재 탄약/보유 탄약 값을 받아옵니다.
	UPROPERTY()
	TObjectPtr<ULMSWeaponComponent> CachedWeaponComponent;

	// 탄약 델리게이트가 중복 바인딩되는 것을 막는 플래그입니다.
	// InitializeCombatHUD가 여러 번 호출될 수 있으므로 필요합니다.
	bool bWeaponDelegatesBound = false;

	// GAS Attribute 값이 바뀔 때 HUD를 갱신하도록 델리게이트를 연결합니다.
	void BindAttributeDelegates();

	// 플레이어의 그로기 태그(state.Incapacitated) 변화 알림을 HUD 갱신 함수에 연결합니다.
	void BindStateTagDelegates();

	// 현재 그로기 태그 보유 여부를 기준으로 HUD 상태를 즉시 갱신합니다.
	void UpdateGroggyStateUI() const;

	// state.Incapacitated 태그가 붙거나 제거될 때 호출됩니다.
	void HandleIncapacitatedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);

	// 그로기 태그 델리게이트가 중복 바인딩되는 것을 막는 플래그입니다.
	bool bStateTagDelegatesBound = false;

	// 현재 Attribute 값을 기준으로 HUD 전체를 한 번 갱신합니다.
	void UpdateAllCombatHUD();

	// 체력/쉴드/스테미나 각각의 HUD 표시를 갱신합니다.
	void UpdateHealthUI();
	void UpdateShieldUI();
	void UpdateStaminaUI();

	// 스킬 쿨타임 HUD에 전달합니다.
	void UpdateSkillCooldownUI(float CurrentCooldown, float MaxCooldown) const;

	//탄약 보유량을 HUD에 전달합니다.
	void UpdateAmmoUI(int32 CurrentAmmo, int32 ReserveAmmo) const;

	// PlayerController가 조종 중인 Pawn에서 무기 컴포넌트를 찾아 캐싱합니다.
	// 탄약 UI는 PlayerState가 아니라 실제 캐릭터의 WeaponComponent 값을 사용합니다.
	void CacheWeaponSource();

	// WeaponComponent의 탄약 변경 델리게이트를 HUD 갱신 함수에 연결합니다.
	// 총 발사, 장전, 무기 장착으로 탄약 값이 바뀌면 HUD가 자동으로 갱신됩니다.
	void BindWeaponDelegates();

	// 무기 컴포넌트에서 탄약 변경 알림을 받았을 때 호출됩니다.
	// 받은 값을 그대로 HUD 블루프린트의 SetAmmo 이벤트로 전달합니다.
	UFUNCTION()
	void HandleAmmoChanged(int32 CurrentAmmo, int32 ReserveAmmo);

	// 무기 컴포넌트에서 스킬 쿨타임 변경 알림을 받았을 때 호출됩니다.
	// 받은 현재 쿨타임/전체 쿨타임 값을 HUD 블루프린트의 SetSkillCooldown 이벤트로 전달합니다.
	UFUNCTION()
	void HandleSkillCooldownChanged(float CurrentCooldown, float MaxCooldown);
	 
	// ==============================
	// Interaction UI Internal Updates
	// ==============================
	// Presenter 내부에서 WBP_CombatHUD의 BlueprintImplementableEvent를 호출하는 저수준 전달 함수입니다.
	// 외부 시스템은 가능하면 이 함수들을 직접 부르지 말고,
	// public Interaction UI API(Show/Hide/Set 계열)를 통해 의미 단위로 요청합니다.
	// 이렇게 나누면 나중에 UI 표시 절차가 바뀌어도 외부 코드를 고치지 않아도 됩니다.

	// 상호작용 안내 문구만 HUD 블루프린트에 전달합니다.
	void UpdateInteractionPrompt(const FText& KeyText, const FText& InteractionText) const;

	// 상호작용 안내 UI의 표시 여부만 HUD 블루프린트에 전달합니다.
	void UpdateInteractionVisible(bool bVisible) const;

	// 상호작용 진행률 값만 HUD 블루프린트에 전달합니다.
	void UpdateInteractionProgress(float Progress) const;

	// 상호작용 진행바의 표시 여부만 HUD 블루프린트에 전달합니다.
	void UpdateInteractionProgressVisible(bool bVisible) const;

	// 팀원 상태 데이터를 Combat HUD로 전달합니다.
	void UpdateTeamMemberStatus(
		int32 MemberIndex,
		const FText& Nickname,
		float CurrentHealth,
		float MaxHealth,
		float CurrentShield,
		float MaxShield
	) const;

	// 팀원 슬롯의 표시 여부를 Combat HUD로 전달합니다.
	void UpdateTeamMemberVisible(int32 MemberIndex, bool bVisible) const;
		

	// GAS Attribute 변경 알림을 받으면 해당 HUD 표시를 다시 갱신합니다.
	void HandleHealthChanged(const FOnAttributeChangeData& Data);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& Data);

	// 그로기 체력 값이 바뀌면 HP UI를 갱신합니다.
	// 그로기 상태일 때는 일반 Health 대신 IncapHealth를 HP 바에 표시합니다.
	void HandleIncapHealthChanged(const FOnAttributeChangeData& Data);

	// 그로기 최대 체력 값이 바뀌면 HP UI를 갱신합니다.
	void HandleMaxIncapHealthChanged(const FOnAttributeChangeData& Data);

	void HandleShieldChanged(const FOnAttributeChangeData& Data);
	void HandleMaxShieldChanged(const FOnAttributeChangeData& Data);
	void HandleStaminaChanged(const FOnAttributeChangeData& Data);
	void HandleMaxStaminaChanged(const FOnAttributeChangeData& Data);

	// PlayerController가 소유한 UI 관리자 컴포넌트입니다.
	UPROPERTY()
	TObjectPtr<UUIManagerComponent> UIManagerComponent;

	// UIManagerComponent가 생성한 실제 전투 HUD 위젯입니다.
	UPROPERTY()
	TObjectPtr<ULMSCombatHUDWidget> CombatHUDWidget;

	// PlayerState에 있는 GAS 컴포넌트와 AttributeSet입니다.
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<ULMSAttributeSet> CachedAttributeSet;

	// 델리게이트가 중복으로 연결되는 것을 막기 위한 플래그입니다.
	bool bAttributeDelegatesBound = false;
};
