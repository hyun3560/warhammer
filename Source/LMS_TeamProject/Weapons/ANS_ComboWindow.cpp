#include "ANS_ComboWindow.h"

#include "Components/SkeletalMeshComponent.h"
#include "LMSWeaponComponent.h"

void UANS_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		if (ULMSWeaponComponent* WeaponComponent = Owner->FindComponentByClass<ULMSWeaponComponent>())
		{
			WeaponComponent->OpenComboWindow();
		}
	}
}

void UANS_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr)
	{
		if (ULMSWeaponComponent* WeaponComponent = Owner->FindComponentByClass<ULMSWeaponComponent>())
		{
			WeaponComponent->CloseComboWindow();
		}
	}
}

FString UANS_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("Combo Window");
}
