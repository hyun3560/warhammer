#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IndicatorOverlayWidget.generated.h"

class UCanvasPanel;
class UIndicatorEntryWidget;
class UIndicatorManagerComponent;

// 화면 전체를 덮는 인디케이터 오버레이입니다. 매 프레임 IndicatorManagerComponent가 계산한
// 데이터를 읽어서, UIndicatorEntryWidget 인스턴스를 풀링 방식으로 재사용하며 화면에 배치합니다.
UCLASS()
class LMS_TEAMPROJECT_API UIndicatorOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 위젯 블루프린트(WBP_IndicatorOverlay)에서 BindWidget으로 연결할 루트 캔버스입니다.
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvas;

	// 자식 엔트리로 생성할 위젯 클래스입니다. (WBP_IndicatorEntry 지정)
	UPROPERTY(EditDefaultsOnly, Category = "Indicator")
	TSubclassOf<UIndicatorEntryWidget> IndicatorEntryClass;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY()
	TArray<TObjectPtr<UIndicatorEntryWidget>> EntryPool;

	UIndicatorManagerComponent* FindIndicatorManager() const;
};
