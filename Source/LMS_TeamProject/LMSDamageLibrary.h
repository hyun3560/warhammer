#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffect.h"
#include "LMSDamageLibrary.generated.h"

UCLASS()
class LMS_TEAMPROJECT_API ULMSDamageLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 공격자가 타겟에게 데미지 GE 적용 (타겟은 이미 확정된 상태로 받음)
	UFUNCTION(BlueprintCallable, Category = "GAS|Damage")
	static void ApplyDamageEffect(
		AActor* Source,
		AActor* Target,
		float Damage,
		TSubclassOf<UGameplayEffect> DamageEffect);
};