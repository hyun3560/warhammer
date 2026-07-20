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

	static float DebugLogTimer = 0.f;
	DebugLogTimer += InDeltaTime;
	const bool bShouldLog = DebugLogTimer >= 1.f;
	if (bShouldLog)
	{
		DebugLogTimer = 0.f;
	}

	if (!RootCanvas || !IndicatorEntryClass)
	{
		if (bShouldLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Indicator] NativeTick abort on %s: RootCanvas=%s, IndicatorEntryClass=%s"),
				*GetClass()->GetName(),
				RootCanvas ? TEXT("OK") : TEXT("NULL"), IndicatorEntryClass ? TEXT("OK") : TEXT("NULL"));
		}
		return;
	}

	UIndicatorManagerComponent* IndicatorManager = FindIndicatorManager();
	if (!IndicatorManager)
	{
		if (bShouldLog)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Indicator] NativeTick abort: IndicatorManager NOT FOUND (OwningPlayer=%s)"),
				GetOwningPlayer() ? *GetOwningPlayer()->GetName() : TEXT("NULL"));
		}
		return;
	}

	const TArray<FLMSIndicatorScreenData>& IndicatorDataList = IndicatorManager->GetIndicatorDataList();
	if (bShouldLog)
	{
		UE_LOG(LogTemp, Log, TEXT("[Indicator] IndicatorDataList.Num()=%d, EntryPool.Num()=%d"), IndicatorDataList.Num(), EntryPool.Num());
	}

	// 필요한 만큼 풀을 확장합니다 (이미 있는 엔트리는 재사용).
	while (EntryPool.Num() < IndicatorDataList.Num())
	{
		UIndicatorEntryWidget* NewEntry = CreateWidget<UIndicatorEntryWidget>(GetOwningPlayer(), IndicatorEntryClass);
		if (!NewEntry)
		{
			break;
		}

		if (UCanvasPanelSlot* NewSlot = RootCanvas->AddChildToCanvas(NewEntry))
		{
			// 아이콘 중심이 계산된 스크린 좌표에 오도록 정렬 기준점을 중앙으로 맞춥니다.
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
			// 이번 프레임엔 쓰이지 않는 엔트리는 숨겨서 재사용을 대기합니다.
			Entry->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		const FLMSIndicatorScreenData& Data = IndicatorDataList[Index];
		Entry->SetVisibility(ESlateVisibility::HitTestInvisible);
		Entry->UpdateVisual(Data);

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Entry->Slot))
		{
			// ProjectWorldLocationToScreen은 실제 픽셀 좌표를 반환하지만,
			// UMG CanvasPanelSlot은 DPI 스케일이 적용된 뷰포트 좌표를 기대하므로 나눠서 보정합니다.
			const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
			const FVector2D CorrectedPosition = (ViewportScale > KINDA_SMALL_NUMBER)
				? (Data.ScreenPosition / ViewportScale)
				: Data.ScreenPosition;
			CanvasSlot->SetPosition(CorrectedPosition);
		}
	}
}
