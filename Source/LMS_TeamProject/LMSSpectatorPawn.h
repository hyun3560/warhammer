#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h" 
#include "LMSSpectatorPawn.generated.h"


class USpringArmComponent;
class UCameraComponent;

UCLASS()
class LMS_TEAMPROJECT_API ALMSSpectatorPawn : public APawn
{
    GENERATED_BODY()

public:
    ALMSSpectatorPawn();

    // 서버에서 호출: 관전 대상 지정
    void SetSpectateTarget(AActor* NewTarget);



protected:
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> SpringArm;

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    UPROPERTY(ReplicatedUsing = OnRep_SpectateTarget)
    TObjectPtr<AActor> SpectateTarget;


    UFUNCTION(Server, Reliable)
    void ServerCycleTarget();

    UFUNCTION()
    void OnRep_SpectateTarget();

    void AttachToTarget();

    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<class UInputMappingContext> SpectatorMappingContext;

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<class UInputAction> LookAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    TObjectPtr<class UInputAction> NextTargetAction;  // 좌클릭

    // 살아있는 아군 목록 (state.Dead 없는 캐릭터들)
    TArray<AActor*> GetLivingAllies() const;

    void Look(const FInputActionValue& Value);

    void OnNextTarget();  // 로컬에서 불림
};