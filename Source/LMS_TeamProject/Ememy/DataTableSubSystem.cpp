// Fill out your copyright notice in the Description page of Project Settings.


#include "DataTableSubSystem.h"
#include "Engine/DataTable.h"

void UDataTableSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!EnemyDataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDataTableSubSystem::Initialize - EnemyDataTable is not set. Call SetEnemyDataTable() (e.g. from GameInstance's Event Init)."));
	}
}

void UDataTableSubSystem::SetEnemyDataTable(UDataTable* InEnemyDataTable)
{
	EnemyDataTable = InEnemyDataTable;
}

const FEnemyTableRow* UDataTableSubSystem::GetEnemyData(int32 ID) const
{
	if (!EnemyDataTable)
	{
		return nullptr;
	}

	const FName RowName(*FString::FromInt(ID));
	return EnemyDataTable->FindRow<FEnemyTableRow>("GetEnemyData", *FString::FromInt(ID));
}
