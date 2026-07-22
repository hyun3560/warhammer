#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "InteractionDetectorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FOnInteractTargetChanged, AActor*, NewTarget, FGameplayTag, InteractionType);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class LMS_TEAMPROJECT_API UInteractionDetectorComponent : public USphereComponent
{
    GENERATED_BODY()

public:
    UInteractionDetectorComponent();

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    AActor* GetCurrentTarget() const { return CurrentTarget; }

    UPROPERTY(BlueprintAssignable, Category = "Interaction")
    FOnInteractTargetChanged OnTargetChanged;

    static bool HasLineOfSight(const UWorld* World, const AActor* From, const AActor* To);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void HandleBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void HandleEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
        UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

    void UpdateTarget();
    void GetAimBasis(FVector& OutOrigin, FVector& OutForward) const;
    void SetTarget(AActor* NewTarget);

    UPROPERTY(EditDefaultsOnly, Category = "Interaction")
    float DetectionRadius = 250.f;

    UPROPERTY(EditDefaultsOnly, Category = "Interaction")
    float AimDotThreshold = 0.7f;

    UPROPERTY(EditDefaultsOnly, Category = "Interaction")
    float UpdateInterval = 0.1f;

    UPROPERTY(Transient)
    TArray<AActor*> Candidates;

    UPROPERTY(Transient)
    AActor* CurrentTarget = nullptr;

    FTimerHandle UpdateTimerHandle;
};