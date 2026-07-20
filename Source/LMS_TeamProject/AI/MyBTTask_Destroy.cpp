// Fill out your copyright notice in the Description page of Project Settings.

#include "MyBTTask_Destroy.h"

#include "../Ememy/BaseEnemyCharacter.h"
#include "AIController.h"

UMyBTTask_Destroy::UMyBTTask_Destroy() {
	NodeName = TEXT("Destroy");
}

EBTNodeResult::Type UMyBTTask_Destroy::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ABaseEnemyCharacter* Character = AIController ? Cast<ABaseEnemyCharacter>(AIController->GetPawn()) : nullptr;

	Character->Destroy();

	return EBTNodeResult::Succeeded;
}
