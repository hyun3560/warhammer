// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EnemyTableRow.h"
#include "DataTableSubSystem.generated.h"

class UDataTable;

/**
 * Central owner of enemy stat data. Loaded once per GameInstance so actors
 * no longer need to hold their own reference to the data table.
 *
 * EnemyDataTable has no default in C++; inject it via SetEnemyDataTable(),
 * e.g. from the project's GameInstance Blueprint's Event Init.
 */
UCLASS()
class LMS_TEAMPROJECT_API UDataTableSubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Looks up a row by ID (matched against the row name). Returns nullptr if not found. */
	const FEnemyTableRow* GetEnemyData(int32 ID) const;

	/** Assigns the data table this subsystem looks rows up from. */
	UFUNCTION(BlueprintCallable, Category = "Data")
	void SetEnemyDataTable(UDataTable* InEnemyDataTable);

protected:
	/** Data table containing per-enemy stats, keyed by ID (as the row name). */
	UPROPERTY(BlueprintReadOnly, Category = "Data")
	TObjectPtr<UDataTable> EnemyDataTable;
};
