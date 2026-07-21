#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ComboSection.generated.h"

UENUM(BlueprintType)
enum class EComboSectionEventType : uint8
{
	Begin,
	End
};

UCLASS(meta = (DisplayName = "Combo Section"))
class LMS_TEAMPROJECT_API UAN_ComboSection : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo")
	EComboSectionEventType EventType = EComboSectionEventType::Begin;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combo", meta = (ClampMin = "1"))
	int32 ComboIndex = 1;
};
