// Fill out your copyright notice in the Description page of Project Settings.

#include "MonsterSpawner.h"
#include "BaseEnemyCharacter.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"

AMonsterSpawner::AMonsterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMonsterSpawner::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && bAutoStart)
	{
		StartSpawning();
	}
}

void AMonsterSpawner::StartSpawning()
{
	if (!HasAuthority())
	{
		return;
	}

	CheckAndRespawn();
	GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AMonsterSpawner::CheckAndRespawn, RespawnInterval, true);
}

void AMonsterSpawner::StopSpawning()
{
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
}

void AMonsterSpawner::CheckAndRespawn()
{
	AliveMonsters.RemoveAll([](const TObjectPtr<ABaseEnemyCharacter>& Monster) { return !IsValid(Monster); });

	while (AliveMonsters.Num() < MaxAliveCount)
	{
		SpawnOneMonster();
	}
}

FVector AMonsterSpawner::FindSpawnLocation() const
{
	if (UNavigationSystemV1* NavSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation NavLoc;
		if (NavSystem->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLoc))
		{
			return NavLoc.Location;
		}
	}

	const FVector2D RandomOffset = FMath::RandPointInCircle(SpawnRadius);
	return GetActorLocation() + FVector(RandomOffset.X, RandomOffset.Y, 0.f);
}

void AMonsterSpawner::SpawnOneMonster()
{
	if (!MonsterClass || !GetWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParams.Owner = this;

	ABaseEnemyCharacter* NewMonster = GetWorld()->SpawnActor<ABaseEnemyCharacter>(MonsterClass, FindSpawnLocation(), GetActorRotation(), SpawnParams);
	if (NewMonster)
	{
		AliveMonsters.Add(NewMonster);
		NewMonster->OnDestroyed.AddDynamic(this, &AMonsterSpawner::OnMonsterDestroyed);
	}
}

void AMonsterSpawner::OnMonsterDestroyed(AActor* DestroyedActor)
{
	AliveMonsters.RemoveAll([DestroyedActor](const TObjectPtr<ABaseEnemyCharacter>& Monster) { return Monster == DestroyedActor; });
}
