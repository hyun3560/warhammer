#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IndicatorTypes.h"
#include "IndicatorEntryWidget.generated.h"

class UImage;

// 인디케이터 하나(팀원/적/목표/핑 아이콘 하나)를 표현하는 위젯입니다.
// 위치 갱신은 C++(UIndicatorOverlayWidget)에서 CanvasPanelSlot으로 처리하고,
// 타입별 색상/모양 변경은 블루프린트에서 OnIndicatorDataUpdated를 구현해서 처리합니다.
UCLASS()
class LMS_TEAMPROJECT_API UIndicatorEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 매 프레임 최신 데이터로 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void UpdateVisual(const FLMSIndicatorScreenData& NewData);

	UPROPERTY(BlueprintReadOnly, Category = "Indicator")
	FLMSIndicatorScreenData IndicatorData;

	// 디버그용: WBP의 IconImage와 이름이 같아야 BindWidgetOptional로 자동 연결됩니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

protected:
	// 타입에 따른 아이콘 색상/모양, 화살표 회전 등 실제 비주얼 처리는 블루프린트에서 구현합니다.
	UFUNCTION(BlueprintImplementableEvent, Category = "Indicator")
	void OnIndicatorDataUpdated();

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
