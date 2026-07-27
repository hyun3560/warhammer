#include "LMSRescueStation.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "LMS_TeamProjectCharacter.h"
#include "LMSGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ALMSRescueStation::ALMSRescueStation()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	SetRootComponent(StationMesh);

	StationMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// TODO(Interactable): 임시 — WorldDynamic으로 디텍터에 잡히게 함.
	// 최신 브랜치 머지 후 SetCollisionObjectType(ECC_GameTraceChannel3)으로 교체.
	StationMesh->SetCollisionObjectType(ECC_WorldDynamic);

	StationMesh->SetCollisionResponseToAllChannels(ECR_Block);
	StationMesh->SetGenerateOverlapEvents(true);
}

void ALMSRescueStation::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALMSRescueStation, UsedCount);
}

void ALMSRescueStation::OnRep_UsedCount()
{
	OnStationStateChanged(!HasUsesLeft());
}

//////////////////////////////////////////////////////////////////////////
// ILMSInteractableInterface

bool ALMSRescueStation::CanInteract_Implementation(AActor* Interactor) const
{
	if (!Interactor || !HasUsesLeft())
	{
		return false;
	}

	// 상호작용하는 본인이 멀쩡해야 함 (다운/사망 중엔 불가)
	if (const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Interactor))
	{
		static const FGameplayTag IncapTag =
			FGameplayTag::RequestGameplayTag(FName("state.Incapacitated"));
		static const FGameplayTag DeadTag =
			FGameplayTag::RequestGameplayTag(FName("state.Dead"));

		UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
		if (!ASC || ASC->HasMatchingGameplayTag(IncapTag) || ASC->HasMatchingGameplayTag(DeadTag))
		{
			return false;
		}
	}

	// 살릴 대상이 하나라도 있어야 프롬프트가 뜸
	TArray<ALMS_TeamProjectCharacter*> DeadChars;
	CollectDeadCharacters(DeadChars);

	return DeadChars.Num() > 0;
}

FGameplayTag ALMSRescueStation::GetInteractionType_Implementation() const
{
	static const FGameplayTag RescueType =
		FGameplayTag::RequestGameplayTag(FName("Interaction.RescueAll"));
	return RescueType;
}

int32 ALMSRescueStation::GetInteractInputID_Implementation() const
{
	return static_cast<int32>(ELMSAbilityInputID::Interact_RescueAll);
}

//////////////////////////////////////////////////////////////////////////
// 부활 처리

void ALMSRescueStation::CollectDeadCharacters(TArray<ALMS_TeamProjectCharacter*>& Out) const
{
	Out.Reset();

	static const FGameplayTag DeadTag =
		FGameplayTag::RequestGameplayTag(FName("state.Dead"));

	// 죽은 플레이어는 PlayerState->GetPawn()이 스펙테이터 폰이므로
	// PlayerArray로는 시체를 못 찾음. 월드에서 직접 수집.
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(), ALMS_TeamProjectCharacter::StaticClass(), Found);

	for (AActor* Actor : Found)
	{
		ALMS_TeamProjectCharacter* Char = Cast<ALMS_TeamProjectCharacter>(Actor);
		if (!Char)
		{
			continue;
		}

		UAbilitySystemComponent* ASC = Char->GetAbilitySystemComponent();
		if (ASC && ASC->HasMatchingGameplayTag(DeadTag))
		{
			Out.Add(Char);
		}
	}
}

void ALMSRescueStation::PerformRescueAll()
{
	if (!HasAuthority() || !HasUsesLeft())
	{
		return;
	}

	TArray<ALMS_TeamProjectCharacter*> DeadChars;
	CollectDeadCharacters(DeadChars);

	if (DeadChars.Num() == 0)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const int32 Count = DeadChars.Num();

	for (int32 i = 0; i < Count; ++i)
	{
		ALMS_TeamProjectCharacter* Char = DeadChars[i];
		if (!Char)
		{
			continue;
		}

		// 구조물 정면 기준 부채꼴 배치
		const float AngleDeg = (Count == 1)
			? 0.f
			: -RescueSpreadAngle * 0.5f + (RescueSpreadAngle / (Count - 1)) * i;

		const FVector Dir = Forward.RotateAngleAxis(AngleDeg, FVector::UpVector);

		FVector SpawnLoc = Origin + Dir * RescueSpawnRadius;
		SpawnLoc.Z = Origin.Z + Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();

		// 구조물을 등지고 서게
		const FRotator SpawnRot(0.f, Dir.Rotation().Yaw, 0.f);

		Char->RescueFromDeath(SpawnLoc, SpawnRot);
	}

	++UsedCount;
	OnRep_UsedCount();   // 서버는 OnRep이 안 불리므로 직접 호출
}