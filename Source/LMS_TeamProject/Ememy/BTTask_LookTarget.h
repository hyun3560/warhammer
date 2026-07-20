// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_LookTarget.generated.h"

/**
 * Rotates the controlled character to face the actor stored in the selected Blackboard key.
 */
UCLASS()
class LMS_TEAMPROJECT_API UBTTask_LookTarget : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_LookTarget();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};
