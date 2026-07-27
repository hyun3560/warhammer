#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BombSite.generated.h"

class ALMS_TeamProjectCharacter;
class UStaticMeshComponent;
class ABombSite; // 델리게이트 선언은 그보다 위에서 먼저 ABombSite* 를 쓰고있기 떄문에 전방선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBombSitePlantedSignature, ABombSite*, BombSite);

UCLASS(Blueprintable)
class LMS_TEAMPROJECT_API ABombSite : public AActor
{
    GENERATED_BODY()

public:
    ABombSite();

    // 멀티플레이 복제용 함수, 액터에서 어떤 변수들을 네트워크로 복제할지 등록하는 함수
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // 폭탄 설치 가능여부 검사
    UFUNCTION(BlueprintCallable, Category = "Bomb")
    bool CanPlantBomb(const ALMS_TeamProjectCharacter* Character) const;

    // 폭탄 설치 완료 *설치 완료를 발생시키는 함수 *상태변경 명령
    UFUNCTION(BlueprintCallable, Category = "Bomb")
    void CompletePlanting();

    // 현재 설치 완료 상태를 물어보는 함수 *상태조회
    UFUNCTION(BlueprintCallable, Category = "Bomb")
    bool IsBombPlanted() const { return bBombPlanted; }

    // 설치에 걸리는 시간을 알려줌
    UFUNCTION(BlueprintCallable, Category = "Bomb")
    float GetPlantDuration() const { return PlantDuration; }

    // 설치 가능한 최대 거리를 알려줌
    UFUNCTION(BlueprintCallable, Category = "Bomb")
    float GetMaxPlantDistance() const { return MaxPlantDistance; }

    // 설치 완료를 알려줌 *상태 변경 후 알림
    UPROPERTY(BlueprintAssignable, Category = "Bomb")
    FBombSitePlantedSignature OnBombPlanted;

protected:
    // 폭탄 설치 지점의 시각적 메시
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bomb")
    TObjectPtr<UStaticMeshComponent> SiteMesh;

    // 폭탄 설치 시간
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bomb", meta = (ClampMin = "0.1"))
    float PlantDuration = 5.0f;

    // 설치 가능한 거리
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bomb", meta = (ClampMin = "0.0"))
    float MaxPlantDistance = 300.0f;

    // 폭탄 설치 완료 여부를 저장하는 변수
    UPROPERTY(ReplicatedUsing = OnRep_BombPlanted, BlueprintReadOnly, Category = "Bomb")
    bool bBombPlanted = false;

    // 블루프린트에서 구현할 이벤트 예) 폭탄메시, 사운드
    UFUNCTION(BlueprintImplementableEvent, Category = "Bomb", meta = (DisplayName = "On Bomb Planted"))
    void ReceiveBombPlanted();

    // 복제 콜백 함수 *bBombPlanted가 서버에서 클라이언트로 복제되어 값이 바뀌면 클라이언트에서 호출
    UFUNCTION()
    void OnRep_BombPlanted();

private:
    // 알림을 모아서 실행하는 내부 함수
    void BroadcastBombPlanted();
};