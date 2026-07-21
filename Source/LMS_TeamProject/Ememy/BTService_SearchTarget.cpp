// Fill out your copyright notice in the Description page of Project Settings.

#include "BTService_SearchTarget.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"

UBTService_SearchTarget::UBTService_SearchTarget()
{
	NodeName = TEXT("Search Target");
}

void UBTService_SearchTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!AIController || !BlackboardComp)
	{
		return;
	}

	UAIPerceptionComponent* Perception = AIController->GetPerceptionComponent();
	if (!Perception)
	{
		return;
	}

	TArray<AActor*> SightedActors;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), SightedActors);

	AActor* FoundCharacter = nullptr;
	for (AActor* Actor : SightedActors)
	{
		if (Actor && Actor->IsA<ACharacter>())
		{
			FoundCharacter = Actor;
			break;
		}
	}
	if (FoundCharacter)
	{
		BlackboardComp->SetValueAsObject(GetSelectedBlackboardKey(), FoundCharacter);
	}
	else
	{
		BlackboardComp->ClearValue(GetSelectedBlackboardKey());
	}
}
