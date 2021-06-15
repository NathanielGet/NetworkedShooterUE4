// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetworkShooterGameMode.h"
#include "NetworkShooterSpawnPoint.generated.h"

UCLASS()
class NETWORKSHOOTER_API ANetworkShooterSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANetworkShooterSpawnPoint();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void ActorBeginOverlaps(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void ActorEndOverlaps(AActor* OverlappedActor, AActor* OtherActor);

	bool GetBlocked()
	{
		return OverlappingActors.Num() != 0;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ETeam Team;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	class UCapsuleComponent* SpawnCapsule;

	TArray<AActor*> OverlappingActors;
};
