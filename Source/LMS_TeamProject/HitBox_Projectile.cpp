// Fill out your copyright notice in the Description page of Project Settings.

#include "HitBox_Projectile.h"
#include "Components/SphereComponent.h"
#include "Ememy/BaseEnemyCharacter.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "LMSAttributeSet.h"
#include "Ememy/EnemyTableRow.h"
#include "LMS_TeamProjectCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "LMSDamageLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

AHitBox_Projectile::AHitBox_Projectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	InitialSpeed = 3000.f;
	MaxSpeed = 3000.f;
	ArcParam = 0.5f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComponent->InitSphereRadius(15.f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &AHitBox_Projectile::OnHit);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AHitBox_Projectile::OnBeginOverlap);
	RootComponent = CollisionComponent;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 0.f;  // LaunchToTarget 호출 전까지 정지
	ProjectileMovement->MaxSpeed = MaxSpeed;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.f;
}

void AHitBox_Projectile::BeginPlay()
{
	Super::BeginPlay();
}

void AHitBox_Projectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

bool AHitBox_Projectile::LaunchToTarget(FVector TargetLocation)
{
	FVector LaunchVelocity;
	bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
		this,
		LaunchVelocity,
		GetActorLocation(),
		TargetLocation,
		0.f,       // 기본 중력 사용
		ArcParam
	);

	if (bSuccess)
	{
		ProjectileMovement->MaxSpeed = MaxSpeed;//LaunchVelocity.Size();
		ProjectileMovement->Velocity = LaunchVelocity;
	}

	return bSuccess;
}

void AHitBox_Projectile::InitializeProjectile(float InRadius, float InInitSpeed, float InMaxSpeed)
{
	CollisionComponent->SetSphereRadius(InRadius);
	InitialSpeed = InInitSpeed;
	MaxSpeed = InMaxSpeed;

	ProjectileMovement->InitialSpeed = InInitSpeed;
	ProjectileMovement->MaxSpeed = InMaxSpeed;
}

void AHitBox_Projectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}

	HandleImpact(OtherActor);

	// 데미지 대상이 아니어도(벽/지형 등 Block 충돌) 임팩트 이펙트 재생 후 소멸
	if (!bImpacted && !IsActorBeingDestroyed())
	{
		const FVector ImpactLoc = Hit.ImpactPoint.IsZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
		FinishImpact(ImpactLoc);
	}
}

void AHitBox_Projectile::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
	{
		return;
	}

	// 오버랩은 벽이 아닌 Pawn(적) 감지용. 유효 대상 아니면 발사체 유지(통과).
	HandleImpact(OtherActor);
}

void AHitBox_Projectile::HandleImpact(AActor* OtherActor)
{
	if (bImpacted || !HasAuthority())
	{
		return;
	}

	// 쏜 주체/무기 자신은 무시
	if (!OtherActor || OtherActor == this || OtherActor == Owner || OtherActor == GetInstigator())
	{
		return;
	}

	// 1) 적이 쏜 발사체 → 플레이어 피격 (BJH 기존 로직)
	if (ABaseEnemyCharacter* Enemy = Cast<ABaseEnemyCharacter>(Owner))
	{
		ALMS_TeamProjectCharacter* Target = Cast<ALMS_TeamProjectCharacter>(OtherActor);
		if (!Target)
		{
			return;
		}

		const FEnemyTableRow* Data = Enemy->GetEnemyData();
		const float Damage = Data ? Data->Damage : 0.f;

		ULMSDamageLibrary::ApplyDamageEffect(Enemy, Target, Damage, DamageEffect);
		FinishImpact(GetActorLocation());
		return;
	}

	// 2) 플레이어(라이플)가 쏜 발사체 → 적 피격 (신규)
	if (ABaseEnemyCharacter* EnemyTarget = Cast<ABaseEnemyCharacter>(OtherActor))
	{
		AActor* DamageSource = Owner ? Owner : this;
		ULMSDamageLibrary::ApplyDamageEffect(DamageSource, EnemyTarget, ProjectileDamage, DamageEffect);
		FinishImpact(GetActorLocation());
	}
}

void AHitBox_Projectile::FinishImpact(const FVector& ImpactPoint)
{
	if (bImpacted)
	{
		return;
	}
	bImpacted = true;

	// 모든 클라이언트에서 이펙트/사운드 재생 (소멸 전에 호출)
	Multicast_PlayImpactFX(ImpactPoint, GetActorRotation());

	Destroy();
}

void AHitBox_Projectile::Multicast_PlayImpactFX_Implementation(FVector Location, FRotator Rotation)
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Location, Rotation);
	}
	else if (ImpactEffectCascade)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactEffectCascade, Location, Rotation);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
	}
}
