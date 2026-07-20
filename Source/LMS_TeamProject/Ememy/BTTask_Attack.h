// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "GameplayTagContainer.h"
#include "BTTask_Attack.generated.h"

/**
 * Plays an attack montage on the controlled character and finishes the task
 * (Succeeded/Failed) once the montage ends.
 */
UCLASS()
class LMS_TEAMPROJECT_API UBTTask_Attack : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTTask_Attack();

	// 이 태그를 가진 어빌리티를 몽타주와 함께 실행 (비어 있으면 스킵)
	UPROPERTY(EditAnywhere, Category = "Attack")
	FGameplayTag AbilityTag;

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	//virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:
	//void OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	TWeakObjectPtr<UBehaviorTreeComponent> OwningComp;
	bool bIsAborting = false;
};
