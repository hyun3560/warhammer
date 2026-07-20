// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "BaseEnemyCharacter.h"

AEnemyAIController::AEnemyAIController()
{
	AIPerceptionComp = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));

	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	AIPerceptionComp->ConfigureSense(*SightConfig);
	AIPerceptionComp->SetDominantSense(SightConfig->GetSenseImplementation());

	SetPerceptionComponent(*AIPerceptionComp);
}

void AEnemyAIController::BeginPlay()
{
	Super::BeginPlay();

	if (Blackboard)
	{
		ABaseEnemyCharacter* Enemy = Cast<ABaseEnemyCharacter>(GetPawn());
		if (Enemy)
		{
			const FEnemyTableRow* Data = Enemy->GetEnemyData();
			if (Data == nullptr)
			{
				UE_LOG(LogTemp, Log, TEXT("%s"), Enemy->GetFName());
				return;
			}
			Blackboard->SetValueAsFloat(TEXT("CoolTime"), Data->CoolTime);
			Blackboard->SetValueAsFloat(TEXT("MaxCoolTime"), Data->CoolTime);
			Blackboard->SetValueAsFloat(TEXT("AttackDistance"), Data->AttackDistance);
		}
	}
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (!BehaviorTreeAsset)
	{
		return;
	}

	RunBehaviorTree(BehaviorTreeAsset);

	if (Blackboard)
	{
		Blackboard->SetValueAsVector(TEXT("SpawnLocation"), InPawn->GetActorLocation());		
	}
}

void AEnemyAIController::OnUnPossess()
{
	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(BrainComponent);
	if (BTComp)
	{
		BTComp->StopTree();
	}

	Super::OnUnPossess();
}
