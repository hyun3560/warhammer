// Fill out your copyright notice in the Description page of Project Settings.

#include "BTService_SearchTarget.h"
#include "AIController.h"
#include "../LMS_TeamProjectCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"

namespace
{
	bool IsActorDead(AActor* Actor)
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Actor);
		if (!ASI)
		{
			return false;
		}

		UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
		if (!ASC)
		{
			return false;
		}

		static const FGameplayTag DeadTag =
			FGameplayTag::RequestGameplayTag(FName("state.Dead"));
		return ASC->HasMatchingGameplayTag(DeadTag);
	}
}

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

	AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(GetSelectedBlackboardKey()));
	if (CurrentTarget && IsActorDead(CurrentTarget))
	{
		BlackboardComp->ClearValue(GetSelectedBlackboardKey());
		CurrentTarget = nullptr;
	}

	if (CurrentTarget)
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

	for (AActor* Actor : SightedActors)
	{
		if (!Actor || !Actor->IsA<ALMS_TeamProjectCharacter>())
		{
			continue;
		}

		if (IsActorDead(Actor))
		{
			continue;
		}

		BlackboardComp->SetValueAsObject(GetSelectedBlackboardKey(), Actor);
		return;
	}
}
