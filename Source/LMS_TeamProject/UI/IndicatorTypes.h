#pragma once

#include "CoreMinimal.h"
#include "IndicatorTypes.generated.h"

UENUM(BlueprintType)
enum class ELMSIndicatorType : uint8
{
	Ping		UMETA(DisplayName = "Ping"),
	Ally		UMETA(DisplayName = "Ally"),
	Enemy		UMETA(DisplayName = "Enemy"),
	Objective	UMETA(DisplayName = "Objective")
};

// 인디케이터 매니저가 매 프레임 계산한 결과를 위젯에 전달할 때 사용하는 데이터입니다.
USTRUCT(BlueprintType)
struct FLMSIndicatorScreenData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Indicator")
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Indicator")
	ELMSIndicatorType Type = ELMSIndicatorType::Ally;

	// 뷰포트 기준 스크린 좌표 (화면 밖이면 가장자리에 clamp된 좌표)
	UPROPERTY(BlueprintReadOnly, Category = "Indicator")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	// 화면 밖일 때 화살표를 회전시킬 각도 (degrees)
	UPROPERTY(BlueprintReadOnly, Category = "Indicator")
	float RotationAngleDegrees = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Indicator")
	bool bIsOffScreen = false;

	UPROPERTY(BlueprintReadOnly, Category = "Indicator")
	float DistanceToTarget = 0.f;
};
