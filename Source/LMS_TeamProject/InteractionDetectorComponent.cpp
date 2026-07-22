#include "InteractionDetectorComponent.h"

#include "GameFramework/Pawn.h"
#include "LMSInteractableInterface.h"
#include "TimerManager.h"

UInteractionDetectorComponent::UInteractionDetectorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;

    SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    SetCollisionObjectType(ECC_WorldDynamic);
    SetCollisionResponseToAllChannels(ECR_Ignore);
    SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);   // 임시. 나중에 Interactable 채널로.
    SetGenerateOverlapEvents(true);
}

void UInteractionDetectorComponent::BeginPlay()
{
    Super::BeginPlay();

    SetSphereRadius(DetectionRadius);

    // 조준 판정은 카메라/컨트롤러 기준이라 로컬 컨트롤 플레이어에서만
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        SetCollisionEnabled(ECollisionEnabled::NoCollision);
        SetGenerateOverlapEvents(false);
        return;
    }

    OnComponentBeginOverlap.AddDynamic(this, &UInteractionDetectorComponent::HandleBeginOverlap);
    OnComponentEndOverlap.AddDynamic(this, &UInteractionDetectorComponent::HandleEndOverlap);
}

void UInteractionDetectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void UInteractionDetectorComponent::HandleBeginOverlap(
    UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    if (!OtherActor || OtherActor == GetOwner())
    {
        return;
    }

    // 인터페이스 미구현이면 후보에도 안 넣음 (여기서 한 번만 체크)
    if (!OtherActor->Implements<ULMSInteractableInterface>())
    {
        return;
    }

    Candidates.AddUnique(OtherActor);

    // 0 → 1: 타이머 시작
    if (Candidates.Num() == 1)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                UpdateTimerHandle, this, &UInteractionDetectorComponent::UpdateTarget,
                UpdateInterval, true);
        }
    }
}

void UInteractionDetectorComponent::HandleEndOverlap(
    UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
    if (!OtherActor)
    {
        return;
    }

    Candidates.Remove(OtherActor);

    if (OtherActor == CurrentTarget)
    {
        SetTarget(nullptr);
    }

    // 0개: 타이머 정지 (평소 비용 0)
    if (Candidates.Num() == 0)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        }
    }
}

void UInteractionDetectorComponent::GetAimBasis(FVector& OutOrigin, FVector& OutForward) const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn)
    {
        OutOrigin = FVector::ZeroVector;
        OutForward = FVector::ForwardVector;
        return;
    }

    OutOrigin = OwnerPawn->GetActorLocation();

    // ControlRotation의 yaw만 → 캐릭터 회전 설정·1인칭/3인칭 무관
    const FRotator ControlRot = OwnerPawn->GetControlRotation();
    OutForward = FRotator(0.f, ControlRot.Yaw, 0.f).Vector();
}

bool UInteractionDetectorComponent::HasLineOfSight(
    const UWorld* World, const AActor* From, const AActor* To)
{
    if (!World || !From || !To)
    {
        return false;
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractionLOS), false, From);
    Params.AddIgnoredActor(To);

    FHitResult Hit;
    const bool bBlocked = World->LineTraceSingleByChannel(
        Hit,
        From->GetActorLocation(),
        To->GetActorLocation(),
        ECC_Visibility,
        Params);

    return !bBlocked;   // 사이에 막는 게 없으면 보임
}
void UInteractionDetectorComponent::UpdateTarget()
{
    // 소멸된 후보 정리
    Candidates.RemoveAll([](const AActor* A) { return !IsValid(A); });
    if (Candidates.Num() == 0)
    {
        SetTarget(nullptr);
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(UpdateTimerHandle);
        }
        return;
    }

    FVector Origin, Forward;
    GetAimBasis(Origin, Forward);

    AActor* Best = nullptr;
    float BestDot = AimDotThreshold;   // 임계값 미만은 애초에 탈락

    AActor* OwnerActor = GetOwner();

    for (AActor* Candidate : Candidates)
    {
        // 상호작용 가능한 상태인가 (매 주기 재확인 — 상태는 변함)
        if (!ILMSInteractableInterface::Execute_CanInteract(Candidate, OwnerActor))
        {
            continue;
        }

        // 수평 조준 (yaw만)
        const FVector ToTarget = (Candidate->GetActorLocation() - Origin).GetSafeNormal2D();
        if (ToTarget.IsNearlyZero())
        {
            continue;
        }

        const float Dot = FVector::DotProduct(Forward, ToTarget);
        if (Dot <= BestDot)
        {
            continue;
        }

        Best = Candidate;
        BestDot = Dot;
    }

    // 최종 후보 1명에게만 가시성 검사 (벽 뒤 제외)
    if (Best && !HasLineOfSight(GetWorld(), OwnerActor, Best))
    {
        Best = nullptr;
    }

    SetTarget(Best);
}

void UInteractionDetectorComponent::SetTarget(AActor* NewTarget)
{
    if (CurrentTarget == NewTarget)
    {
        return;   // 바뀔 때만 브로드캐스트
    }

    CurrentTarget = NewTarget;

    FGameplayTag Type;
    if (CurrentTarget)
    {
        Type = ILMSInteractableInterface::Execute_GetInteractionType(CurrentTarget);
    }

    OnTargetChanged.Broadcast(CurrentTarget, Type);
}