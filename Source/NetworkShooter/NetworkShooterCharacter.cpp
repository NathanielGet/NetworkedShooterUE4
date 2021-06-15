// Copyright Epic Games, Inc. All Rights Reserved.

#include "NetworkShooterCharacter.h"
#include "NetworkShooterProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "NetworkShooterPlayerState.h"
#include "DrawDebugHelpers.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ANetworkShooterCharacter

ANetworkShooterCharacter::ANetworkShooterCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FP_MESH = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	FP_MESH->SetOnlyOwnerSee(true);
	FP_MESH->SetupAttachment(FirstPersonCameraComponent);
	FP_MESH->bCastDynamicShadow = false;
	FP_MESH->CastShadow = false;
	FP_MESH->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	FP_MESH->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component (1st person)
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(FP_MESH, TEXT("GripPoint"));

	// Create a gun mesh component (3rd person)
	TP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TP_Gun"));
	TP_Gun->SetOwnerNoSee(true);
	TP_Gun->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));

	GetMesh()->SetOwnerNoSee(true);

	// Create particle systems
	TP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysTP"));

	TP_GunShotParticle->bAutoActivate = false;
	TP_GunShotParticle->SetupAttachment(TP_Gun);
	TP_GunShotParticle->SetOwnerNoSee(true);

	FP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysFP"));

	FP_GunShotParticle->bAutoActivate = false;
	FP_GunShotParticle->SetupAttachment(FP_Gun);
	FP_GunShotParticle->SetOnlyOwnerSee(true);

	BulletParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BulletSysTP"));
	BulletParticle->bAutoActivate = false;
	BulletParticle->SetupAttachment(FirstPersonCameraComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Note: The ProjectileClass and the skeletal mesh/anim blueprints for Mesh1P, FP_Gun, and VR_Gun 
	// are set in the derived blueprint asset named MyCharacter to avoid direct content references in C++.

	// Create VR Controllers.
	R_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("R_MotionController"));
	R_MotionController->MotionSource = FXRMotionControllerBase::RightHandSourceId;
	R_MotionController->SetupAttachment(RootComponent);
	L_MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("L_MotionController"));
	L_MotionController->SetupAttachment(RootComponent);

	// Create a gun and attach it to the right-hand VR controller.
	// Create a gun mesh component
	VR_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VR_Gun"));
	VR_Gun->SetOnlyOwnerSee(false);			// otherwise won't be visible in the multiplayer
	VR_Gun->bCastDynamicShadow = false;
	VR_Gun->CastShadow = false;
	VR_Gun->SetupAttachment(R_MotionController);
	VR_Gun->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	VR_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("VR_MuzzleLocation"));
	VR_MuzzleLocation->SetupAttachment(VR_Gun);
	VR_MuzzleLocation->SetRelativeLocation(FVector(0.000004, 53.999992, 10.000000));
	VR_MuzzleLocation->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));		// Counteract the rotation of the VR gun model.

	// Uncomment the following line to turn motion controllers on by default:
	//bUsingMotionControllers = true;
}

void ANetworkShooterCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (GetLocalRole() != ROLE_Authority)
	{
		SetTeam(CurrentTeam);
	}
}

void ANetworkShooterCharacter::SetTeam_Implementation(ETeam NewTeam)
{
	FLinearColor outColor;

	if (NewTeam == ETeam::BLUE_TEAM)
	{
		outColor = FLinearColor(0.0f, 0.0f, 0.5f);
	}
	else
	{
		outColor = FLinearColor(0.5f, 0.0f, 0.0f);
	}

	if (DynamicMat == nullptr)
	{
		DynamicMat = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);

		DynamicMat->SetVectorParameterValue(TEXT("BodyColor"), outColor);

		GetMesh()->SetMaterial(0, DynamicMat);
		FP_MESH->SetMaterial(0, DynamicMat);
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void ANetworkShooterCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ANetworkShooterCharacter::OnFire);

	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &ANetworkShooterCharacter::OnResetVR);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ANetworkShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ANetworkShooterCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ANetworkShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &ANetworkShooterCharacter::LookUpAtRate);
}

void ANetworkShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ANetworkShooterCharacter, CurrentTeam);
}

void ANetworkShooterCharacter::OnFire()
{
	// try and play a firing animation if specified
	if (FP_FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = FP_MESH->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FP_FireAnimation, 1.f);
		}
	}

	// Play the FP Particle effect if specified
	if (FP_GunShotParticle != nullptr)
	{
		FP_GunShotParticle->Activate(true);
	}

	FVector mousePos;
	FVector mouseDir;

	APlayerController* pController = Cast<APlayerController>(GetController());

	FVector2D ScreenPos = GEngine->GameViewport->Viewport->GetSizeXY();

	pController->DeprojectScreenPositionToWorld(ScreenPos.X / 2.0f,
		ScreenPos.Y / 2.0f,
		mousePos,
		mouseDir);

	mouseDir *= 10000000.0f;

	ServerFire(mousePos, mouseDir);
}

bool ANetworkShooterCharacter::ServerFire_Validate(const FVector pos, const FVector dir)
{
	if (pos != FVector(ForceInit) && dir != FVector(ForceInit))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void ANetworkShooterCharacter::ServerFire_Implementation(const FVector pos, const FVector dir)
{
	Fire(pos, dir);
	MultiCastShootEffects();
}

void ANetworkShooterCharacter::MultiCastShootEffects_Implementation()
{
	// try and play a firing animation if specified
	if (TP_FireAnimation != NULL)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(TP_FireAnimation, 1.f);
		}
	}

	// try and play the sound if specified
	if (FireSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	if (TP_GunShotParticle != nullptr)
	{
		TP_GunShotParticle->Activate(true);
	}

	if (BulletParticle != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BulletParticle->Template, BulletParticle->GetComponentLocation(),
			BulletParticle->GetComponentRotation());
	}
}

