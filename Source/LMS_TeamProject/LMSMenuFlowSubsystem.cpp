#include "LMSMenuFlowSubsystem.h"

#include "Engine/World.h"

void ULMSMenuFlowSubsystem::RequestGameplayIntro()
{
	bPendingGameplayIntro = true;
}

bool ULMSMenuFlowSubsystem::ConsumeGameplayIntroRequest()
{
	const bool bShouldRunIntro = bPendingGameplayIntro;
	bPendingGameplayIntro = false;
	return bShouldRunIntro;
}

bool ULMSMenuFlowSubsystem::ShouldShowStartupMenu(const UWorld* World) const
{
	return World && World->GetNetMode() == NM_Standalone && !bPendingGameplayIntro;
}
