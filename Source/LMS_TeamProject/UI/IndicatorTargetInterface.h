#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IndicatorTypes.h"
#include "IndicatorTargetInterface.generated.h"

UINTERFACE(BlueprintType)
class LMS_TEAMPROJECT_API UIndicatorTargetInterface : public UInterface
{
	GENERATED_BODY()
};

// 오프스크린/엣지 인디케이터로 표시되고 싶은 액터가 구현하는 최소 인터페이스입니다.
class LMS_TEAMPROJECT_API IIndicatorTargetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Indicator")
	ELMSIndicatorType GetIndicatorType() const;

	// 죽은 캐릭터, 아직 준비되지 않은 액터 등 표시하면 안 되는 조건을 여기서 걸러냅니다.
	UFUNCTION(BlueprintNativeEvent, Category = "Indicator")
	bool ShouldShowIndicator() const;
};
