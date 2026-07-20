#include "IndicatorEntryWidget.h"
#include "UObject/UnrealType.h"
#include "Components/Image.h"

void UIndicatorEntryWidget::UpdateVisual(const FLMSIndicatorScreenData& NewData)
{
	IndicatorData = NewData;

	static float DebugLogTimer = 0.f;
	DebugLogTimer += 0.016f;
	if (DebugLogTimer >= 0.5f)
	{
		DebugLogTimer = 0.f;
		const UEnum* EnumPtr = StaticEnum<ELMSIndicatorType>();
		UE_LOG(LogTemp, Warning, TEXT("[Indicator] UpdateVisual Type=%s Target=%s IconImageBound=%s"),
			*EnumPtr->GetNameStringByValue((int64)NewData.Type),
			*GetNameSafe(NewData.Target),
			IconImage ? TEXT("YES") : TEXT("NO"));
	}

	OnIndicatorDataUpdated();

	if (DebugLogTimer == 0.f && IconImage)
	{
		const FLinearColor CurrentColor = IconImage->GetColorAndOpacity();
		UE_LOG(LogTemp, Warning, TEXT("[Indicator] After OnIndicatorDataUpdated, IconImage Color=(%f,%f,%f,%f)"),
			CurrentColor.R, CurrentColor.G, CurrentColor.B, CurrentColor.A);
	}
}

void UIndicatorEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
}
