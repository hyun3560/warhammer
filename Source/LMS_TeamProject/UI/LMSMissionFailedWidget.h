#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LMSMissionFailedWidget.generated.h"

class UButton;

UCLASS()
class LMS_TEAMPROJECT_API ULMSMissionFailedWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UFUNCTION()
	void HandleTitleClicked();

	UFUNCTION()
	void HandleQuitClicked();

	void BuildWidgetTree();
	UButton* BuildMenuButton(const FText& LabelText);

	UPROPERTY()
	TObjectPtr<UButton> TitleButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;
};
