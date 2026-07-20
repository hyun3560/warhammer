// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

/**
 *
 */
UCLASS()
class LMS_TEAMPROJECT_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	/** Behavior tree asset to run when this controller possesses a pawn */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "BlackBoard")
	TObjectPtr<class UBehaviorTree> BehaviorTreeAsset;

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	/** Sight sense used to detect other actors */
	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<class UAIPerceptionComponent> AIPerceptionComp;

	UPROPERTY(VisibleAnywhere, Category = "AI|Perception")
	TObjectPtr<class UAISenseConfig_Sight> SightConfig;
};
