// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_Attack.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "BaseEnemyCharacter.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"

UBTTask_Attack::UBTTask_Attack()
{
	NodeName = TEXT("Attack");
	bCreateNodeInstance = true;
}

EBTNodeResult::Type UBTTask_Attack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	ABaseEnemyCharacter* Character = AIController ? Cast<ABaseEnemyCharacter>(AIController->GetPawn()) : nullptr;
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;

	if (!Character)
	{
		return EBTNodeResult::Failed;
	}

	if (AbilityTag.IsValid())
	{
		if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
		{
			FGameplayTagContainer TagContainer(AbilityTag);
			ASC->TryActivateAbilitiesByTag(TagContainer);
		}
	}

	OwningComp = &OwnerComp;

	/*FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UBTTask_Attack::OnAttackMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndDelegate, Character->AttackMontage);*/

	//return EBTNodeResult::InProgress;

	return EBTNodeResult::Succeeded;
}

//EBTNodeResult::Type UBTTask_Attack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
//{
//	AAIController* AIController = OwnerComp.GetAIOwner();
//	ABaseEnemyCharacter* Character = AIController ? Cast<ABaseEnemyCharacter>(AIController->GetPawn()) : nullptr;
//	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
//
//	if (!AnimInstance || !Character->AttackMontage)
//	{
//		return EBTNodeResult::Aborted;
//	}
//
//	bIsAborting = true;
//	AnimInstance->Montage_Stop(0.f, Character->AttackMontage);
//
//	return EBTNodeResult::InProgress;
//}
//
//void UBTTask_Attack::OnAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
//{
//	if (UBehaviorTreeComponent* BTComp = OwningComp.Get())
//	{
//		if (UBlackboardComponent* BlackboardComp = BTComp->GetBlackboardComponent())
//		{
//			const float MaxCoolTime = BlackboardComp->GetValueAsFloat(TEXT("MaxCoolTime"));
//			BlackboardComp->SetValueAsFloat(TEXT("CoolTime"), MaxCoolTime);
//			BlackboardComp->SetValueAsBool(TEXT("CanAttack"), false);
//		}
//
//		if (bIsAborting)
//		{
//			bIsAborting = false;
//			FinishLatentAbort(*BTComp);
//		}
//		else
//		{
//			FinishLatentTask(*BTComp, bInterrupted ? EBTNodeResult::Failed : EBTNodeResult::Succeeded);
//		}
//	}
//}

