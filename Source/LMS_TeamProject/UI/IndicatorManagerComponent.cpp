#include "IndicatorManagerComponent.h"
#include "IndicatorTargetInterface.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UIndicatorManagerComponent::UIndicatorManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.f; // 매 프레임 계산해야 부드럽게 움직임
	// 카메라 회전이 실제로 반영된 "이후"에 화면 좌표를 계산해야 한 프레임 지연(떨림)이 없습니다.
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UIndicatorManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UIndicatorManagerComponent::RegisterTarget(AActor* Target, ELMSIndicatorType Type)
{
	if (!Target)
	{
		return;
	}

	for (const FTrackedIndicatorTarget& Existing : TrackedTargets)
	{
		if (Existing.Target.Get() == Target)
		{
			return;
		}
	}

	TrackedTargets.Add(FTrackedIndicatorTarget{ Target, Type });
}

void UIndicatorManagerComponent::UnregisterTarget(AActor* Target)
{
	TrackedTargets.RemoveAll([Target](const FTrackedIndicatorTarget& Tracked)
	{
		return !Tracked.Target.IsValid() || Tracked.Target.Get() == Target;
	});
}

void UIndicatorManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CachedIndicatorData.Reset();

	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PC->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return;
	}

	const FVector2D ScreenCenter(ViewportSizeX * 0.5f, ViewportSizeY * 0.5f);
	const float HalfWidth = FMath::Max(ScreenCenter.X - ScreenEdgeMargin, 1.f);
	const float HalfHeight = FMath::Max(ScreenCenter.Y - ScreenEdgeMargin, 1.f);

	FVector CameraLocation;
	FRotator CameraRotation;
	PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
	const FVector CameraForward = CameraRotation.Vector();

	TrackedTargets.RemoveAll([](const FTrackedIndicatorTarget& Tracked)
	{
		return !Tracked.Target.IsValid();
	});

	for (const FTrackedIndicatorTarget& Tracked : TrackedTargets)
	{
		AActor* Target = Tracked.Target.Get();
		if (!Target)
		{
			continue;
		}

		if (Target->Implements<UIndicatorTargetInterface>())
		{
			const bool bShouldShow = IIndicatorTargetInterface::Execute_ShouldShowIndicator(Target);
			if (!bShouldShow)
			{
				continue;
			}
		}

		const FVector TargetLocation = Target->GetActorLocation();

		FVector2D ScreenPosition;
		const bool bProjected = PC->ProjectWorldLocationToScreen(TargetLocation, ScreenPosition, true);

		const FVector ToTarget = (TargetLocation - CameraLocation).GetSafeNormal();
		const bool bBehindCamera = FVector::DotProduct(CameraForward, ToTarget) < 0.f;

		FLMSIndicatorScreenData Data;
		Data.Target = Target;
		Data.Type = Tracked.Type;
		Data.DistanceToTarget = FVector::Dist(CameraLocation, TargetLocation);

		const bool bWithinViewportBounds = bProjected
			&& ScreenPosition.X >= 0.f && ScreenPosition.X <= ViewportSizeX
			&& ScreenPosition.Y >= 0.f && ScreenPosition.Y <= ViewportSizeY;

		if (bWithinViewportBounds && !bBehindCamera)
		{
			Data.bIsOffScreen = false;
			Data.ScreenPosition = ScreenPosition;
			Data.RotationAngleDegrees = 0.f;
		}
		else
		{
			FVector2D Direction = ScreenPosition - ScreenCenter;
			if (bBehindCamera)
			{
				// 카메라 뒤에 있으면 투영 좌표가 반전되어 나오므로 방향을 뒤집어 보정합니다.
				Direction *= -1.f;
			}

			if (Direction.IsNearlyZero())
			{
				Direction = FVector2D(0.f, -1.f);
			}

			Data.bIsOffScreen = true;
			Data.ScreenPosition = ComputeEdgeClampedPosition(ScreenCenter, Direction, HalfWidth, HalfHeight);
			Data.RotationAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(Direction.Y, Direction.X)) + 90.f;
		}

		CachedIndicatorData.Add(Data);
	}
}

FVector2D UIndicatorManagerComponent::ComputeEdgeClampedPosition(const FVector2D& ScreenCenter, const FVector2D& Direction,
	float HalfWidth, float HalfHeight)
{
	// 화면 중심에서 Direction 방향으로 뻗은 선이 마진 적용된 사각형의 어느 변과 먼저 만나는지 계산합니다.
	const float ScaleX = (FMath::Abs(Direction.X) > KINDA_SMALL_NUMBER) ? (HalfWidth / FMath::Abs(Direction.X)) : TNumericLimits<float>::Max();
	const float ScaleY = (FMath::Abs(Direction.Y) > KINDA_SMALL_NUMBER) ? (HalfHeight / FMath::Abs(Direction.Y)) : TNumericLimits<float>::Max();
	const float Scale = FMath::Min(ScaleX, ScaleY);

	return ScreenCenter + Direction * Scale;
}
