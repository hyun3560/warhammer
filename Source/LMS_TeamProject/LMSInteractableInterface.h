// LMSInteractableInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "LMSInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class ULMSInteractableInterface : public UInterface
{
    GENERATED_BODY()
};

class LMS_TEAMPROJECT_API ILMSInteractableInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    bool CanInteract(AActor* Interactor) const;
    virtual bool CanInteract_Implementation(AActor* Interactor) const { return false; }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    FGameplayTag GetInteractionType() const;
    virtual FGameplayTag GetInteractionType_Implementation() const { return FGameplayTag(); }

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
    int32 GetInteractInputID() const;
    virtual int32 GetInteractInputID_Implementation() const { return INDEX_NONE; }
};