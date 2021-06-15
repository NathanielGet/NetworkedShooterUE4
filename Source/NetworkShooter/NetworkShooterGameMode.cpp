// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkShooterGameMode.h"
#include "NetworkShooter.h"
#include "NetworkShooterHUD.h"
#include "NetworkShooterPlayerState.h"
#include "NetworkShooterSpawnPoint.h"
#include "NetworkShooterCharacter.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h" 
#include "NSGameState.h"

bool ANetworkShooterGameMode::bInGameMenu = true;

ANetworkShooterGameMode::ANetworkShooterGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;
	PlayerStateClass = ANetworkShooterPlayerState::StaticClass();

	// use our custom HUD class
	HUDClass = ANetworkShooterHUD::StaticClass();

	bReplicates = true;

	GameStateClass = ANSGameState::StaticClass();
}

void ANetworkShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		Cast<ANSGameState>(GameState)->bInMenu = bInGameMenu;

		for (TActorIterator<ANetworkShooterSpawnPoint> Iter(GetWorld()); Iter; ++Iter)
		{
			if ((*Iter)->Team == ETeam::RED_TEAM)
			{
				RedSpawns.Add(*Iter);
			}
			else
			{
				BlueSpawns.Add(*Iter);
			}
		}

		// Spawn the server
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();

		if (thisCont)
		{
			ANetworkShooterCharacter* thisChar = Cast<ANetworkShooterCharacter>(thisCont->GetPawn());

			thisChar->SetTeam(ETeam::BLUE_TEAM);
			BlueTeam.Add(thisChar);
			Spawn(thisChar);
		}

		AGameModeBase::bUseSeamlessTravel = true;
	}
}

void ANetworkShooterGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Quit || EndPlayReason == EEndPlayReason::EndPlayInEditor)
	{
		bInGameMenu = true;
	}
}

void ANetworkShooterGameMode::Tick(float DeltaSeconds)
{
	if(GetLocalRole() == ROLE_Authority)
	{
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();

		if (ToBeSpawned.Num() != 0)
		{
			for (auto charToSpawn : ToBeSpawned)
			{
				Spawn(charToSpawn);
			}
		}

		if (thisCont != nullptr && thisCont->IsInputKeyDown(EKeys::R))
		{
			bInGameMenu = false;
			GetWorld()->ServerTravel(L"/Game/FirstPersonCPP/Maps/FirstPersonExampleMap?Listen");

			Cast<ANSGameState>(GameState)->bInMenu = bInGameMenu;
		}
	}
}

void ANetworkShooterGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	ANetworkShooterCharacter* Teamless = Cast<ANetworkShooterCharacter>(NewPlayer->GetPawn());
	ANetworkShooterPlayerState* NPlayerState = Cast<ANetworkShooterPlayerState>(NewPlayer->PlayerState);

	if (Teamless != nullptr && NPlayerState != nullptr)
	{
		Teamless->SetNetworkShooterPlayerState(NPlayerState);
	}

	// Assign Team and spawn
	if (GetLocalRole() == ROLE_Authority && Teamless != nullptr)
	{
		if (BlueTeam.Num() > RedTeam.Num())
		{
			RedTeam.Add(Teamless);
			NPlayerState->Team = ETeam::RED_TEAM;
		}
		else if (BlueTeam.Num() < RedTeam.Num())
		{
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}
		else
		{
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}

		Teamless->CurrentTeam = NPlayerState->Team;
		Teamless->SetTeam(NPlayerState->Team);
		Spawn(Teamless);
	}
}

void ANetworkShooterGameMode::Spawn(ANetworkShooterCharacter* Character)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// Find Spawn point that is not blocked
		ANetworkShooterSpawnPoint* thisSpawn{ nullptr };
		TArray<ANetworkShooterSpawnPoint*>* targetTeam{ nullptr };

		if (Character->CurrentTeam == ETeam::BLUE_TEAM)
		{
			targetTeam = &BlueSpawns;
		}
		else
		{
			targetTeam = &RedSpawns;
		}

		for (auto Spawn : (*targetTeam))
		{
			Spawn->UpdateOverlaps();
			if (!Spawn->GetBlocked())
			{
				// Remove from spawn queue location
				if (ToBeSpawned.Find(Character) != INDEX_NONE)
				{
					ToBeSpawned.Remove(Character);
				}

				// Otherwise set actor location
				Character->SetActorLocation(Spawn->GetActorLocation());

				Spawn->UpdateOverlaps();

				return;
			}
		}

		if (ToBeSpawned.Find(Character) == INDEX_NONE)
		{
			ToBeSpawned.Add(Character);
		}
	}
}

void ANetworkShooterGameMode::Respawn(ANetworkShooterCharacter* Character)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		AController* thisPC = Character->GetController();
		Character->DetachFromControllerPendingDestroy();

		ANetworkShooterCharacter* newChar = Cast<ANetworkShooterCharacter>(GetWorld()->SpawnActor(DefaultPawnClass));

		if (newChar)
		{
			thisPC->Possess(newChar);
			ANetworkShooterPlayerState* thisPS = Cast<ANetworkShooterPlayerState>(newChar->GetController()->PlayerState);

			newChar->CurrentTeam = thisPS->Team;
			newChar->SetNetworkShooterPlayerState(thisPS);

			Spawn(newChar);

			newChar->SetTeam(newChar->GetNetworkShooterPlayerState()->Team);
		}
	}
}