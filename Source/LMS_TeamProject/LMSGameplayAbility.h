// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "LMSGameplayAbility.generated.h"


UENUM(BlueprintType)
enum class ELMSAbilityInputID : uint8
{
	None        UMETA(DisplayName = "None"),
	Confirm     UMETA(DisplayName = "Confirm"),
	Cancel      UMETA(DisplayName = "Cancel"),
	Sprint      UMETA(DisplayName = "Sprint"),
	Dash        UMETA(DisplayName = "Dash"),
	Interact    UMETA(DisplayName = "Interact"),
	WeaponSkill UMETA(DisplayName = "WeaponSkill"),
	PrimaryAttack UMETA(DisplayName = "FirePrimary"),   // ← 이렇게 추가
	SecondaryAttack UMETA(DisplayName = "FireSecondary"), // 필요하면 보조사격도
	Reload      UMETA(DisplayName = "Reload")
};

/**
 * Base gameplay ability class for LMS_TeamProject. Project-specific abilities should derive from this.
 */
UCLASS()
class LMS_TEAMPROJECT_API ULMSGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	ULMSGameplayAbility();

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	ELMSAbilityInputID AbilityInputID = ELMSAbilityInputID::None;
};
