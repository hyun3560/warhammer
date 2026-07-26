// Copyright Epic Games, Inc. All Rights Reserved.

#include "LMS_TeamProjectGameMode.h"

#include "EngineUtils.h"
#include "LMSPlayerController.h"
#include "LMS_TeamProjectCharacter.h"
#include "LMS_TeamProjectPlayerState.h"

ALMS_TeamProjectGameMode::ALMS_TeamProjectGameMode()
{
	PlayerStateClass = ALMS_TeamProjectPlayerState::StaticClass();
}

void ALMS_TeamProjectGameMode::NotifyPlayerCharacterDied()
{
	CheckMissionFailed();
}

void ALMS_TeamProjectGameMode::CheckMissionFailed()
{
	if (bMissionFailed)
	{
		return;
	}

	bool bFoundPlayerCharacter = false;

	for (TActorIterator<ALMS_TeamProjectCharacter> It(GetWorld()); It; ++It)
	{
		const ALMS_TeamProjectCharacter* PlayerCharacter = *It;
		if (!PlayerCharacter)
		{
			continue;
		}

		bFoundPlayerCharacter = true;

		if (!PlayerCharacter->IsDead())
		{
			return;
		}
	}

	if (!bFoundPlayerCharacter)
	{
		return;
	}

	bMissionFailed = true;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (ALMSPlayerController* LMSPlayerController = Cast<ALMSPlayerController>(It->Get()))
		{
			LMSPlayerController->ClientShowMissionFailedScreen();
		}
	}
}
