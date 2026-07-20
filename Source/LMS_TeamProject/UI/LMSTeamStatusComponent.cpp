#include "LMSTeamStatusComponent.h"

#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "../LMSAttributeSet.h"
#include "../LMS_TeamProjectPlayerState.h"
#include "TimerManager.h"
#include "LMSCombatHUDPresenterComponent.h"

ULMSTeamStatusComponent::ULMSTeamStatusComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void ULMSTeamStatusComponent::InitializeTeamStatus()
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	CacheCombatHUDPresenter();
	RefreshTeamStatus();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			RefreshTimerHandle,
			this,
			&ULMSTeamStatusComponent::RefreshTeamStatus,
			RefreshInterval,
			true
		);
	}
}

void ULMSTeamStatusComponent::CacheCombatHUDPresenter()
{
	if (CombatHUDPresenterComponent)
	{
		return;
	}

	if (AActor* Owner = GetOwner())
	{
		CombatHUDPresenterComponent = Owner->FindComponentByClass<ULMSCombatHUDPresenterComponent>();
	}
}

void ULMSTeamStatusComponent::RefreshTeamStatus()
{
	CacheCombatHUDPresenter();

	if (!CombatHUDPresenterComponent)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetOwner());
	if (!PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	const APlayerState* LocalPlayerState = PlayerController->PlayerState;
	const AGameStateBase* GameState = GetWorld() ? GetWorld()->GetGameState<AGameStateBase>() : nullptr;
	if (!LocalPlayerState || !GameState)
	{
		for (int32 SlotIndex = 0; SlotIndex < MaxTeamMemberSlots; ++SlotIndex)
		{
			CombatHUDPresenterComponent->HideTeamMemberStatus(SlotIndex);
		}
		return;
	}

	int32 SlotIndex = 0;

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!PlayerState || PlayerState == LocalPlayerState)
		{
			continue;
		}

		const ALMS_TeamProjectPlayerState* TeamPlayerState = Cast<ALMS_TeamProjectPlayerState>(PlayerState);
		if (!TeamPlayerState)
		{
			continue;
		}

		const ULMSAttributeSet* TeamAttributeSet = TeamPlayerState->GetAttributeSet();
		if (!TeamAttributeSet)
		{
			continue;
		}

		CombatHUDPresenterComponent->ShowTeamMemberStatus(
			SlotIndex,
			FText::Format(
				FText::FromString(TEXT("Player {0}")),
				FText::AsNumber(SlotIndex + 1)
			),
			TeamAttributeSet->GetHealth(),
			TeamAttributeSet->GetMaxHealth(),
			TeamAttributeSet->GetShield(),
			TeamAttributeSet->GetMaxShield()
		);
		++SlotIndex;
		if (SlotIndex >= MaxTeamMemberSlots)
		{
			break;
		}
	}

	for (; SlotIndex < MaxTeamMemberSlots; ++SlotIndex)
	{
		CombatHUDPresenterComponent->HideTeamMemberStatus(SlotIndex);
	}
}
