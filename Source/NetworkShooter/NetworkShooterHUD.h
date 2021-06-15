// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NetworkShooterHUD.generated.h"

UCLASS()
class ANetworkShooterHUD : public AHUD
{
	GENERATED_BODY()

public:
	ANetworkShooterHUD();

	/** Primary draw call for the HUD */
	virtual void DrawHUD() override;

private:
	/** Crosshair asset pointer */
	class UTexture2D* CrosshairTex;

};

