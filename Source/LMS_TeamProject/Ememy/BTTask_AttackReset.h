// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_AttackReset.generated.h"

/**
 * 
 */
UCLASS()
class LMS_TEAMPROJECT_API UBTTask_AttackReset : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_AttackReset();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
