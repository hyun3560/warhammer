// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_TargetDistance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_TargetDistance::UBTService_TargetDistance()
{
	NodeName = TEXT("Target Distance");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
	bCreateNodeInstance = true;
	bNotifyBecomeRelevant = true;
}

void UBTService_TargetDistance::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	AIController = OwnerComp.GetAIOwner();
	BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	AttackDistance = BlackboardComp->GetValueAsFloat(TEXT("AttackDistance"));
}

void UBTService_TargetDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	if (!BlackboardComp)
	{
		return;
	}

	APawn* ControlledPawn = AIController ? AIController->GetPawn() : nullptr;

	CoolTime = BlackboardComp->GetValueAsFloat(TEXT("CoolTime"));

	CoolTime -= DeltaSeconds;

	if (!ControlledPawn || CoolTime > 0)
	{
		BlackboardComp->SetValueAsFloat(TEXT("CoolTime"), CoolTime);
		return;
	}

	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TEXT("Target")));
	if (!TargetActor)
	{
		BlackboardComp->SetValueAsBool(GetSelectedBlackboardKey(), false);
		return;
	}

	const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
	const bool bInRange = Distance <= AttackDistance;

	BlackboardComp->SetValueAsBool(GetSelectedBlackboardKey(), bInRange);
}

