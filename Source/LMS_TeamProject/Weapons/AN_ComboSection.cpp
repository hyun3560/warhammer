#include "AN_ComboSection.h"

#include "Components/SkeletalMeshComponent.h"
#include "LMSWeaponComponent.h"

void UAN_ComboSection::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	ULMSWeaponComponent* WeaponComponent = Owner ? Owner->FindComponentByClass<ULMSWeaponComponent>() : nullptr;
	if (!WeaponComponent)
	{
		return;
	}

	if (EventType == EComboSectionEventType::Begin)
	{
		WeaponComponent->NotifyComboSectionBegin(ComboIndex);
	}
	else
	{
		WeaponComponent->NotifyComboSectionEnd(ComboIndex);
	}
}

FString UAN_ComboSection::GetNotifyName_Implementation() const
{
	const TCHAR* EventName = EventType == EComboSectionEventType::Begin ? TEXT("Begin") : TEXT("End");
	return FString::Printf(TEXT("Combo %s %d"), EventName, ComboIndex);
}
