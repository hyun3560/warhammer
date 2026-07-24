#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LMSMainMenuWidget.generated.h"

class UButton;
class UTextBlock;

UCLASS()
class LMS_TEAMPROJECT_API ULMSMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetStatusMessage(const FText& Message);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	UFUNCTION()
	void HandleHostClicked();

	UFUNCTION()
	void HandleJoinClicked();

	UFUNCTION()
	void HandleQuitClicked();

	void BuildMenuTree();
	UButton* BuildMenuButton(const FText& LabelText);

	UPROPERTY()
	TObjectPtr<UButton> HostButton;

	UPROPERTY()
	TObjectPtr<UButton> JoinButton;

	UPROPERTY()
	TObjectPtr<UButton> QuitButton;

	UPROPERTY()
	TObjectPtr<UTextBlock> StatusText;
};
