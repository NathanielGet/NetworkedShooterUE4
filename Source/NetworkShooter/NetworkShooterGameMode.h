// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NetworkShooterGameMode.generated.h"

class ANetworkShooterCharacter;
class ANetworkShooterSpawnPoint;

UENUM(BlueprintType)
enum class ETeam : uint8
{
	BLUE_TEAM,
	RED_TEAM
};

UCLASS(minimalapi)
class ANetworkShooterGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ANetworkShooterGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Respawn(ANetworkShooterCharacter* Character);
	void Spawn(ANetworkShooterCharacter* Character);

private:
	TArray<ANetworkShooterCharacter*> RedTeam;
	TArray<ANetworkShooterCharacter*> BlueTeam;

	TArray<ANetworkShooterSpawnPoint*> RedSpawns;
	TArray<ANetworkShooterSpawnPoint*> BlueSpawns;

	TArray<ANetworkShooterCharacter*> ToBeSpawned;

	bool bGameStarted;
	static bool bInGameMenu;
};



