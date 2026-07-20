// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_TargetDistance.generated.h"

class AAIController;
class UBlackboardComponent;

/**
 * Reads the "Target" object value from the Blackboard, measures the distance between
 * the controlled pawn and the target, and writes whether that distance is within
 * DistanceThreshold into the "bInRange" Blackboard key.
 */
UCLASS()
class LMS_TEAMPROJECT_API UBTService_TargetDistance : public UBTService_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTService_TargetDistance();

	/** Distance (in unreal units) compared against the current target distance */
	UPROPERTY(EditAnywhere, Category = "AI")
	float DistanceThreshold = 100.f;

	UPROPERTY()
	TObjectPtr<AAIController> AIController;

	UPROPERTY()
	TObjectPtr<UBlackboardComponent> BlackboardComp;

	double CoolTime = 0.f;
	double AttackDistance = 0.f;

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};
