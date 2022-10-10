// Fill out your copyright notice in the Description page of Project Settings.


#include "PickupSpawnPoint.h"

#include "Pickup.h"


APickupSpawnPoint::APickupSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates =  true;
}

void APickupSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	StartSpawnPickupTimer((AActor*)nullptr); // We are passing in the nullptr just so we can satisfy the input parameters of the OnDestroyed Delegate and use a C style cast which is very dangerous nut for this case it's okay
}

void APickupSpawnPoint::SpawnPickup()
{
	int32 NumPickupClasses = PickupClasses.Num();
	if (NumPickupClasses > 0)
	{
		int32 Selection = FMath::RandRange(0, NumPickupClasses - 1);
		SpawnedPickup = GetWorld()->SpawnActor<APickup>(
			PickupClasses[Selection],
			GetActorTransform()
		);

		if (HasAuthority() && SpawnedPickup)
		{
			SpawnedPickup->OnDestroyed.AddDynamic(this, &APickupSpawnPoint::StartSpawnPickupTimer);
		}
	}
}

void APickupSpawnPoint::SpawnPickupTimerFinish()
{
	if (HasAuthority())
	{
		SpawnPickup();
	}
}

void APickupSpawnPoint::StartSpawnPickupTimer(AActor* DestroyedActor)
{
	const float SpawnTime = FMath::FRandRange(SpawnPickupTimeMin, SpawnPickupTimeMax);
	GetWorldTimerManager().SetTimer(
		SpawnPickupTimer,
		this,
		&APickupSpawnPoint::SpawnPickupTimerFinish,
		SpawnTime
	);
}

void APickupSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

