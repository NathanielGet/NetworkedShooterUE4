// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "NetworkShooterGameMode.h"
#include "NetworkShooterPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class NETWORKSHOOTER_API ANetworkShooterPlayerState : public APlayerState
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Replicated)
	float Health;

	UPROPERTY(Replicated)
	uint8 Deaths;
	
	UPROPERTY(Replicated)
	ETeam Team;
};
