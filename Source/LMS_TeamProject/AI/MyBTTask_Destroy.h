// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "MyBTTask_Destroy.generated.h"

/**
 * 
 */
UCLASS()
class LMS_TEAMPROJECT_API UMyBTTask_Destroy : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

private:
	UMyBTTask_Destroy();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
};
