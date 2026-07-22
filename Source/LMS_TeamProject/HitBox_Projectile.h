// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "HitBox_Projectile.generated.h"


UCLASS()
class LMS_TEAMPROJECT_API AHitBox_Projectile : public AActor
{
	GENERATED_BODY()

public:
	AHitBox_Projectile();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<class UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS")
	TSubclassOf<UGameplayEffect> DamageEffect;

	// LaunchToTarget 실패 시 직선 발사 폴백 속도 (ProjectileMovement->InitialSpeed 와 별개)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float InitialSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MaxSpeed;

	// 포물선 높이 조절 (0.0 = 직선, 1.0 = 고포물선)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArcParam;

	// OnHit에서 적용할 데미지 (InitializeProjectile로 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float Damage;

	// 켜면 매 프레임 콜리전 구(Sphere)를 디버그로 그린다 (개발용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Debug")
	bool bDrawDebugCollision;

	// 목표 지점을 향해 포물선으로 발사 (Enemy 등 곡사 투사체용)
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	bool LaunchToTarget(FVector TargetLocation);

	// Direction 방향으로 직선 발사 (라이플 등 직사 투사체용)
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void LaunchStraight(FVector Direction, float Speed);

	// Spawn 직후 호출하여 충돌 반경, 데미지, 초기/최대 속도를 설정
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitializeProjectile(float InRadius, float InDamage, float InInitSpeed, float InMaxSpeed);

	FORCEINLINE class UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
