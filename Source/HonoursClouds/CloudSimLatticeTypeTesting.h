// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CloudSimLatticeTypeTesting.generated.h"

DECLARE_STATS_GROUP(TEXT("Cloud_Simulator_LatticeTest"), STATGROUP_LatticeTest, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("LatticeTest - PopulateArray"), STAT_PopulateArray1, STATGROUP_LatticeTest);
DECLARE_CYCLE_STAT(TEXT("LatticeTest - TransitionArray"), STAT_TransitionArray1, STATGROUP_LatticeTest);
DECLARE_CYCLE_STAT(TEXT("LatticeTest - PopulateVector"), STAT_PopulateVector1, STATGROUP_LatticeTest);
DECLARE_CYCLE_STAT(TEXT("LatticeTest - TransitionVector"), STAT_TransitionVector1, STATGROUP_LatticeTest);
DECLARE_CYCLE_STAT(TEXT("LatticeTest - PopulateList"), STAT_PopulateList1, STATGROUP_LatticeTest);
DECLARE_CYCLE_STAT(TEXT("LatticeTest - TransitionList"), STAT_TransitionList1, STATGROUP_LatticeTest);

UCLASS()
class HONOURSCLOUDS_API ACloudSimLatticeTypeTesting : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ACloudSimLatticeTypeTesting();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	int x_size = 100;
	int y_size = 100;
	int z_size = 100;
	
	UFUNCTION(BlueprintCallable)
	void PopulateArray();
	
	UFUNCTION(BlueprintCallable)
	void TransitionArray();

	UFUNCTION(BlueprintCallable)
	void PopulateVector();
	
	UFUNCTION(BlueprintCallable)
	void TransitionVector();
	
	UFUNCTION(BlueprintCallable)
	void PopulateList();
	
	UFUNCTION(BlueprintCallable)
	void TransitionList();
	
	float Alattice[200][200][200];
	
	std::vector<std::vector<std::vector<float>>> Vlattice;
	
	std::list<std::list<std::list<float>>> Llattice;
};
