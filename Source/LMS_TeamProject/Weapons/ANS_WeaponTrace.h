#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_WeaponTrace.generated.h"

UCLASS(meta = (DisplayName = "Weapon Trace"))
class LMS_TEAMPROJECT_API UANS_WeaponTrace : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Trace")
	FName TraceStartSocketName = TEXT("TraceStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Trace")
	FName TraceEndSocketName = TEXT("TraceEnd");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Trace")
	float TraceRadius = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Trace")
	float DamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Trace")
	bool bDrawDebugTrace = true;
};
