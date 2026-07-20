// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "EnemyTableRow.generated.h"

/**
 * Native replacement for the Struct_EnemyData Blueprint struct used by DT_EnemyData.
 */
USTRUCT(BlueprintType)
struct FEnemyTableRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 ID = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	double MaxHP = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	double Damage = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	double AttackDistance = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	double CoolTime = 0.0;
};
