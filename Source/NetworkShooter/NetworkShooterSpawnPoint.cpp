// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkShooterSpawnPoint.h"
#include "Components/CapsuleComponent.h"

// Sets default values
ANetworkShooterSpawnPoint::ANetworkShooterSpawnPoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));;
	SpawnCapsule->SetCollisionProfileName("OverlapAllDynamic");
	SpawnCapsule->SetGenerateOverlapEvents(true);
	SpawnCapsule->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Overlap);

	OnActorBeginOverlap.AddDynamic(this, &ANetworkShooterSpawnPoint::ActorBeginOverlaps);
	OnActorEndOverlap.AddDynamic(this, &ANetworkShooterSpawnPoint::ActorEndOverlaps);
}

void ANetworkShooterSpawnPoint::OnConstruction(const FTransform& Transform)
{
	if (Team == ETeam::RED_TEAM)
	{
		SpawnCapsule->ShapeColor = FColor(255, 0, 0);
	}
	else // (Team == ETeam::BLUE_TEAM
	{
		SpawnCapsule->ShapeColor = FColor(0, 0, 255);
	}
}

// Called when the game starts or when spawned
void ANetworkShooterSpawnPoint::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ANetworkShooterSpawnPoint::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SpawnCapsule->UpdateOverlaps();
}

void ANetworkShooterSpawnPoint::ActorBeginOverlaps(AActor* OverlappedActor, AActor* OtherActor)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		if (OverlappingActors.Find(OtherActor) == INDEX_NONE)
		{
			OverlappingActors.Add(OtherActor);
		}
	}
}

void ANetworkShooterSpawnPoint::ActorEndOverlaps(AActor* OverlappedActor, AActor* OtherActor)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		if (OverlappingActors.Find(OtherActor) != INDEX_NONE)
		{
			OverlappingActors.Remove(OtherActor);
		}
	}
}

