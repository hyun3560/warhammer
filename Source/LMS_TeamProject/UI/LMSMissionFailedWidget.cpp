#include "LMSMissionFailedWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "../LMSPlayerController.h"

TSharedRef<SWidget> ULMSMissionFailedWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void ULMSMissionFailedWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, UWidgetTree::StaticClass(), TEXT("WidgetTree"));
	}

	if (!WidgetTree)
	{
		return;
	}

	TitleButton = nullptr;
	QuitButton = nullptr;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.01f, 0.012f, 0.014f, 0.82f));
	Backdrop->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));
	BackdropSlot->SetZOrder(0);

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MissionFailedBox"));
	UCanvasPanelSlot* MenuSlot = RootCanvas->AddChildToCanvas(MenuBox);
	MenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	MenuSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	MenuSlot->SetSize(FVector2D(460.f, 300.f));
	MenuSlot->SetPosition(FVector2D::ZeroVector);
	MenuSlot->SetZOrder(1);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("MISSION FAILED")));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.12f, 0.10f, 1.f)));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 42;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	TitleSlot->SetHorizontalAlignment(HAlign_Fill);

	UTextBlock* MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
	MessageText->SetText(FText::FromString(TEXT("All players have fallen.")));
	MessageText->SetJustification(ETextJustify::Center);
	MessageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.90f, 1.f)));
	FSlateFontInfo MessageFont = MessageText->GetFont();
	MessageFont.Size = 16;
	MessageText->SetFont(MessageFont);
	UVerticalBoxSlot* MessageSlot = MenuBox->AddChildToVerticalBox(MessageText);
	MessageSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 26.f));
	MessageSlot->SetHorizontalAlignment(HAlign_Fill);

	TitleButton = BuildMenuButton(FText::FromString(TEXT("RETURN TO TITLE")));
	MenuBox->AddChildToVerticalBox(TitleButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	QuitButton = BuildMenuButton(FText::FromString(TEXT("QUIT GAME")));
	MenuBox->AddChildToVerticalBox(QuitButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 0.f));

	if (TitleButton)
	{
		TitleButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleTitleClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
	}
}

UButton* ULMSMissionFailedWidget::BuildMenuButton(const FText& LabelText)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SizeBox->SetHeightOverride(52.f);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(LabelText);
	Label->SetJustification(ETextJustify::Center);
	Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.04f, 0.045f, 0.05f, 1.f)));
	FSlateFontInfo LabelFont = Label->GetFont();
	LabelFont.Size = 18;
	Label->SetFont(LabelFont);

	SizeBox->AddChild(Label);
	Button->AddChild(SizeBox);
	return Button;
}

void ULMSMissionFailedWidget::HandleTitleClicked()
{
	if (ALMSPlayerController* LMSPlayerController = GetOwningPlayer<ALMSPlayerController>())
	{
		LMSPlayerController->ReturnToTitleFromMissionFailed();
	}
}

void ULMSMissionFailedWidget::HandleQuitClicked()
{
	if (ALMSPlayerController* LMSPlayerController = GetOwningPlayer<ALMSPlayerController>())
	{
		LMSPlayerController->QuitGameFromMissionFailed();
	}
}
