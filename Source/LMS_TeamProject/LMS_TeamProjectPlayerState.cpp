// Fill out your copyright notice in the Description page of Project Settings.

#include "LMS_TeamProjectPlayerState.h"
#include "AbilitySystemComponent.h"
#include "LMSAttributeSet.h"
#include "Net/UnrealNetwork.h"

ALMS_TeamProjectPlayerState::ALMS_TeamProjectPlayerState()
{
	// 1. ASC 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// 2. 복제 모드 — 플레이어는 Mixed
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 3. AttributeSet 생성 (ASC가 자동으로 인식)
	AttributeSet = CreateDefaultSubobject<ULMSAttributeSet>(TEXT("AttributeSet"));

	// 4. PlayerState는 기본 NetUpdateFrequency가 낮음 → 올려줌
	NetUpdateFrequency = 100.f;
}

UAbilitySystemComponent* ALMS_TeamProjectPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALMS_TeamProjectPlayerState::SetSelectedWeaponID(FName NewWeaponID)
{
	SelectedWeaponID = NewWeaponID;
	bHasSelectedWeapon = !NewWeaponID.IsNone();
}

void ALMS_TeamProjectPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALMS_TeamProjectPlayerState, SelectedWeaponID);
	DOREPLIFETIME(ALMS_TeamProjectPlayerState, bHasSelectedWeapon);
}