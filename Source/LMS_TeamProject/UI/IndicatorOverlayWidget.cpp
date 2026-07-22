#include "IndicatorOverlayWidget.h"
#include "IndicatorEntryWidget.h"
#include "IndicatorManagerComponent.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UIndicatorOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

UIndicatorManagerComponent* UIndicatorOverlayWidget::FindIndicatorManager() const
{
	APlayerController* PC = GetOwningPlayer();
	return PC ? PC->FindComponentByClass<UIndicatorManagerComponent>() : nullptr;
}

void UIndicatorOverlayWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!RootCanvas || !IndicatorEntryClass)
	{
		return;
	}

	UIndicatorManagerComponent* IndicatorManager = FindIndicatorManager();
	if (!IndicatorManager)
	{
		return;
	}

	const TArray<FLMSIndicatorScreenData>& IndicatorDataList = IndicatorManager->GetIndicatorDataList();

	while (EntryPool.Num() < IndicatorDataList.Num())
	{
		UIndicatorEntryWidget* NewEntry = CreateWidget<UIndicatorEntryWidget>(GetOwningPlayer(), IndicatorEntryClass);
		if (!NewEntry)
		{
			break;
		}
		if (UCanvasPanelSlot* NewSlot = RootCanvas->AddChildToCanvas(NewEntry))
		{
			NewSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		}
		EntryPool.Add(NewEntry);
	}

	for (int32 Index = 0; Index < EntryPool.Num(); ++Index)
	{
		UIndicatorEntryWidget* Entry = EntryPool[Index];
		if (!Entry)
		{
			continue;
		}
		if (Index >= IndicatorDataList.Num())
		{
			Entry->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const FLMSIndicatorScreenData& Data = IndicatorDataList[Index];
		Entry->SetVisibility(ESlateVisibility::HitTestInvisible);
		Entry->UpdateVisual(Data);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Entry->Slot))
		{
			const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
			const FVector2D CorrectedPosition = (ViewportScale > KINDA_SMALL_NUMBER)
				? (Data.ScreenPosition / ViewportScale)
				: Data.ScreenPosition;
			CanvasSlot->SetPosition(CorrectedPosition);
		}
	}
}