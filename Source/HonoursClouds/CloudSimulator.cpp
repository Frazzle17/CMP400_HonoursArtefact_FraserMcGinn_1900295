// Fill out your copyright notice in the Description page of Project Settings.
#include "CloudSimulator.h"
#include "Engine/Texture2D.h"
#include "../../Plugins/Developer/RiderLink/Source/RD/thirdparty/clsocket/src/ActiveSocket.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ACloudSimulator::ACloudSimulator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//Create and set up test plane to show off texture
	PlaneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Test Plane"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> Mesh(TEXT("StaticMesh'/Engine/BasicShapes/Plane.Plane'"));
	PlaneMesh->SetStaticMesh(Mesh.Object);
	PlaneMesh->SetupAttachment(RootComponent);
	
	ConstructorHelpers::FObjectFinder<UMaterial> Mat(TEXT("Material'/Game/Simulation/TestMaterial.TestMaterial'"));
	CustomMaterial = Mat.Object;
}

// Called when the game starts or when spawned
void ACloudSimulator::BeginPlay()
{
	Super::BeginPlay();

	DynamicMaterial = UMaterialInstanceDynamic::Create(CustomMaterial, NULL);
	PlaneMesh->SetMaterial(0, DynamicMaterial);

	//Initialise empty cell for default population
	FCloudCellData empty_cell;
	empty_cell.velocity = FVector3f(0,0,0);
	empty_cell.water_vapor = 0.f;
	empty_cell.water_droplets = 0.f;
	
	FAdvectionData empty_advection;
	empty_advection.A_water_droplets = 0.f;
	empty_advection.A_water_vapor = 0.f;
	empty_cell.advection_data = empty_advection;
	
	F2DArray empty_2Darray;
	empty_2Darray.nested_array_2D.Init(empty_cell, z_sim_size);

	F3DArray empty_3Darray;
	empty_3Darray.nested_array_3D.Init(empty_2Darray, y_sim_size);
	
	cloud_lattice.Init(empty_3Darray, x_sim_size);

	//set default camera to free cam
	cameraID = 0;

	//set default type to cloud simulation
	sim_type = 0;

	//calculate the amount of time needed to process one cell if looped through whole lattice once within update_length
	per_length = update_length / (x_sim_size * y_sim_size * z_sim_size);
	//if doing cloud simulation divide per_length by 6 as must loop through lattice 6 times
	//then set stage to first simulation stage
	if(sim_type == 0)
	{
		per_length /= 6;
		currentStage = EStage::Velocity;
	}
	//if doing testing divide per_length by 2 as must loop through lattice twice (including render to texture)
	//then set stage to testing stage
	else
	{
		currentStage = EStage::Test;
		per_length /= 2;
	}
	ResetSim();
}

// Called every frame
void ACloudSimulator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//get player controller to detect key presses
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if(PlayerController)
	{
		//if player presses 1 key switch to free cam
		if(PlayerController->IsInputKeyDown(EKeys::One))
		{
			cameraID = 0;
		}
		//if player presses 2 key switch to fixed volumetric cloud cam
		if(PlayerController->IsInputKeyDown(EKeys::Two))
		{
			cameraID = 1;
		}
		//if player presses 3 key switch to fixed texture cam
		if(PlayerController->IsInputKeyDown(EKeys::Three))
		{
			cameraID = 2;
		}

		//if player presses Z key switch to cloud simulation
		if(PlayerController->IsInputKeyDown(EKeys::Z))
		{
			sim_type = 0;
			per_length = update_length / (x_sim_size * y_sim_size * z_sim_size * 6);
			ZeroLattice();
			AddFromVaporSource();
			currentStage = EStage::Velocity;
			update_timer = 0;
			ResetSim();
		}
		//if player presses X key switch to half and half testing
		if(PlayerController->IsInputKeyDown(EKeys::X))
		{
			sim_type = 1;
			per_length = update_length / (x_sim_size * y_sim_size * z_sim_size * 2);
			currentStage = EStage::Test;
			update_timer = 0;
			ResetSim();
		}
	}
	
	//if timer has reached the time step, reset the simulation and return from the function
	if(update_timer > update_length)
	{
		ResetSim();
		return;
	}
	//otherwise increase the timer by delta time
	update_timer += DeltaTime;

	//calculate how many cells to loop through this frame by dividing delta time by per length (then add 1 in case of values <0
	//only if the current stage isn't texture as this occurs in blueprints
	if(currentStage != EStage::Texture)
	{
		iteration_length = (DeltaTime / per_length) + 1;
	}

	//switch between sim and testing
	switch(sim_type)
	{
	default:
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString("Error in Sim Type switching."));
		break;
		
	case(0):
		//switch between current stage of simulation
		switch(currentStage)
		{
		default:
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString("Error in Stage switching."));
			break;

		case(EStage::Velocity):
			AlterVelocity(iteration_num);
			break;

		case(EStage::Diffuse):
			DiffuseWaterVapour(iteration_num);
			break;

		case(EStage::Advect1):
			Advection(iteration_num);
			break;

		case(EStage::Advect2):
			Advection(iteration_num);
			break;

		case(EStage::Transition):
			PhaseTransition(iteration_num);
			break;

		case(EStage::Texture):
			//occurs in blueprints
			break;
		}
		break;

	case(1):
		//do not call testing function while writing to texture
		if(currentStage != EStage::Texture)
		{
		    HalfandHalf(iteration_num);
		}
		break;
	}
}

