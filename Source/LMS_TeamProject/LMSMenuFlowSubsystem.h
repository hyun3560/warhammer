#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LMSMenuFlowSubsystem.generated.h"

UCLASS()
class LMS_TEAMPROJECT_API ULMSMenuFlowSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "LMS|Menu")
	void RequestGameplayIntro();

	bool ConsumeGameplayIntroRequest();
	bool ShouldShowStartupMenu(const UWorld* World) const;

	UFUNCTION(BlueprintPure, Category = "LMS|Menu")
	FName GetMenuCameraTag() const { return MenuCameraTag; }

	UFUNCTION(BlueprintPure, Category = "LMS|Menu")
	FName GetGameMapName() const { return GameMapName; }

	UFUNCTION(BlueprintPure, Category = "LMS|Menu")
	float GetIntroBlendTime() const { return IntroBlendTime; }

private:
	bool bPendingGameplayIntro = false;

	UPROPERTY(EditDefaultsOnly, Category = "LMS|Menu")
	FName MenuCameraTag = TEXT("MenuCamera");

	UPROPERTY(EditDefaultsOnly, Category = "LMS|Menu")
	FName GameMapName = TEXT("/Game/Resource/Scifi_desert_city/Level/L_showcase_level");

	UPROPERTY(EditDefaultsOnly, Category = "LMS|Menu")
	float IntroBlendTime = 2.5f;
};
