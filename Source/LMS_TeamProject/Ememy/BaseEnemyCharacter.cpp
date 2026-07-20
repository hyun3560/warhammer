// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseEnemyCharacter.h"
#include "AbilitySystemComponent.h"
#include "../LMSAttributeSet.h"
#include "Engine/GameInstance.h"
#include "DataTableSubSystem.h"
#include "GameplayTagContainer.h"
#include "../UI/IndicatorManagerComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AIController.h"
#include "BrainComponent.h"

ABaseEnemyCharacter::ABaseEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<ULMSAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ABaseEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABaseEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		// 인디케이터 시스템이 적(빨강)을 구분할 때 사용하는 팀 태그입니다.
		static const FGameplayTag TeamEnemyTag = FGameplayTag::RequestGameplayTag(FName("Team.Enemy"));
		if (HasAuthority() && !AbilitySystemComponent->HasMatchingGameplayTag(TeamEnemyTag))
		{
			AbilitySystemComponent->AddReplicatedLooseGameplayTag(TeamEnemyTag);
		}
	}

	if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (UIndicatorManagerComponent* IndicatorManager = LocalPC->FindComponentByClass<UIndicatorManagerComponent>())
		{
			IndicatorManager->RegisterTarget(this, ELMSIndicatorType::Enemy);
		}
	}
}

void ABaseEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (UIndicatorManagerComponent* IndicatorManager = LocalPC->FindComponentByClass<UIndicatorManagerComponent>())
		{
			IndicatorManager->UnregisterTarget(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ABaseEnemyCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		GiveDefaultAbilities();

		// OnHealthZero는 PostGameplayEffectExecute(서버)에서만 브로드캐스트됩니다.
		if (HasAuthority() && AttributeSet)
		{
			const FEnemyTableRow* Data = GetEnemyData();
			if (Data)
			{
				AttributeSet->SetMaxHealth(Data->MaxHP);
				AttributeSet->SetHealth(Data->MaxHP);
			}

			AttributeSet->OnHealthZero.RemoveAll(this);
			AttributeSet->OnHealthZero.AddUObject(this, &ABaseEnemyCharacter::HandleHealthZero);
		}
	}
}

ELMSIndicatorType ABaseEnemyCharacter::GetIndicatorType_Implementation() const
{
	return ELMSIndicatorType::Enemy;
}

bool ABaseEnemyCharacter::ShouldShowIndicator_Implementation() const
{
	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("state.Dead"));
	return !AbilitySystemComponent || !AbilitySystemComponent->HasMatchingGameplayTag(DeadTag);
}

void ABaseEnemyCharacter::GiveDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	for (TSubclassOf<UGameplayAbility>& Ability : DefaultAbilities)
	{
		if (Ability)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability, 1));
		}
	}
}

void ABaseEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABaseEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

float ABaseEnemyCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || !AbilitySystemComponent || ActualDamage <= 0.f)
	{
		return ActualDamage;
	}

	AbilitySystemComponent->ApplyModToAttribute(
		ULMSAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		-ActualDamage);

	UE_LOG(LogTemp, Log, TEXT("%s took %.1f damage (ApplyDamage), remaining Health = %.1f"),
		*GetName(), ActualDamage, AttributeSet ? AttributeSet->GetHealth() : 0.f);

	// ApplyModToAttribute는 PostGameplayEffectExecute를 거치지 않아 OnHealthZero가 자동으로 안 불림 -> 여기서 직접 체크.
	if (AttributeSet && AttributeSet->GetHealth() <= 0.f)
	{
		HandleDeath();
	}

	return ActualDamage;
}

const FEnemyTableRow* ABaseEnemyCharacter::GetEnemyData() const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UDataTableSubSystem* DataSubsystem = GameInstance ? GameInstance->GetSubsystem<UDataTableSubSystem>() : nullptr;
	return DataSubsystem ? DataSubsystem->GetEnemyData(UniqueID) : nullptr;
}

void ABaseEnemyCharacter::HandleHealthZero(const FGameplayEffectModCallbackData& Data)
{
	HandleDeath();
}

void ABaseEnemyCharacter::HandleDeath()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName("state.Dead"));
	if (AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))
	{
		return;
	}
	AbilitySystemComponent->AddReplicatedLooseGameplayTag(DeadTag);

	SetActorEnableCollision(false);
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}

	if (AAIController* AICon = Cast<AAIController>(GetController()))
	{
		if (UBrainComponent* Brain = AICon->GetBrainComponent())
		{
			Brain->StopLogic(TEXT("Dead"));
		}
	}

	HandleDeathDestroy();

	//GetWorldTimerManager().SetTimer(DeathDestroyTimerHandle, this, &ABaseEnemyCharacter::HandleDeathDestroy, DeathDestroyDelay, false);
}

void ABaseEnemyCharacter::HandleDeathDestroy()
{
	Destroy();
}


