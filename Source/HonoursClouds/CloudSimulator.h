// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IntVectorTypes.h"
#include "Camera/CameraActor.h"
#include "Components/VolumetricCloudComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/Texture2D.h"
#include "CloudSimulator.generated.h"

//struct to store advection data
USTRUCT(BlueprintType)
struct FAdvectionData
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	float A_water_vapor;
	
	UPROPERTY(BlueprintReadWrite)
	float A_water_droplets;
};

//struct to store cell data
USTRUCT(BlueprintType)
struct FCloudCellData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector3f velocity;
	
	UPROPERTY(BlueprintReadWrite)
	float water_vapor;
	
	UPROPERTY(BlueprintReadWrite)
	float water_droplets;
	
	UPROPERTY(BlueprintReadWrite)
	FAdvectionData advection_data;
};

//struct to store an array to create a 2D array
USTRUCT(BlueprintType)
struct F2DArray
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<FCloudCellData> nested_array_2D;

	FCloudCellData& operator[] (int32 i)
	{
		return nested_array_2D[i];
	}

	void Add(FCloudCellData cell_data)
	{
		nested_array_2D.Add(cell_data);
	}
};

//struct to store a 2D array to create a 3D array
USTRUCT(BlueprintType)
struct F3DArray
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<F2DArray> nested_array_3D;

	F2DArray& operator[] (int32 i)
	{
		return nested_array_3D[i];
	}

	void Add(F2DArray nested_array)
	{
		nested_array_3D.Add(nested_array);
	}
};

//enum for every stage
UENUM(BlueprintType)
enum EStage
{
	Velocity UMETA(DisplayName = "Velocity"),
	Diffuse UMETA(DisplayName = "Diffuse"),
	Advect1 UMETA(DisplayName = "Advect1"),
	Advect2 UMETA(DisplayName = "Advect2"),
	Transition UMETA(DisplayName = "Transition"),
	Test UMETA(DisplayName = "Test"),
	Texture UMETA(DisplayName = "Texture")
};

UCLASS()
class HONOURSCLOUDS_API ACloudSimulator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACloudSimulator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	//Test Functions / Variables
	UFUNCTION(BlueprintCallable)
	void ResetSim();

	UFUNCTION(BlueprintCallable)
	bool ProgressSim();
	
	UFUNCTION(BlueprintCallable)
	void HalfandHalf(int iteration_start);

	UPROPERTY(BlueprintReadWrite)
	int currentHalf;

	UFUNCTION(BlueprintCallable)
	void DifferentDensities(int iteration_start);
	
	UPROPERTY(BlueprintReadWrite)
	int cameraID; //0=default, 1=cloud camera, 2=texture camera

	UPROPERTY(BlueprintReadWrite)
	int sim_type; //0=default(broken), 1=half and half test, 2=?
	
	//Simulation Functions
	UFUNCTION(BlueprintCallable)
	void ZeroLattice();
	
	UFUNCTION(BlueprintCallable)
	void AddFromVaporSource();
	
	UFUNCTION(BlueprintCallable)
	void AlterVelocity(int iteration_start);

	UFUNCTION(BlueprintCallable)
	void DiffuseWaterVapour(int iteration_start);

	UFUNCTION(BlueprintCallable)
	void Advection(int iteration_start);
	
	UFUNCTION(BlueprintCallable)
	void PhaseTransition(int iteration_start);

	//number of cells in lattice
	UPROPERTY(BlueprintReadWrite)
	int x_sim_size = 50;

	UPROPERTY(BlueprintReadWrite)
	int y_sim_size = 50;

	UPROPERTY(BlueprintReadWrite)
	int z_sim_size = 16;

	//real world size simulation space takes up
	float x_world_size = 1000.f;
	float y_world_size = 1000.f;
	float z_world_size = 1000.f;

	//3D vector lattice
	UPROPERTY(BlueprintReadWrite)
	TArray<F3DArray> cloud_lattice;

	//constant coefficients
	float K_viscosity_ratio = 2.4 * (10^-5);
	float K_pressure_effect = 0.1;
	float K_water_vapour_diffusion = 0.5;
	float phase_transition_rate = 100;

	//optimisation variables
	UPROPERTY(BlueprintReadWrite)
	TEnumAsByte<EStage> currentStage;
	
	UPROPERTY(BlueprintReadWrite)
	float update_length = 10.f;

	UPROPERTY(BlueprintReadWrite)
	float update_timer = 0.f;

	UPROPERTY(BlueprintReadWrite)
	float per_length = 0.f;
	
	UPROPERTY(BlueprintReadWrite)
	int iteration_num = 0;

	UPROPERTY(BlueprintReadWrite)
	int iteration_length = 0;

	UPROPERTY(BlueprintReadWrite)
	int current_x = 0;
	UPROPERTY(BlueprintReadWrite)
	int current_y = 0;
	UPROPERTY(BlueprintReadWrite)
	int current_z = 0;

	//plane texture render variables
	UTexture2D* CustomTexture;
	UMaterial* CustomMaterial;
	UMaterialInstanceDynamic* DynamicMaterial;
	UStaticMeshComponent* PlaneMesh;
};
