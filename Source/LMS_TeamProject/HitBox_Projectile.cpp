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

AHitBox_Projectile::AHitBox_Projectile()
{
	PrimaryActorTick.bCanEverTick = true;

	InitialSpeed = 3000.f;
	MaxSpeed = 3000.f;
	ArcParam = 0.5f;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComponent->InitSphereRadius(15.f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &AHitBox_Projectile::OnHit);
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
		ProjectileMovement->MaxSpeed = LaunchVelocity.Size();
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
	ABaseEnemyCharacter* Enemy = Cast<ABaseEnemyCharacter>(Owner);
	if (Enemy)
	{
		const FEnemyTableRow* Data= Enemy->GetEnemyData();
		float Damage = Data ? Data->Damage : 0;
		ALMS_TeamProjectCharacter* Target = Cast<ALMS_TeamProjectCharacter>(OtherActor);
		// todo : TakeDamage
		if (Target)
		{
			Target->TakeDamage(Damage);
			Destroy();
		}
	}
	/*else
	{
		ALMS_TeamProjectCharacter* Character = Cast<ALMS_TeamProjectCharacter>(Owner);
		ULMSWeaponComponent* WeaponData = Character->GetWeaponComponent();
		if (WeaponData)
		{
			WeaponData
		}
	}*/
}