//sets every cell in the lattice to a value of 0
void ACloudSimulator::ZeroLattice()
{
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			for(int z = 0; z < z_sim_size; z++)
			{
				cloud_lattice[x][y][z].velocity = FVector3f(0,0,0);
				cloud_lattice[x][y][z].water_droplets = 0.f;
				cloud_lattice[x][y][z].water_vapor = 0.f;
				cloud_lattice[x][y][z].advection_data.A_water_droplets = 0.f;
				cloud_lattice[x][y][z].advection_data.A_water_vapor = 0.f;
			}
		}
	}	
}

//reset the simulation by resetting the current iteration and the x, y, and z iterators
void ACloudSimulator::ResetSim()
{
	iteration_num = 0;
	current_x = 0;
	current_y = 0;
	current_z = 0;
}

//progress the simulation to the next step by increasing each iterator and looping back round to 0 for any overflows
//when z overflows that means the simulation step has ended, returning true instead of false
bool ACloudSimulator::ProgressSim()
{
	iteration_num++;
	
	current_x++;
	if(current_x >= x_sim_size)
	{
		current_x -= x_sim_size;

		current_y++;
		if(current_y >= y_sim_size)
		{
			current_y -= y_sim_size;

			current_z++;
			if(current_z >= z_sim_size)
			{
				ResetSim();
				return true;
			}
		}
	}
	return false;
}

//Tests to ensure writing to texture works
void ACloudSimulator::HalfandHalf(int iteration_start)
{
	//commented out code showing how this function operated before optimisation was included
	/*
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			for(int z = 0; z < z_sim_size; z++)
			{
				if(x > (x_sim_size / 2))
				{
					cloud_lattice[x][y][z].water_droplets = 1;
				}
				else
				{
					cloud_lattice[x][y][z].water_droplets = 0;
				}
			}
		}
	}
	*/
	
	while(iteration_num <= (iteration_start + iteration_length))
	{
		if(current_x > (x_sim_size / 2))
		{
			cloud_lattice[current_x][current_y][current_z].water_droplets = 1;
		}
		else
		{
			cloud_lattice[current_x][current_y][current_z].water_droplets = 0;
		}

		//progress simulation to next step, if simulation stage finished, progress to next stage and return from function
		if(ProgressSim())
		{
			currentStage = EStage::Texture;
			return;
		}
	}
}

//Adds water vapor into system from a vapor source
void ACloudSimulator::AddFromVaporSource()
{
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			cloud_lattice[x][y][0].water_vapor += 0.1;
		}
	} 
}

