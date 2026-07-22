#include "BombSite.h"

#include "../LMS_TeamProjectCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

// BombSite 액터의 기본 설정을 하는 생성자
ABombSite::ABombSite()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true; 

    // SiteMesh 컴포넌트를 생성
    SiteMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SiteMesh"));
    SetRootComponent(SiteMesh);
    // 충돌판정 Query로 켠다
    SiteMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    // 모든 채널에 대해 Block으로 반응
    SiteMesh->SetCollisionResponseToAllChannels(ECR_Block);
}

// 이 액터에서 어떤 변수들을 네트워크로 복제할지 등록
void ABombSite::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); // 부모 클래스인 AActor의 복제 설정도 유지

    DOREPLIFETIME(ABombSite, bBombPlanted); // ABombSite의 bBombPlanted 변수를 네트워크로 복제
}

// 설치 가능 여부를 검사
bool ABombSite::CanPlantBomb(const ALMS_TeamProjectCharacter* Character) const
{
    if (bBombPlanted || !Character)
    {
        return false;
    }

    // 거리 검사               플레이어 위치              BombSite의 위치
    return FVector::Dist(Character->GetActorLocation(), GetActorLocation()) <= MaxPlantDistance;
}

// 설치 완료 처리
void ABombSite::CompletePlanting()
{ 
    if (!HasAuthority() || bBombPlanted) // !HasAuthority() : 서버가 아니면 실행X
    {
        return;
    }

    bBombPlanted = true;
    BroadcastBombPlanted();
}

void ABombSite::OnRep_BombPlanted()
{
    if (bBombPlanted) // 서버에서 설치여부를 저장하는 변수가 true라면
    {
        BroadcastBombPlanted(); // 클라이언트에서도 설치 완료 이벤트를 실행
    }
}

// 설치 완료 알림을 모아서 실행
void ABombSite::BroadcastBombPlanted()
{
    OnBombPlanted.Broadcast(this);
    ReceiveBombPlanted();
}