// Fill out your copyright notice in the Description page of Project Settings.

#include "HitBox_Projectile.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectTypes.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"
#include "LMSDamageLibrary.h"

AHitBox_Projectile::AHitBox_Projectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// 서버에서 스폰한 총알을 클라이언트에도 생성한다.
	bReplicates = true;
	// 위치를 매 프레임 복제하지 않고, 각 클라이언트의 ProjectileMovement가 스스로 굴린다 (부드러운 이동).
	SetReplicateMovement(false);

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

	// 클라이언트에서 초기 복제로 LaunchVelocity가 이미 채워진 경우, OnRep이 호출되지 않을 수 있어 여기서 반영한다.
	if (!HasAuthority() && !LaunchVelocity.IsZero())
	{
		OnRep_LaunchVelocity();
	}
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
	FVector OutVelocity;
	bool bSuccess = UGameplayStatics::SuggestProjectileVelocity_CustomArc(
		this,
		OutVelocity,
		GetActorLocation(),
		TargetLocation,
		0.f,       // 기본 중력 사용
		ArcParam
	);

	if (bSuccess)
	{
		ProjectileMovement->MaxSpeed = OutVelocity.Size();
		ProjectileMovement->Velocity = OutVelocity;
	}

	return bSuccess;
}

void AHitBox_Projectile::LaunchStraight(FVector Direction, float Speed)
{
	const FVector NormalizedDirection = Direction.GetSafeNormal();

	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->Velocity = NormalizedDirection * Speed;

	// 클라이언트에도 발사 속도를 복제하여 각자 ProjectileMovement가 굴리게 한다.
	if (HasAuthority())
	{
		LaunchVelocity = ProjectileMovement->Velocity;
	}
}

void AHitBox_Projectile::OnRep_LaunchVelocity()
{
	// 클라이언트에서 서버가 정한 속도로 발사를 재현한다.
	if (ProjectileMovement)
	{
		ProjectileMovement->MaxSpeed = LaunchVelocity.Size();
		ProjectileMovement->Velocity = LaunchVelocity;
		ProjectileMovement->UpdateComponentVelocity();
	}
}

void AHitBox_Projectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHitBox_Projectile, LaunchVelocity);
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
	if (!ProjectileOwner || OtherActor == ProjectileOwner)
	{
		return;
	}

	// 데미지 적용 대상(ASC 보유)인지에 따라 캐릭터/월드 피격 이펙트를 구분한다.
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	const bool bHitCharacter = TargetASC != nullptr;

	if (bHitCharacter)
	{
		ULMSDamageLibrary::ApplyDamageEffect(ProjectileOwner, OtherActor, Damage, DamageEffect);
	}

	// 충돌 지점/법선을 파라미터에 담아 GameplayCue 실행 (Multicast로 모든 클라이언트에 전파됨)
	if (UAbilitySystemComponent* OwnerASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ProjectileOwner))
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = Hit.ImpactPoint.IsZero() ? GetActorLocation() : FVector(Hit.ImpactPoint);
		CueParams.Normal = Hit.ImpactNormal;
		CueParams.Instigator = ProjectileOwner;
		CueParams.EffectCauser = this;

		const FGameplayTag CueTag = bHitCharacter
			? FGameplayTag::RequestGameplayTag(FName("GameplayCue.Projectile.Hit.Character"))
			: FGameplayTag::RequestGameplayTag(FName("GameplayCue.Projectile.Hit.World"));

		OwnerASC->ExecuteGameplayCue(CueTag, CueParams);
	}

	Destroy();
}
