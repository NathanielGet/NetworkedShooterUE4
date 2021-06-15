// Fill out your copyright notice in the Description page of Project Settings.


#include "NetworkShooterPlayerState.h"
#include "Net/UnrealNetwork.h"

ANetworkShooterPlayerState::ANetworkShooterPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Health = 100.0f;
	Deaths = 0;
	Team = ETeam::BLUE_TEAM;
}

void ANetworkShooterPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkShooterPlayerState, Health);
	DOREPLIFETIME(ANetworkShooterPlayerState, Deaths);
	DOREPLIFETIME(ANetworkShooterPlayerState, Team);
}