//Updates the local velocity of each cell based on viscosity and pressure effects
void ACloudSimulator::AlterVelocity(int iteration_start)
{
	//commented out code showing how this function operated before optimisation was included
	/*
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			for(int z = 0; z < z_sim_size; z++)
			{
				FVector3f cell_zminus = FVector3f(0,0,0);
				FVector3f cell_xminus_zplus = FVector3f(0,0,0);
				FVector3f cell_xplus_zminus = FVector3f(0,0,0);

				if(z > 0)
				{
					cell_zminus = cloud_lattice[x][y][z-1].velocity;
					if(x < x_sim_size-1)
					{
						cell_xplus_zminus = cloud_lattice[x+1][y][z-1].velocity;
					}
				}
				if(x > 0 && z < z_sim_size-1)
				{
					cell_xminus_zplus = cloud_lattice[x-1][y][z+1].velocity;
				}
				
				//V*(x,y,z) = V(x,y,z) + Kv[V(x,y,z-1) - 6V(x,y,z)] + Kp[-V(x-1,y,z+1) - V(x+1,y,z-1)]
				//Where: V* = velocity we're trying to calculate, V = current velocity, (x,y,z) = cell position in lattice, Kv = viscosity ratio, Kp = coefficient of pressure effect
				cloud_lattice[x][y][z].velocity = cloud_lattice[x][y][z].velocity + (K_viscosity_ratio * (cell_zminus) - (6 * cloud_lattice[x][y][z].velocity)) + (K_pressure_effect * ((-1 * cell_xminus_zplus) - cell_xplus_zminus));
			}
		}
	}
	*/

	while(iteration_num <= (iteration_start + iteration_length))
	{
		FVector3f cell_zminus = FVector3f(0,0,0);
		FVector3f cell_xminus_zplus = FVector3f(0,0,0);
		FVector3f cell_xplus_zminus = FVector3f(0,0,0);

		if(current_z > 0)
		{
			cell_zminus = cloud_lattice[current_x][current_y][current_z-1].velocity;
			if(current_x < x_sim_size-1)
			{
				cell_xplus_zminus = cloud_lattice[current_x+1][current_y][current_z-1].velocity;
			}
		}
		if(current_x > 0 && current_z < z_sim_size-1)
		{
			cell_xminus_zplus = cloud_lattice[current_x-1][current_y][current_z+1].velocity;
		}
				
		//V*(x,y,z) = V(x,y,z) + Kv[V(x,y,z-1) - 6V(x,y,z)] + Kp[-V(x-1,y,z+1) - V(x+1,y,z-1)]
		//Where: V* = velocity we're trying to calculate, V = current velocity, (x,y,z) = cell position in lattice, Kv = viscosity ratio, Kp = coefficient of pressure effect
		cloud_lattice[current_x][current_y][current_z].velocity = cloud_lattice[current_x][current_y][current_z].velocity + (K_viscosity_ratio * (cell_zminus) - (6 * cloud_lattice[current_x][current_y][current_z].velocity)) + (K_pressure_effect * ((-1 * cell_xminus_zplus) - cell_xplus_zminus));

		//progress simulation to next step, if simulation stage finished, progress to next stage and return from function
		if(ProgressSim())
		{
			currentStage = EStage::Diffuse;
			return;
		}
	}
}

//Updates the amount of water vapor in each cell based on diffusion rules
void ACloudSimulator::DiffuseWaterVapour(int iteration_start)
{
	//commented out code showing how this function operated before optimisation was included
	/*
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			for(int z = 0; z < z_sim_size; z++)
			{
				float zminus = 0.f;
				if(z > 0){zminus = cloud_lattice[x][y][z-1].water_vapor;}
				
				//Wv*(x,y,z) = Wv(x,y,z) + Kdw[Wv(x,y,z) - 6Wv(x,y,z)]
				//Where: Wv* = water vapor we're trying to calculate, Wv = current water vapor, (x,y,z) = cell position in lattice, Kdw = coefficient of water vapor diffusion
				cloud_lattice[x][y][z].water_vapor = cloud_lattice[x][y][z].water_vapor + (K_water_vapour_diffusion * (zminus) - (6 * cloud_lattice[x][y][z].water_vapor));
			}
		}
	}
	*/

	while(iteration_num <= (iteration_start + iteration_length))
	{
		float zminus = 0.f;
		if(current_z > 0){zminus = cloud_lattice[current_x][current_y][current_z-1].water_vapor;}
				
		//Wv*(x,y,z) = Wv(x,y,z) + Kdw[Wv(x,y,z) - 6Wv(x,y,z)]
		//Where: Wv* = water vapor we're trying to calculate, Wv = current water vapor, (x,y,z) = cell position in lattice, Kdw = coefficient of water vapor diffusion
		cloud_lattice[current_x][current_y][current_z].water_vapor = cloud_lattice[current_x][current_y][current_z].water_vapor + (K_water_vapour_diffusion * (zminus) - (6 * cloud_lattice[current_x][current_y][current_z].water_vapor));

		//progress simulation to next step, if simulation stage finished, progress to next stage and return from function
		if(ProgressSim())
		{
			currentStage = EStage::Advect1;
			return;
		}
	}
}

