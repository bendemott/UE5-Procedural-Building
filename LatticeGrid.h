// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UDynamicMesh.h"
#include "BuildingEnums.h"
#include "LatticeGrid.generated.h"

UENUM(BlueprintType)
enum class ELatticeMaterialSlots : uint8
{
	All,
	Framing_All,
	Framing_Horizontal,
	Framing_Vertical,
	Border_All,
	Border_Horizontal,
	Border_Vertical
};

USTRUCT(BlueprintType)
struct PROCEDURALBUILDINGS_API FLatticeGridOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bUseLocalSeed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Random Seed", ToolTip = "Randomness Seed", EditCondition = "bUseLocalSeed"))
	int32 RandomSeed = 1;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bHasRows = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Lattice Rows", ToolTip = "Number of Lattice Rows (Horizontal),  Setting to 0 will fit as many as possible", EditCondition = "bHasRows"))
	int32 Rows = 0;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bHasColumns = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Lattice Columns", ToolTip = "Number of Lattice Columns (Vertical), Setting to 0 will fit as many as possible", EditCondition = "bHasColumns"))
	int32 Columns = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Size", ToolTip = "Size of Rows,Columns,Depth in cm", ClampMin = 1, UIMax = 100000, Unit = "Centimeter"))
	FVector Size = FVector(50.f, 100.f, 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Size Variance", ToolTip = "Size of Rows,Columns,Depth variance in cm", Unit = "Centimeter"))
	FVector SizeVariance = FVector(0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Spacing", ToolTip = "Distance between the edge of each Row/Column", ClampMin = 1, Unit = "Centimeter"))
	FVector2D Spacing = FVector2D(500.f, 450.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Spacing Variance", ToolTip = "Randomness in distance between edge of each row/column", Unit = "Centimeter"))
	FVector2D SpacingVariance = FVector2D(0.f);

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bHasBorder = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Border Size", ToolTip = "Randomness in distance between edge of each row/column", EditCondition = "bHasBorder", Unit = "Centimeter"))
	FVector BorderSize = FVector(50.f, 30.f, 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Materials", ToolTip = "Materials to apply"))
	TMap<ELatticeMaterialSlots, UMaterialInterface*> MaterialSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UV Scale Mode", ToolTip = "UV Scaling Mode"))
	EBuildingUVScaleMode UVScaleMode = EBuildingUVScaleMode::Fixed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UV Size", ToolTip = "UV Scale Size - The UV coordinates will repeat after this distance on the mesh", Unit = "Centimeter", EditCondition = "UVScaleMode == EBuildingUVScaleMode::Fixed"))
	float UVSize = 1000.f; // 10 meters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UV Origin Mode", ToolTip = "UV Origin - Detemrines where on the mesh the center of the UV coordinates will be, setting this to random will create a non-uniform appearance in the exture layout"))
	EBuildingUVOriginMode UVOriginMode = EBuildingUVOriginMode::MinCoordinate;
};

/**
 * 
 */
class PROCEDURALBUILDINGS_API LatticeGrid
{
public:
	static const int32 RANDOM_OFFSET = 972959;
	static const int32 MAX_LATTICES = 100;

	LatticeGrid();
	LatticeGrid(FLatticeGridOptions* Options);
	void ApplyLattice(UDynamicMesh* Mesh, TArray<UMaterialInterface*>& MaterialSet);
	~LatticeGrid();

private:
	FLatticeGridOptions* Options;

	// TODO move to static utility function
	FTransform GetMeshUVTransform(FBox& MeshBounds, EBuildingUVScaleMode ScaleMode, EBuildingUVOriginMode OriginMode, FRandomStream* Randomizer = nullptr, float UVSize = 0.f);

};
