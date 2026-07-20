// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_AttackReset.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_AttackReset::UBTTask_AttackReset()
{
	NodeName = TEXT("Attack Reset");
}

EBTNodeResult::Type UBTTask_AttackReset::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (Blackboard == nullptr)
	{
		return EBTNodeResult::Failed;
	}
	
	float MaxCool = Blackboard->GetValueAsFloat("MaxCoolTime");
	Blackboard->SetValueAsFloat(GetSelectedBlackboardKey(), MaxCool);
	Blackboard->SetValueAsBool("CanAttack", false);
	
	return EBTNodeResult::Succeeded;
}
