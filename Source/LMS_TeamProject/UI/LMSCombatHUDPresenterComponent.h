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

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LMS_TEAMPROJECT_API ULMSCombatHUDPresenterComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULMSCombatHUDPresenterComponent();

	// 전투 HUD를 찾고, PlayerState의 Attribute 값을 HUD로 전달할 준비를 합니다.
	void InitializeCombatHUD();

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

	void UpdateInteractionPrompt(const FText& KeyText, const FText& InteractionText) const;
	void UpdateInteractionVisible(bool bVisible) const;

	// 상호작용 진행률을 HUD에 전달하는 함수입니다.
	void UpdateInteractionProgress(float Progress) const;

	// 진행 바의 숨김여부를 HUD에 전달하는 함수입니다.
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
