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

	// 플레이어가 쏜 발사체가 적에게 줄 데미지 (FireRifleProjectile에서 설정). 적 발사체는 EnemyData->Damage 사용.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	float ProjectileDamage = 0.f;

	// 피격(적/플레이어/벽) 시 재생할 임팩트 이펙트. Niagara 우선, 없으면 Cascade 사용. BP 디테일에서 지정.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|FX")
	class UNiagaraSystem* ImpactEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|FX")
	class UParticleSystem* ImpactEffectCascade = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|FX")
	class USoundBase* ImpactSound = nullptr;

	// 모든 클라이언트에서 임팩트 이펙트/사운드 재생 (서버가 피격 시 호출)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayImpactFX(FVector Location, FRotator Rotation);

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

	// Pawn(적)을 Block하지 않고 Overlap으로 감지하는 경우용 (플레이어 라이플 발사체)
	UFUNCTION()
	virtual void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

private:
	// Hit/Overlap 공통 피격 처리 (양방향: 적→플레이어, 플레이어→적). 중복 방지 후 데미지 적용.
	void HandleImpact(AActor* OtherActor);

	// 임팩트 이펙트 멀티캐스트 + 소멸 (서버 전용 호출)
	void FinishImpact(const FVector& ImpactPoint);

	bool bImpacted = false;
};
