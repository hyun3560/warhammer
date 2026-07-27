#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LMSInteractableInterface.h"
#include "LMSRescueStation.generated.h"

class UStaticMeshComponent;
class ALMS_TeamProjectCharacter;

UCLASS()
class LMS_TEAMPROJECT_API ALMSRescueStation : public AActor, public ILMSInteractableInterface
{
	GENERATED_BODY()

public:
	ALMSRescueStation();

	/** 서버 전용 — 죽은 팀원 전원 부활 */
	void PerformRescueAll();

	//~ Begin ILMSInteractableInterface
	virtual bool CanInteract_Implementation(AActor* Interactor) const override;
	virtual FGameplayTag GetInteractionType_Implementation() const override;
	virtual int32 GetInteractInputID_Implementation() const override;
	//~ End ILMSInteractableInterface

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rescue")
	UStaticMeshComponent* StationMesh;

	/** 사용 가능 횟수 (0 이하 = 무제한) */
	UPROPERTY(EditAnywhere, Category = "Rescue")
	int32 MaxUses = 1;

	UPROPERTY(ReplicatedUsing = OnRep_UsedCount)
	int32 UsedCount = 0;

	UFUNCTION()
	void OnRep_UsedCount();

	/** 소진 시 시각 변화용 — BP에서 머티리얼/이펙트 처리 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Rescue")
	void OnStationStateChanged(bool bExhausted);

	/** 부활자를 배치할 구조물 기준 거리 */
	UPROPERTY(EditAnywhere, Category = "Rescue")
	float RescueSpawnRadius = 200.f;

	/** 여러 명일 때 부채꼴 전체 각도 */
	UPROPERTY(EditAnywhere, Category = "Rescue")
	float RescueSpreadAngle = 90.f;

	void CollectDeadCharacters(TArray<ALMS_TeamProjectCharacter*>& Out) const;

	bool HasUsesLeft() const { return MaxUses <= 0 || UsedCount < MaxUses; }
};