// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonsterSpawner.generated.h"

class ABaseEnemyCharacter;

// 레벨에 배치해서 사용하는 몬스터 스포너.
// 지정된 몬스터 클래스를 최대 개체수만큼 유지하고, 죽어서 사라지면 일정 시간 뒤 다시 채워 넣는다.
UCLASS()
class LMS_TEAMPROJECT_API AMonsterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMonsterSpawner();

protected:
	virtual void BeginPlay() override;

public:
	// 스폰할 몬스터 클래스
	UPROPERTY(EditAnywhere, Category = "Spawn")
	TSubclassOf<ABaseEnemyCharacter> MonsterClass;

	// 이 스포너가 동시에 유지할 최대 몬스터 수
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0"))
	int32 MaxAliveCount = 3;

	// 부족한 개체수를 확인하고 다시 채우는 주기(초)
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0.1"))
	float RespawnInterval = 5.f;

	// 스포너 위치를 중심으로 몬스터를 흩뿌릴 반경
	UPROPERTY(EditAnywhere, Category = "Spawn", meta = (ClampMin = "0"))
	float SpawnRadius = 300.f;

	// BeginPlay에서 자동으로 스폰을 시작할지 여부
	UPROPERTY(EditAnywhere, Category = "Spawn")
	bool bAutoStart = true;

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void StartSpawning();

	UFUNCTION(BlueprintCallable, Category = "Spawn")
	void StopSpawning();

private:
	UPROPERTY()
	TArray<TObjectPtr<ABaseEnemyCharacter>> AliveMonsters;

	FTimerHandle RespawnTimerHandle;

	void CheckAndRespawn();
	void SpawnOneMonster();
	FVector FindSpawnLocation() const;

	UFUNCTION()
	void OnMonsterDestroyed(AActor* DestroyedActor);
};
