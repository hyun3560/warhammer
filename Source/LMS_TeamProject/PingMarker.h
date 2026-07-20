#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/IndicatorTargetInterface.h"
#include "PingMarker.generated.h"

// 마우스 휠 클릭으로 찍는 핑 마커입니다. 서버가 스폰하면 액터 리플리케이션으로
// 모든 클라이언트에 자동 복제되고, 일정 시간 뒤 자동으로 파괴됩니다.
UCLASS()
class LMS_TEAMPROJECT_API APingMarker : public AActor, public IIndicatorTargetInterface
{
	GENERATED_BODY()

public:
	APingMarker();

	//~ Begin IIndicatorTargetInterface
	virtual ELMSIndicatorType GetIndicatorType_Implementation() const override;
	virtual bool ShouldShowIndicator_Implementation() const override;
	//~ End IIndicatorTargetInterface

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, Category = "Ping")
	TObjectPtr<USceneComponent> SceneRoot;

	// 스폰 후 이 시간(초)이 지나면 자동으로 파괴됩니다.
	UPROPERTY(EditDefaultsOnly, Category = "Ping")
	float LifeSpanSeconds = 5.f;
};