//moves values of each cell to a different cell based on local velocity
void ACloudSimulator::Advection(int iteration_start)
{
	//commented out code showing how this function operated before optimisation was included
	/*
	//loop through lattice to calculate advection data for current time step
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			for(int z = 0; z < z_sim_size; z++)
			{
				//l, m, and n are the x, y and z integer portions of velocity
				int l = (int)cloud_lattice[x][y][z].velocity.X;
				int m = (int)cloud_lattice[x][y][z].velocity.Y;
				int n = (int)cloud_lattice[x][y][z].velocity.Z;

				if((l > 0 && l < x_sim_size-1) && (m > 0 && m < y_sim_size-1) && (n > 0 && n < z_sim_size-1))
				{
					//weightX, weightY and weightZ are the x, y and z fractional portions of velocity
					float weightX = cloud_lattice[x][y][z].velocity.X - l;
					float weightY = cloud_lattice[x][y][z].velocity.X - m;
					float weightZ = cloud_lattice[x][y][z].velocity.X - n;

					//Add cell values to adjacent cells weighted based on velocity
					cloud_lattice[l][m][n].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * ((1 - weightX) * (1 - weightY) * (1 - weightZ));
					cloud_lattice[l][m][n].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * ((1 - weightX) * (1 - weightY) * (1 - weightZ));

					cloud_lattice[l+1][m][n].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * (weightX * (1 - weightY) * (1 - weightZ));
					cloud_lattice[l+1][m][n].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * (weightX * (1 - weightY) * (1 - weightZ));

					cloud_lattice[l][m+1][n].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * ((1 - weightX) * weightY * (1 - weightZ));
					cloud_lattice[l][m+1][n].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * ((1 - weightX) * weightY * (1 - weightZ));

					cloud_lattice[l][m][n+1].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * ((1 - weightX) * (1 - weightY) * weightZ);
					cloud_lattice[l][m][n+1].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * ((1 - weightX) * (1 - weightY) * weightZ);

					cloud_lattice[l+1][m+1][n].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * (weightX * weightY * (1 - weightZ));
					cloud_lattice[l+1][m+1][n].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * (weightX * weightY * (1 - weightZ));

					cloud_lattice[l+1][m][n+1].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * ((1 - weightX) * weightY * weightZ);
					cloud_lattice[l+1][m][n+1].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * ((1 - weightX) * weightY * weightZ);

					cloud_lattice[l][m+1][n+1].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * (weightX * (1 - weightY) * weightZ);
					cloud_lattice[l][m+1][n+1].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * (weightX * (1 - weightY) * weightZ);

					cloud_lattice[l+1][m+1][n+1].advection_data.A_water_vapor += cloud_lattice[x][y][z].water_vapor * (weightX * weightY * weightZ);
					cloud_lattice[l+1][m+1][n+1].advection_data.A_water_droplets += cloud_lattice[x][y][z].water_droplets * (weightX * weightY * weightZ);
				}
			}
		}
	}

	//loop through the lattice a second time to add advection data
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			for(int z = 0; z < z_sim_size; z++)
			{
				//add advection data onto current data then zero advection data
				cloud_lattice[x][y][z].water_vapor += cloud_lattice[x][y][z].advection_data.A_water_vapor;
				cloud_lattice[x][y][z].advection_data.A_water_vapor = 0.f;
				cloud_lattice[x][y][z].water_droplets += cloud_lattice[x][y][z].advection_data.A_water_droplets;
				cloud_lattice[x][y][z].advection_data.A_water_droplets = 0.f;
			}
		}
	}
	*/

	//define variables outside of switch statement to avoid errors
	int l = 0;
	int m = 0;
	int n = 0;

	float weightX = 0;
	float weightY = 0;
	float weightZ = 0;
	
	while(iteration_num <= (iteration_start + iteration_length))
	{
		switch(currentStage)
		{
		default:
			GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, FString("Error in Advection Stage."));
			break;

		case(EStage::Advect1):
			//l, m, and n are the x, y and z integer portions of velocity
			l = (int)cloud_lattice[current_x][current_y][current_z].velocity.X;
			m = (int)cloud_lattice[current_x][current_y][current_z].velocity.Y;
			n = (int)cloud_lattice[current_x][current_y][current_z].velocity.Z;

			if((l > 0 && l < x_sim_size-1) && (m > 0 && m < y_sim_size-1) && (n > 0 && n < z_sim_size-1))
			{
				//weightX, weightY and weightZ are the x, y and z fractional portions of velocity
				weightX = cloud_lattice[current_x][current_y][current_z].velocity.X - l;
				weightY = cloud_lattice[current_x][current_y][current_z].velocity.X - m;
				weightZ = cloud_lattice[current_x][current_y][current_z].velocity.X - n;

				//Add cell values to adjacent cells weighted based on velocity
				cloud_lattice[l][m][n].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * ((1 - weightX) * (1 - weightY) * (1 - weightZ));
				cloud_lattice[l][m][n].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * ((1 - weightX) * (1 - weightY) * (1 - weightZ));

				cloud_lattice[l+1][m][n].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * (weightX * (1 - weightY) * (1 - weightZ));
				cloud_lattice[l+1][m][n].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * (weightX * (1 - weightY) * (1 - weightZ));

				cloud_lattice[l][m+1][n].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * ((1 - weightX) * weightY * (1 - weightZ));
				cloud_lattice[l][m+1][n].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * ((1 - weightX) * weightY * (1 - weightZ));

				cloud_lattice[l][m][n+1].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * ((1 - weightX) * (1 - weightY) * weightZ);
				cloud_lattice[l][m][n+1].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * ((1 - weightX) * (1 - weightY) * weightZ);

				cloud_lattice[l+1][m+1][n].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * (weightX * weightY * (1 - weightZ));
				cloud_lattice[l+1][m+1][n].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * (weightX * weightY * (1 - weightZ));

				cloud_lattice[l+1][m][n+1].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * ((1 - weightX) * weightY * weightZ);
				cloud_lattice[l+1][m][n+1].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * ((1 - weightX) * weightY * weightZ);

				cloud_lattice[l][m+1][n+1].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * (weightX * (1 - weightY) * weightZ);
				cloud_lattice[l][m+1][n+1].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * (weightX * (1 - weightY) * weightZ);

				cloud_lattice[l+1][m+1][n+1].advection_data.A_water_vapor += cloud_lattice[current_x][current_y][current_z].water_vapor * (weightX * weightY * weightZ);
				cloud_lattice[l+1][m+1][n+1].advection_data.A_water_droplets += cloud_lattice[current_x][current_y][current_z].water_droplets * (weightX * weightY * weightZ);
			}

			//progress simulation to next step, if simulation stage finished, progress to next stage and return from function
			if(ProgressSim())
			{
				currentStage = EStage::Advect2;
				return;
			}
			break;

		case(EStage::Advect2):
			//add advection data onto current data then zero advection data
			cloud_lattice[current_x][current_y][current_z].water_vapor += cloud_lattice[current_x][current_y][current_z].advection_data.A_water_vapor;
			cloud_lattice[current_x][current_y][current_z].advection_data.A_water_vapor = 0.f;
			cloud_lattice[current_x][current_y][current_z].water_droplets += cloud_lattice[current_x][current_y][current_z].advection_data.A_water_droplets;
			cloud_lattice[current_x][current_y][current_z].advection_data.A_water_droplets = 0.f;

			//progress simulation to next step, if simulation stage finished, progress to next stage and return from function
			if(ProgressSim())
			{
				currentStage = EStage::Transition;
				return;
			}
			break;
		}
	}
}

