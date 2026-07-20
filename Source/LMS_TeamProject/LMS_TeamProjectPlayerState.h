// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "LMS_TeamProjectPlayerState.generated.h"

class UAbilitySystemComponent;
class ULMSAttributeSet;

UCLASS()
class LMS_TEAMPROJECT_API ALMS_TeamProjectPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ALMS_TeamProjectPlayerState();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	ULMSAttributeSet* GetAttributeSet() const { return AttributeSet; }

	// Replicated로 선언한 변수들이 네트워크를 통해 클라이언트에 전달되도록 등록하는 함수입니다.
	// SelectedWeaponID와 bHasSelectedWeapon을 서버 기준 상태로 동기화할 때 사용합니다.
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 서버에서 플레이어가 선택한 무기 ID를 저장합니다.
	// Weapon DataTable의 RowName과 같은 값을 사용합니다. 예: "Rifle", "Hammer"
	void SetSelectedWeaponID(FName NewWeaponID);

	// 현재 플레이어가 선택한 무기 ID를 반환합니다.
	FName GetSelectedWeaponID() const { return SelectedWeaponID; }

	// 플레이어가 무기 선택을 완료했는지 반환합니다.
	bool HasSelectedWeapon() const { return bHasSelectedWeapon; }

protected:
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<ULMSAttributeSet> AttributeSet;

	// 플레이어가 선택한 무기 ID입니다.
	// 서버에서 설정하고 클라이언트로 복제됩니다.
	UPROPERTY(Replicated)
	FName SelectedWeaponID = NAME_None;

	// 플레이어가 무기를 한 번이라도 선택 완료했는지 여부입니다.
	// 나중에 전원 선택 완료 체크에 사용할 수 있습니다.
	UPROPERTY(Replicated)
	bool bHasSelectedWeapon = false;
};