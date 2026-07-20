// Fill out your copyright notice in the Description page of Project Settings.


#include "Hit_Box.h"

#include "LMS_TeamProjectCharacter.h"

// Sets default values
AHit_Box::AHit_Box()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AHit_Box::BeginPlay()
{
	Super::BeginPlay();
}

void AHit_Box::BoxTraceHit(float Distance, FVector Size, float Damage)
{
	TArray<FHitResult> HitResults;

	FVector StartPos =GetOwner()->GetActorLocation() + (GetOwner()->GetActorForwardVector() * Distance);
	FVector Start = StartPos;   
	FVector End = StartPos;     

	AActor* OwnerActor = GetOwner();
	FQuat Orientation = OwnerActor ? OwnerActor->GetActorQuat() : FQuat::Identity; 

	FCollisionShape BoxShape = FCollisionShape::MakeBox(Size);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	if (OwnerActor)
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}

	bool bHit = GetWorld()->SweepMultiByChannel(
		HitResults,        
		Start,
		End,
		Orientation,       
		ECC_Camera,    
		BoxShape,          
		QueryParams        
	);

	if (bHit)
	{
		for (FHitResult result : HitResults)
		{
			if (ALMS_TeamProjectCharacter* Character = Cast<ALMS_TeamProjectCharacter>(result.GetActor()))
			{
				Character->TakeDamage(Damage);
				UE_LOG(LogTemp, Log, TEXT("%f"), Damage);
				break;
			}
		}
	}

	//Destroy();
}

// Called every frame
void AHit_Box::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