void ANetworkShooterCharacter::Fire(const FVector pos, const FVector dir)
{
	// Perform Raycast
	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);

	FCollisionQueryParams ColQuery;
	ColQuery.AddIgnoredActor(this);

	FHitResult HitRes;
	GetWorld()->LineTraceSingleByObjectType(HitRes, pos, dir, ObjQuery, ColQuery);

	DrawDebugLine(GetWorld(), pos, dir, FColor::Red, true, 100, 0, 5.0f);

	if (HitRes.bBlockingHit)
	{
		ANetworkShooterCharacter* OtherChar = Cast<ANetworkShooterCharacter>(HitRes.GetActor());

		if (OtherChar != nullptr && OtherChar->GetNetworkShooterPlayerState()->Team != this->GetNetworkShooterPlayerState()->Team)
		{
			FDamageEvent thisEvent(UDamageType::StaticClass());
			OtherChar->TakeDamage(10.0f, thisEvent, this->GetController(), this);

			APlayerController* thisPC = Cast<APlayerController>(GetController());

			FForceFeedbackParameters FeedbackParams;
			FeedbackParams.bLooping = false;
			FeedbackParams.Tag = NAME_None;

			thisPC->ClientPlayForceFeedback(HitSuccessFeedback, FeedbackParams);
		}
	}
}

float ANetworkShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (GetLocalRole() == ROLE_Authority &&
		DamageCauser != this &&
		NSPlayerState->Health > 0)
	{
		NSPlayerState->Health -= Damage;
		PlayPain();

		if (NSPlayerState->Health <= 0)
		{
			NSPlayerState->Deaths++;

			// Player has died time to respawn
			MultiCastRagdoll();

			ANetworkShooterCharacter* OtherChar = Cast<ANetworkShooterCharacter>(DamageCauser);

			if (OtherChar)
			{
				OtherChar->NSPlayerState->SetScore(OtherChar->NSPlayerState->GetScore() + 1.0f);
			}

			// After 3 seconds respawn
			FTimerHandle thisTimer;

			GetWorldTimerManager().SetTimer<ANetworkShooterCharacter>(thisTimer, this, &ANetworkShooterCharacter::Respawn, 3.0f, false);
		}
	}

	return Damage;
}

void ANetworkShooterCharacter::PlayPain_Implementation()
{
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PainSound, GetActorLocation());
	}
}

void ANetworkShooterCharacter::MultiCastRagdoll_Implementation()
{
	GetMesh()->SetPhysicsBlendWeight(1.0f);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName("Ragdoll");
}

void ANetworkShooterCharacter::Respawn()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// Get Location from game mode
		NSPlayerState->Health = 100.0f;
		Cast<ANetworkShooterGameMode>(GetWorld()->GetAuthGameMode())->Respawn(this);
		Destroy(true, true);
	}
}

void ANetworkShooterCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// NSPlayerState = Cast<ANetworkShooterPlayerState>(GetPlayerState());
	NSPlayerState = GetPlayerState<ANetworkShooterPlayerState>();

	if (GetLocalRole() == ROLE_Authority && NSPlayerState != nullptr)
	{
		NSPlayerState->Health = 100.0f;
	}
}

ANetworkShooterPlayerState* ANetworkShooterCharacter::GetNetworkShooterPlayerState()
{
	if (NSPlayerState)
	{
		return NSPlayerState;
	}
	else
	{
		// NSPlayerState = Cast<ANetworkShooterPlayerState>(GetPlayerState());
		NSPlayerState = GetPlayerState<ANetworkShooterPlayerState>();
		return NSPlayerState;
	}
}

void ANetworkShooterCharacter::SetNetworkShooterPlayerState(ANetworkShooterPlayerState* newPS)
{
	// Ensure PS is valid and only set on server
	if (newPS && GetLocalRole() == ROLE_Authority)
	{
		NSPlayerState = newPS;
		SetPlayerState(newPS);
	}
}

void ANetworkShooterCharacter::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

//Commenting this section out to be consistent with FPS BP template.
//This allows the user to turn without using the right virtual joystick

//void ANetworkShooterCharacter::TouchUpdate(const ETouchIndex::Type FingerIndex, const FVector Location)
//{
//	if ((TouchItem.bIsPressed == true) && (TouchItem.FingerIndex == FingerIndex))
//	{
//		if (TouchItem.bIsPressed)
//		{
//			if (GetWorld() != nullptr)
//			{
//				UGameViewportClient* ViewportClient = GetWorld()->GetGameViewport();
//				if (ViewportClient != nullptr)
//				{
//					FVector MoveDelta = Location - TouchItem.Location;
//					FVector2D ScreenSize;
//					ViewportClient->GetViewportSize(ScreenSize);
//					FVector2D ScaledDelta = FVector2D(MoveDelta.X, MoveDelta.Y) / ScreenSize;
//					if (FMath::Abs(ScaledDelta.X) >= 4.0 / ScreenSize.X)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.X * BaseTurnRate;
//						AddControllerYawInput(Value);
//					}
//					if (FMath::Abs(ScaledDelta.Y) >= 4.0 / ScreenSize.Y)
//					{
//						TouchItem.bMoved = true;
//						float Value = ScaledDelta.Y * BaseTurnRate;
//						AddControllerPitchInput(Value);
//					}
//					TouchItem.Location = Location;
//				}
//				TouchItem.Location = Location;
//			}
//		}
//	}
//}

void ANetworkShooterCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ANetworkShooterCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void ANetworkShooterCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ANetworkShooterCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}