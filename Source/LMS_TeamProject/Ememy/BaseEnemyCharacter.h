// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "EnemyTableRow.h"
#include "../UI/IndicatorTargetInterface.h"
#include "BaseEnemyCharacter.generated.h"

class UAbilitySystemComponent;
class ULMSAttributeSet;
class UGameplayAbility;

UCLASS()
class LMS_TEAMPROJECT_API ABaseEnemyCharacter : public ACharacter, public IAbilitySystemInterface, public IIndicatorTargetInterface
{
	GENERATED_BODY()

public:
	ABaseEnemyCharacter();

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	//~ Begin IIndicatorTargetInterface
	virtual ELMSIndicatorType GetIndicatorType_Implementation() const override;
	virtual bool ShouldShowIndicator_Implementation() const override;
	//~ End IIndicatorTargetInterface

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PossessedBy(AController* NewController) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stat")
	int UniqueID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Montage")
	TObjectPtr<class UAnimMontage> AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	const FEnemyTableRow* GetEnemyData() const;

	// 사망 후 액터를 제거하기까지의 지연 시간(초). 사망 애니메이션 재생 시간 확보용.
	UPROPERTY(EditDefaultsOnly, Category = "Stat")
	float DeathDestroyDelay = 2.f;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<ULMSAttributeSet> AttributeSet;

	FTimerHandle DeathDestroyTimerHandle;

	void GiveDefaultAbilities();

	// AttributeSet->OnHealthZero(서버 전용)에 바인딩되어 사망 처리를 수행합니다.
	void HandleHealthZero(const FGameplayEffectModCallbackData& Data);

	void HandleDeathDestroy();
};
