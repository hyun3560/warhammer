// Fill out your copyright notice in the Description page of Project Settings.

#include "HitBox_Projectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "LMSDamageLibrary.h"

AHitBox_Projectile::AHitBox_Projectile()
{
	PrimaryActorTick.bCanEverTick = true;

	InitialSpeed = 3000.f;
	MaxSpeed = 3000.f;
	ArcParam = 0.5f;
	Damage = 0.f;
	bDrawDebugCollision = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComponent->InitSphereRadius(15.f);
	// "Projectile" 프로필이 프로젝트에 정의돼 있지 않아 충돌 응답을 코드에서 직접 설정한다.
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	// Hit 이벤트가 발생하도록 설정 (이게 없으면 OnComponentHit이 호출되지 않음)
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->SetGenerateOverlapEvents(false);
	// 빠른 총알이 얇은 대상을 관통(터널링)하지 않도록 연속 충돌 검사 사용
	CollisionComponent->SetAllUseCCD(true);
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

	if (bDrawDebugCollision && CollisionComponent)
	{
		DrawDebugSphere(
			GetWorld(),
			CollisionComponent->GetComponentLocation(),
			CollisionComponent->GetScaledSphereRadius(),
			12,
			FColor::Green,
			false,
			-1.f,
			0,
			1.5f);
	}
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

void AHitBox_Projectile::LaunchStraight(FVector Direction, float Speed)
{
	const FVector NormalizedDirection = Direction.GetSafeNormal();

	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->Velocity = NormalizedDirection * Speed;
}

void AHitBox_Projectile::InitializeProjectile(float InRadius, float InDamage, float InInitSpeed, float InMaxSpeed)
{
	CollisionComponent->SetSphereRadius(InRadius);
	Damage = InDamage;
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

	AActor* ProjectileOwner = GetOwner();
	if (!ProjectileOwner || !OtherActor || OtherActor == ProjectileOwner)
	{
		return;
	}

	ULMSDamageLibrary::ApplyDamageEffect(ProjectileOwner, OtherActor, Damage, DamageEffect);
	Destroy();
}
