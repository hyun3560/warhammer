#include "PingMarker.h"
#include "UI/IndicatorManagerComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

APingMarker::APingMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void APingMarker::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpanSeconds);

	// 각 클라이언트/서버는 자신의 로컬 플레이어 컨트롤러에만 인디케이터로 등록합니다.
	if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (UIndicatorManagerComponent* IndicatorManager = LocalPC->FindComponentByClass<UIndicatorManagerComponent>())
		{
			IndicatorManager->RegisterTarget(this, ELMSIndicatorType::Ping);
		}
	}
}

void APingMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* LocalPC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		if (UIndicatorManagerComponent* IndicatorManager = LocalPC->FindComponentByClass<UIndicatorManagerComponent>())
		{
			IndicatorManager->UnregisterTarget(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

ELMSIndicatorType APingMarker::GetIndicatorType_Implementation() const
{
	return ELMSIndicatorType::Ping;
}

bool APingMarker::ShouldShowIndicator_Implementation() const
{
	return true;
}
