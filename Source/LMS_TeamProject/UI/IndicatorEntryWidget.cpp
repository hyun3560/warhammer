#include "IndicatorEntryWidget.h"
#include "UObject/UnrealType.h"
#include "Components/Image.h"

void UIndicatorEntryWidget::UpdateVisual(const FLMSIndicatorScreenData& NewData)
{
	IndicatorData = NewData;
	OnIndicatorDataUpdated();
}

void UIndicatorEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}
