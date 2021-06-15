// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkShooterHUD.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "CanvasItem.h"
#include "NetworkShooterCharacter.h"
#include "NetworkShooterGameMode.h"
#include "NetworkShooterPlayerState.h"
#include "NSGameState.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

ANetworkShooterHUD::ANetworkShooterHUD()
{
	// Set the crosshair texture
	static ConstructorHelpers::FObjectFinder<UTexture2D> CrosshairTexObj(TEXT("/Game/FirstPerson/Textures/FirstPersonCrosshair"));
	CrosshairTex = CrosshairTexObj.Object;
}


void ANetworkShooterHUD::DrawHUD()
{
	Super::DrawHUD();

	// Draw very simple crosshair

	// find center of the Canvas
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);

	// offset by half the texture's dimensions so that the center of the texture aligns with the center of the Canvas
	const FVector2D CrosshairDrawPosition( (Center.X),
										   (Center.Y + 20.0f));

	// draw the crosshair
	FCanvasTileItem TileItem( CrosshairDrawPosition, CrosshairTex->Resource, FLinearColor::White);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem( TileItem );

	ANSGameState* thisGameState{ Cast<ANSGameState>(GetWorld()->GetGameState()) };
	
	if (thisGameState != nullptr && thisGameState->bInMenu)
	{
		int BlueScreenPos = 50;
		int RedScreenPos = Center.Y + 50;
		int nameSpacing = 25;
		int NumBlueteam = 1;
		int NumRedteam = 1;

		FString thisString = "BLUE TEAM:";
		DrawText(thisString, FColor::Cyan, 50, BlueScreenPos);

		thisString = "RED TEAM:";
		DrawText(thisString, FColor::Red, 50, RedScreenPos);

		for (auto player : thisGameState->PlayerArray)
		{
			ANetworkShooterPlayerState* thisPS{ Cast<ANetworkShooterPlayerState>(player) };

			if (thisPS)
			{
				if (thisPS->Team == ETeam::BLUE_TEAM)
				{
					thisString = thisPS->GetPlayerName();

					DrawText(thisString, FColor::Cyan, 50, BlueScreenPos + nameSpacing * NumBlueteam);

					NumBlueteam++;
				}
				else
				{
					thisString = thisPS->GetPlayerName();

					DrawText(thisString, FColor::Red, 50, RedScreenPos + nameSpacing * NumRedteam);

					NumRedteam++;
				}
			}
		}

		if (GetWorld()->GetAuthGameMode())
		{
			thisString = "Press R to start game";
			DrawText(thisString, FColor::Yellow, Center.X, Center.Y);
		}
		else
		{
			thisString = "Waiting on Server!!";
			DrawText(thisString, FColor::Yellow, Center.X, Center.Y);
		}
	}
	else
	{
		ANetworkShooterCharacter* ThisChar = Cast<ANetworkShooterCharacter>(UGameplayStatics::GetPlayerPawn(GetWorld(), 0));

		if (ThisChar != nullptr)
		{
			if (ThisChar->GetNetworkShooterPlayerState())
			{
				FString HUDString = FString::Printf(TEXT("Health: %f, Score: %d, Deaths: %d"), ThisChar->GetNetworkShooterPlayerState()->Health,
					ThisChar->GetNetworkShooterPlayerState()->GetScore(), ThisChar->GetNetworkShooterPlayerState()->Deaths);

				DrawText(HUDString, FColor::Yellow, 50, 50);
			}
		}
	}
}
