#include "LMSMainMenuWidget.h"

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

TSharedRef<SWidget> ULMSMainMenuWidget::RebuildWidget()
{
	BuildMenuTree();
	return Super::RebuildWidget();
}

void ULMSMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void ULMSMainMenuWidget::SetStatusMessage(const FText& Message)
{
	if (StatusText)
	{
		StatusText->SetText(Message);
	}
}

void ULMSMainMenuWidget::BuildMenuTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, UWidgetTree::StaticClass(), TEXT("WidgetTree"));
	}

	if (!WidgetTree)
	{
		return;
	}

	HostButton = nullptr;
	JoinButton = nullptr;
	QuitButton = nullptr;
	StatusText = nullptr;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.58f));
	Backdrop->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop);
	BackdropSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
	BackdropSlot->SetOffsets(FMargin(0.f));
	BackdropSlot->SetZOrder(0);

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	UCanvasPanelSlot* MenuSlot = RootCanvas->AddChildToCanvas(MenuBox);
	MenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
	MenuSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	MenuSlot->SetSize(FVector2D(420.f, 430.f));
	MenuSlot->SetPosition(FVector2D(0.f, 0.f));
	MenuSlot->SetZOrder(1);

	UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
	TitleText->SetText(FText::FromString(TEXT("LMS TEAM PROJECT")));
	TitleText->SetJustification(ETextJustify::Center);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.95f, 0.98f, 1.f)));
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 34;
	TitleText->SetFont(TitleFont);
	UVerticalBoxSlot* TitleSlot = MenuBox->AddChildToVerticalBox(TitleText);
	TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 28.f));
	TitleSlot->SetHorizontalAlignment(HAlign_Fill);

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetText(FText::FromString(TEXT("Host a LAN game or find one on this network.")));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetAutoWrapText(true);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.84f, 0.90f, 1.f)));
	FSlateFontInfo StatusFont = StatusText->GetFont();
	StatusFont.Size = 14;
	StatusText->SetFont(StatusFont);
	UVerticalBoxSlot* StatusSlot = MenuBox->AddChildToVerticalBox(StatusText);
	StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
	StatusSlot->SetHorizontalAlignment(HAlign_Fill);

	HostButton = BuildMenuButton(FText::FromString(TEXT("HOST GAME")));
	MenuBox->AddChildToVerticalBox(HostButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	JoinButton = BuildMenuButton(FText::FromString(TEXT("FIND GAME")));
	MenuBox->AddChildToVerticalBox(JoinButton)->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));

	QuitButton = BuildMenuButton(FText::FromString(TEXT("QUIT")));
	MenuBox->AddChildToVerticalBox(QuitButton)->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));

	if (HostButton)
	{
		HostButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleHostClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleJoinClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleQuitClicked);
	}
}

UButton* ULMSMainMenuWidget::BuildMenuButton(const FText& LabelText)
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

void ULMSMainMenuWidget::HandleHostClicked()
{
	if (ALMSPlayerController* LMSPlayerController = GetOwningPlayer<ALMSPlayerController>())
	{
		LMSPlayerController->HostGameFromMainMenu();
	}
}

void ULMSMainMenuWidget::HandleJoinClicked()
{
	if (ALMSPlayerController* LMSPlayerController = GetOwningPlayer<ALMSPlayerController>())
	{
		LMSPlayerController->FindGameFromMainMenu();
	}
}

void ULMSMainMenuWidget::HandleQuitClicked()
{
	if (ALMSPlayerController* LMSPlayerController = GetOwningPlayer<ALMSPlayerController>())
	{
		LMSPlayerController->QuitGameFromMainMenu();
	}
}
