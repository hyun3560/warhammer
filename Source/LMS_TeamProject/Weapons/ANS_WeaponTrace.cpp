#include "ANS_WeaponTrace.h"

#include "Components/SkeletalMeshComponent.h"
#include "LMSWeaponComponent.h"

void UANS_WeaponTrace::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

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

	WeaponComponent->BeginWeaponTrace(TraceStartSocketName, TraceEndSocketName, TraceRadius, DamageMultiplier, bDrawDebugTrace);
}

void UANS_WeaponTrace::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

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

	WeaponComponent->TickWeaponTrace(FrameDeltaTime);
}

void UANS_WeaponTrace::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

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

	WeaponComponent->EndWeaponTrace();
}

FString UANS_WeaponTrace::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Trace");
}
