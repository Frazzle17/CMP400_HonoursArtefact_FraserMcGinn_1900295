// Fill out your copyright notice in the Description page of Project Settings.


#include "CloudSimLatticeTypeTesting.h"

// Sets default values
ACloudSimLatticeTypeTesting::ACloudSimLatticeTypeTesting()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ACloudSimLatticeTypeTesting::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ACloudSimLatticeTypeTesting::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	PopulateArray();
	TransitionArray();

	//PopulateVector();
	//TransitionVector();
	//Vlattice.clear();

	//PopulateList();
	//TransitionList();
	//Llattice.clear();
}

void ACloudSimLatticeTypeTesting::PopulateArray()
{
	{
		SCOPE_CYCLE_COUNTER(STAT_PopulateArray1);
		for(int x = 0; x < 10; x++)
		{
			for(int y = 0; y < 10; y++)
			{
				for(int z = 0; z < 10; z++)
				{
					Alattice[x][y][z] = rand() % 1000;
				}
			}	
		}
	}
}

void ACloudSimLatticeTypeTesting::TransitionArray()
{
	{
		SCOPE_CYCLE_COUNTER(STAT_TransitionArray1);
		for(int x = 0; x < 10; x++)
		{
			for(int y = 0; y < 10; y++)
			{
				for(int z = 0; z < 10; z++)
				{
					Alattice[x][y][z] = (Alattice[x + 1][y][z] / 6) + (Alattice[x - 1][y][z] / 6) + (Alattice[x][y + 1][z] / 6) + (Alattice[x][y - 1][z] / 6) + (Alattice[x][y][z + 1] / 6) + (Alattice[x][y][z - 1] / 6);
				}
			}	
		}
	}
}

void ACloudSimLatticeTypeTesting::PopulateVector()
{
	{
		SCOPE_CYCLE_COUNTER(STAT_PopulateVector1);
		for(int x = 0; x < x_size; x++)
		{
			Vlattice.push_back(std::vector<std::vector<float>>(y_size));
			for(int y = 0; y < y_size; y++)
			{
				Vlattice[x].push_back(std::vector<float>(z_size));
				for(int z = 0; z < z_size; z++)
				{
					Vlattice[x][y].push_back(rand() % 1000);
				}
			}
		}
	}
}

void ACloudSimLatticeTypeTesting::TransitionVector()
{
	{
		SCOPE_CYCLE_COUNTER(STAT_TransitionVector1);
		int x_minus = 0;
		int x_plus = 0;
		int y_minus = 0;
		int y_plus = 0;
		int z_minus = 0;
		int z_plus = 0;
	
		for(int x = 0; x < x_size; x++)
		{
			x_minus = x - 1;
			x_plus = x + 1;
			if (x == 0)
			{
				x_minus = 0;
			}
			if (x == x_size - 1)
			{
				x_plus = x_size - 1;
			}
			for(int y = 0; y < y_size; y++)
			{
				y_minus = y - 1;
				y_plus = y + 1;
				if (y == 0)
				{
					y_minus = 0;
				}
				if (y == y_size - 1)
				{
					y_plus = y_size - 1;
				}
				for(int z = 0; z < z_size; z++)
				{
					z_minus = z - 1;
					z_plus = z + 1;
					if (z == 0)
					{
						z_minus = 0;
					}
					if (z == z_size - 1)
					{
						z_plus = z_size - 1;
					}
					Vlattice[x][y][z] = (Vlattice[x_plus][y][z] / 6) + (Vlattice[x_minus][y][z] / 6) + (Vlattice[x][y_plus][z] / 6) + (Vlattice[x][y_minus][z] / 6) + (Vlattice[x][y][z_plus] / 6) + (Vlattice[x][y][z_minus] / 6);
				}
			}	
		}
	}
}

void ACloudSimLatticeTypeTesting::PopulateList()
{
	{
		SCOPE_CYCLE_COUNTER(STAT_PopulateList1);
		for(int x = 0; x < x_size; x++)
		{
			Llattice.push_back(std::list<std::list<float>>(y_size));
			std::list<std::list<std::list<float>>>::iterator x_iterator = Llattice.begin();
			std::advance(x_iterator, x);
			for(int y = 0; y < y_size; y++)
			{
				(*(x_iterator)).push_back(std::list<float>(z_size));
				std::list<std::list<float>>::iterator y_iterator = (*(x_iterator)).begin();
				std::advance(y_iterator, y);
				for(int z = 0; z < z_size; z++)
				{
					(*(y_iterator)).push_back(rand() % 1000);
				}
			}
		}
	}
}

void ACloudSimLatticeTypeTesting::TransitionList()
{
	{
		SCOPE_CYCLE_COUNTER(STAT_TransitionList1);
		std::list<std::list<std::list<float>>>::iterator x_plus_access1;
		std::list<std::list<std::list<float>>>::iterator x_minus_access1;
		std::list<std::list<float>>::iterator x_plus_access2;
		std::list<std::list<float>>::iterator x_minus_access2;
		std::list<std::list<float>>::iterator y_plus_access;
		std::list<std::list<float>>::iterator y_minus_access;
		std::list<float>::iterator x_plus;
		std::list<float>::iterator x_minus;
		std::list<float>::iterator y_plus;
		std::list<float>::iterator y_minus;
		std::list<float>::iterator z_plus;
		std::list<float>::iterator z_minus;
	
		for(int x = 0; x < x_size; x++)
		{
			std::list<std::list<std::list<float>>>::iterator x_iterator = Llattice.begin();
			std::advance(x_iterator, x);

			x_plus_access1 = x_iterator;
			x_minus_access1 = x_iterator;
			if(x > 0)
			{
				std::advance(x_minus_access1, -1);
			}
			if(x < x_size - 1)
			{
				std::advance(x_plus_access1, 1);
			}
		
			for(int y = 0; y < y_size; y++)
			{
				std::list<std::list<float>>::iterator y_iterator = (*(x_iterator)).begin();
				std::advance(y_iterator, y);

				x_plus_access2 = (*(x_plus_access1)).begin();
				std::advance(x_plus_access2, y);
				x_minus_access2 = (*(x_minus_access1)).begin();
				std::advance(x_minus_access2, y);

				y_plus_access = y_iterator;
				y_minus_access = y_iterator;
				if(y > 0)
				{
					std::advance(y_minus_access, -1);
				}
				if(y < y_size - 1)
				{
					std::advance(y_plus_access, 1);
				}
			
				for(int z = 0; z < z_size; z++)
				{
					std::list<float>::iterator z_iterator = (*(y_iterator)).begin();
					std::advance(z_iterator, z);

					x_plus = (*(x_plus_access2)).begin();
					std::advance(x_plus, z);
					x_minus = (*(x_minus_access2)).begin();
					std::advance(x_minus, z);

					y_plus = (*(y_plus_access)).begin();
					std::advance(y_plus, z);
					y_minus = (*(y_minus_access)).begin();
					std::advance(y_minus, z);

					z_plus = z_iterator;
					z_minus = z_iterator;
					if(z > 0)
					{
						std::advance(z_minus, -1);
					}
					if(z < z_size - 1)
					{
						std::advance(z_plus, 1);
					}

					(*(z_iterator)) = ((*(x_plus)) / 6) + ((*(x_minus)) / 6) + ((*(y_plus)) / 6) + ((*(y_minus)) / 6) + ((*(z_plus)) / 6) + ((*(z_minus)) / 6);
				}
			}
		}
	}
}