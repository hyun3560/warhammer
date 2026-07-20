// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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

	// LaunchToTarget 실패 시 직선 발사 폴백 속도 (ProjectileMovement->InitialSpeed 와 별개)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float InitialSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float MaxSpeed;

	// 포물선 높이 조절 (0.0 = 직선, 1.0 = 고포물선)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArcParam;

	// 목표 지점을 향해 포물선으로 발사
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	bool LaunchToTarget(FVector TargetLocation);

	// Spawn 직후 호출하여 충돌 반경, 데미지, 초기/최대 속도를 설정
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void InitializeProjectile(float InRadius, float InInitSpeed, float InMaxSpeed);

	FORCEINLINE class UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
};
