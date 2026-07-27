#include "BombObjectiveManager.h"
#include "TimerManager.h"

ABombObjectiveManager::ABombObjectiveManager()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ABombObjectiveManager::BeginPlay()
{
	Super::BeginPlay();

	CurrentState = EBombObjectiveState::Idle;
	RemainingTime = 0;
}

void ABombObjectiveManager::StartBombObjective()
{
	// 중복 실행 방지
	if (CurrentState != EBombObjectiveState::Idle)
	{
		return;
	}

	CurrentState = EBombObjectiveState::ReturnCountdown;
	RemainingTime = ReturnTimeLimit;

	OnReturnCountdownChanged(RemainingTime);

	GetWorldTimerManager().SetTimer(
		ReturnCountdownTimerHandle,
		this,
		&ABombObjectiveManager::TickReturnCountdown,
		1.0f,
		true
	);
}

void ABombObjectiveManager::TickReturnCountdown()
{
	if (CurrentState != EBombObjectiveState::ReturnCountdown)
	{
		GetWorldTimerManager().ClearTimer(ReturnCountdownTimerHandle);
		return;
	}

	RemainingTime--;

	if (RemainingTime > 0)
	{
		OnReturnCountdownChanged(RemainingTime);
		return;
	}

	RemainingTime = 0;
	OnReturnCountdownChanged(RemainingTime);

	HandleFailure();
}

void ABombObjectiveManager::NotifyPlayerReturnedToStart()
{
	if (CurrentState != EBombObjectiveState::ReturnCountdown)
	{
		return;
	}

	HandleSuccess();
}

void ABombObjectiveManager::HandleSuccess()
{
	if (CurrentState != EBombObjectiveState::ReturnCountdown)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReturnCountdownTimerHandle);

	OnCountdownHidden();

	StartExplosionSequence(true);
}

void ABombObjectiveManager::HandleFailure()
{
	if (CurrentState != EBombObjectiveState::ReturnCountdown)
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ReturnCountdownTimerHandle);

	OnCountdownHidden();

	StartExplosionSequence(false);
}

void ABombObjectiveManager::StartExplosionSequence(bool bSuccess)
{
	CurrentState = EBombObjectiveState::ExplosionSequence;
	bExplosionWasSuccess = bSuccess;

	OnPlayExplosionSequence(bSuccess);
}

void ABombObjectiveManager::FinishExplosionSequence()
{
	if (CurrentState != EBombObjectiveState::ExplosionSequence)
	{
		return;
	}

	if (bExplosionWasSuccess)
	{
		CurrentState = EBombObjectiveState::Completed;
		OnShowResultScreen(true);
	}
	else
	{
		CurrentState = EBombObjectiveState::Failed;
		OnShowResultScreen(false);
	}
}