//turns water vapour into water droplets based on phase transition rules (condensation/evaporation), and adjusts other variables accordingly
void ACloudSimulator::PhaseTransition(int iteration_start)
{
	//commented out code showing how this function operated before optimisation was included
	/*
	float avg_max = 0;
	for(int x = 0; x < x_sim_size; x++)
	{
		for(int y = 0; y < y_sim_size; y++)
		{
			for(int z = 0; z < z_sim_size; z++)
			{
				//temperature at surface level is ~300K and decreases by 0.6K every 100m up
				//calculate meter length of each cell (z / z_sim_size) * z_world_size
				//divide by 100 and multiply by 0.6 to determine how much the temperature has decreased
				//take away from 300 to determine current temperature at this cell
				float temperature = 300 - ((((z / z_sim_size) * z_world_size) / 100) * 0.6);
				
				//w_max = 217.0 * exp[19.482 - 4303.4 / (T-29.5)] / T
				//Where: w_max = max amount of water vapor in cell, T = cell temperature
				float w_max = (217 * exp((19.482 - (4303.4/(temperature - 29.5))))) / temperature;

				//Wl* = Wl + a(Wv - w_max)
				//Where: Wl* = new amount of water droplets, Wl = current amount of water droplets, a = phase transition rate constant, Wv = current amount of water vapor
				cloud_lattice[x][y][z].water_droplets = cloud_lattice[x][y][z].water_droplets + (phase_transition_rate * (cloud_lattice[x][y][z].water_vapor - w_max));

				avg_max += cloud_lattice[x][y][z].water_droplets;
				if(cloud_lattice[x][y][z].water_droplets > max_droplets)
				{
					max_droplets = cloud_lattice[x][y][z].water_droplets;
				}

				//Wv* = Wv - a(Wv - w_max)
				//Where: Wv* = new amount of water vapor, Wv = current amount of water vapor, a = phase transition rate constant
				cloud_lattice[x][y][z].water_vapor = cloud_lattice[x][y][z].water_vapor - (phase_transition_rate * (cloud_lattice[x][y][z].water_vapor - w_max));
			}
		}
	}
	avg_max /= (x_sim_size * y_sim_size * z_sim_size);
	FString text = std::to_string(avg_max).c_str();
	//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, text);
	*/

	while(iteration_num <= (iteration_start + iteration_length))
	{
		//temperature at surface level is ~300K and decreases by 0.6K every 100m up
		//calculate meter length of each cell (z / z_sim_size) * z_world_size
		//divide by 100 and multiply by 0.6 to determine how much the temperature has decreased
		//take away from 300 to determine current temperature at this cell
		float temperature = 300 - ((((current_z / z_sim_size) * z_world_size) / 100) * 0.6);
				
		//w_max = 217.0 * exp[19.482 - 4303.4 / (T-29.5)] / T
		//Where: w_max = max amount of water vapor in cell, T = cell temperature
		float w_max = (217 * exp((19.482 - (4303.4/(temperature - 29.5))))) / temperature;

		//Wl* = Wl + a(Wv - w_max)
		//Where: Wl* = new amount of water droplets, Wl = current amount of water droplets, a = phase transition rate constant, Wv = current amount of water vapor
		cloud_lattice[current_x][current_y][current_z].water_droplets = cloud_lattice[current_x][current_y][current_z].water_droplets + (phase_transition_rate * (cloud_lattice[current_x][current_y][current_z].water_vapor - w_max));

		//Wv* = Wv - a(Wv - w_max)
		//Where: Wv* = new amount of water vapor, Wv = current amount of water vapor, a = phase transition rate constant
		cloud_lattice[current_x][current_y][current_z].water_vapor = cloud_lattice[current_x][current_y][current_z].water_vapor - (phase_transition_rate * (cloud_lattice[current_x][current_y][current_z].water_vapor - w_max));

		//progress simulation to next step, if simulation stage finished, progress to next stage and return from function
		if(ProgressSim())
		{
			currentStage = EStage::Texture;
			return;
		}
	}
}