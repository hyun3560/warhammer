// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LMSCombatHUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class LMS_TEAMPROJECT_API ULMSCombatHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 현재 체력과 최대 체력을 HUD에 전달합니다.
	// 실제 ProgressBar/Text 갱신 로직은 WBP_LJH_CombatHUD 블루프린트에서 구현합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "LMS|Combat HUD")
	void SetHealth(float CurrentHealth, float MaxHealth);

	// 현재 쉴드와 최대 쉴드를 HUD에 전달합니다.
	// 실제 ProgressBar/Text 갱신 로직은 WBP_LJH_CombatHUD 블루프린트에서 구현합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "LMS|Combat HUD")
	void SetShield(float CurrentShield, float MaxShield);

	// 현재 스테미나와 최대 스테미나를 HUD에 전달합니다.
	// 실제 ProgressBar/Text 갱신 로직은 WBP_LJH_CombatHUD 블루프린트에서 구현합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "LMS|Combat HUD")
	void SetStamina(float CurrentStamina, float MaxStamina);

	// 플레이어 본인이 그로기 상태인지 HUD 블루프린트에 전달합니다.
	// 실제 HP UI 붉은 깜박임 연출은 WBP_CombatHUD 블루프린트에서 구현합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetGroggyState(bool bIsGroggy);

	// 현재 스킬 쿨타임을 확인 후 전달합니다.
	// 실제 ProgressBar/Text 갱신 로직은 WBP_LJH_CombatHUD 블루프린트에서 구현합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetSkillCooldown(float CurrentCooldown, float MaxCooldown);

	// 현재 탄약량을 확인 후 전달합니다.
	// 실제 ProgressBar/Text 갱신 로직은 WBP_LJH_CombatHUD 블루프린트에서 구현합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetAmmo(int32 CurrentAmmo, int32 ReserveAmmo);

	// SetInteractionPrompt는 표시 내용을 받는 문
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetInteractionPrompt(const FText& KeyText, const FText& InteractionText);

	// SetInteractionVisible은 보일지 숨길지 결정하는 문
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetInteractionVisible(bool bVisible);

	// 상호작용 진행률을 HUD ProgressBar에 반영하는 함수입니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetInteractionProgress(float Progress);

	// 상호작용 진행 바를 보일지 숨길지 결정하는 함수입니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetInteractionProgressVisible(bool bVisible);

	// 팀원 상태 UI에 표시할 닉네임, 체력, 쉴드 값을 HUD 블루프린트로 전달합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetTeamMemberStatus(
		int32 MemberIndex,
		const FText& Nickname,
		float CurrentHealth,
		float MaxHealth,
		float CurrentShield,
		float MaxShield
	);

	// 지정한 팀원 슬롯을 보이거나 숨기도록 HUD 블루프린트에 전달합니다.
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Combat HUD")
	void SetTeamMemberVisible(int32 MemberIndex, bool bVisible);

};
