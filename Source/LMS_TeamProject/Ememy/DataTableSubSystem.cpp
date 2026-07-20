// Fill out your copyright notice in the Description page of Project Settings.


#include "DataTableSubSystem.h"
#include "Engine/DataTable.h"

void UDataTableSubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!EnemyDataTable)
	{
		static const TCHAR* FallbackTablePath = TEXT("/Game/KCH/DataTable/DT_MonterData.DT_MonterData");
		EnemyDataTable = LoadObject<UDataTable>(nullptr, FallbackTablePath);

		if (EnemyDataTable)
		{
			UE_LOG(LogTemp, Warning, TEXT("UDataTableSubSystem::Initialize - SetEnemyDataTable() was not called; loaded fallback table '%s'."), FallbackTablePath);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("UDataTableSubSystem::Initialize - EnemyDataTable is not set and fallback load failed. Call SetEnemyDataTable() (e.g. from GameInstance's Event Init)."));
		}
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

	return EnemyDataTable->FindRow<FEnemyTableRow>(RowName, *FString::FromInt(ID));
}
