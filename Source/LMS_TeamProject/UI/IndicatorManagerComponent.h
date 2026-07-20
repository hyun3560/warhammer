#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "IndicatorTypes.h"
#include "IndicatorManagerComponent.generated.h"

// 팀원/적/목표/핑 위치를 화면 안이면 실제 위치에, 화면 밖이면 가장자리에 clamp해서
// 표시하기 위한 좌표를 매 프레임 계산하는 컴포넌트입니다. PlayerController에 붙여서 사용합니다.
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LMS_TEAMPROJECT_API UIndicatorManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UIndicatorManagerComponent();

	// 인디케이터로 추적할 대상을 등록합니다. Target의 BeginPlay 등에서 호출하세요.
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void RegisterTarget(AActor* Target, ELMSIndicatorType Type);

	// 등록된 대상을 해제합니다. Target의 EndPlay 등에서 호출하세요.
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void UnregisterTarget(AActor* Target);

	// 이번 프레임에 계산된 인디케이터 표시 데이터를 반환합니다. 위젯에서 Tick마다 호출해서 사용합니다.
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	const TArray<FLMSIndicatorScreenData>& GetIndicatorDataList() const { return CachedIndicatorData; }

	// 화면 가장자리에서 얼마나 안쪽에 clamp할지 (픽셀)
	UPROPERTY(EditDefaultsOnly, Category = "Indicator")
	float ScreenEdgeMargin = 64.f;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	struct FTrackedIndicatorTarget
	{
		TWeakObjectPtr<AActor> Target;
		ELMSIndicatorType Type;
	};

	TArray<FTrackedIndicatorTarget> TrackedTargets;

	UPROPERTY()
	TArray<FLMSIndicatorScreenData> CachedIndicatorData;

	// 화면 중심에서 타겟 방향으로 뻗은 선과, 마진이 적용된 화면 사각형의 교차점을 계산합니다.
	static FVector2D ComputeEdgeClampedPosition(const FVector2D& ScreenCenter, const FVector2D& Direction,
		float HalfWidth, float HalfHeight);
};
