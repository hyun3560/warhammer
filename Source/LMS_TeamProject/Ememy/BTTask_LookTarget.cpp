// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_LookTarget.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_LookTarget::UBTTask_LookTarget()
{
	NodeName = TEXT("Look Target");
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_LookTarget, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_LookTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ACharacter* Character = AIController ? Cast<ACharacter>(AIController->GetPawn()) : nullptr;
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (!Character || !BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(GetSelectedBlackboardKey()));
	if (!TargetActor)
	{
		return EBTNodeResult::Failed;
	}

	FVector Direction = TargetActor->GetActorLocation() - Character->GetActorLocation();
	Direction.Z = 0.f;

	if (Direction.IsNearlyZero())
	{
		return EBTNodeResult::Succeeded;
	}

	Character->SetActorRotation(Direction.Rotation());

	return EBTNodeResult::Succeeded;
}

