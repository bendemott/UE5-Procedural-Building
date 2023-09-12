

#pragma once

/*
UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle))
bool bCanFly;

UPROPERTY(EditAnywhere, meta=(EditCondition="bCanFly", Units="s"))
float FlapPeriodSeconds;


Units="Percent")
meta=(UIMin=0, UIMax=100)
ClampMin = 1, ClampMax = 100
*/

#include "CoreMinimal.h"
#include "DynamicMeshActor.h"
#include "Containers/Array.h"
#include "BooleanGrid.h"
#include "UDynamicMesh.h"
#include "BuildingEnums.h"
#include "LatticeGrid.h"
#include "DynamicBuilding.generated.h"


UENUM(BlueprintType)
enum class EBuildingMaterialSlots : uint8
{
	All,
	Top_Bottom,
	All_Sides
};

UENUM(BlueprintType)
enum class EBuildingBoxMaterialSlots : uint8
{
	All,
	Top_Bottom,
	All_Sides
};

USTRUCT(BlueprintType)
struct PROCEDURALBUILDINGS_API FSizeAndTransform
{
	GENERATED_BODY()

	FVector Size = FVector::Zero();
	FTransform Transform;
};

USTRUCT(BlueprintType)
struct PROCEDURALBUILDINGS_API FDynamicBuildingGenericBoxOptions
{
	GENERATED_BODY()

public:
	//////////////////// MATERIALS ////////////////////////////

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Materials", ToolTip = "Materials to apply"))
	TMap<EBuildingBoxMaterialSlots, UMaterialInterface*> MaterialSlots;

	//////////////////// UV ///////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UV Scale Mode", ToolTip = "UV Scaling Mode"))
		EBuildingUVScaleMode UVScaleMode = EBuildingUVScaleMode::Fixed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UV Size", ToolTip = "UV Scale Size - The UV coordinates will repeat after this distance on the mesh", Unit = "Centimeter", EditCondition = "UVScaleMode == EBuildingUVScaleMode::Fixed"))
	float UVSize = 2000.f; // 20 meters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "UV Origin Mode", ToolTip = "UV Origin - Detemrines where on the mesh the center of the UV coordinates will be, setting this to random will create a non-uniform appearance in the exture layout"))
	EBuildingUVOriginMode UVOriginMode = EBuildingUVOriginMode::MinCoordinate;

	//////////////////// SIZE ///////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Scale", ToolTip = "The Scale of the Boxes as compared to the main building", Units = "Percent", UIMin = 50, UIMax = 150))
	FVector2D Scale = FVector2D(110);

	UPROPERTY(EditAnywhere, BlueprintReadWrite,  meta = (DisplayName = "Vertical Spacing", ToolTip = "The space vertically between boxes in Meters", UIMin = 1, UIMax = 100))
	int32 VerticalSpacing = 700;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Floor Height", ToolTip = "The height of a single floor/story", UIMin = "2.5", UIMax = "10"))
	float FloorHeight = 400.25;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Number of Floors", ToolTip = "The number of floors within each box, if a range is given, values will randomly be chosen, this determines box height", UIMin = 1, UIMax = 15))
	int32 NumFloors = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Number of Floors Variance", ToolTip = "The number of floors will vary by +/- this amount"))
	int32 NumFloorsVariance = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Box Rotation", ToolTip = "Specify if boxes should be rotated to face a different orientation, unit is in degrees", ClampMin = "0", ClampMax = "359", Units = "Degrees"))
	float ZRotation = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Randomize Rotation Increments", ToolTip = "Rotation will randomize given the increments provided by 'Box Rotation'"))
	bool RotationRandomizeInIncrements = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Randomize Rotation Set", ToolTip = "Rotation will randomize by picking one of the values given", ClampMin = "0", ClampMax = "359", EditCondition = "!RotationRandomizeInIncrements"))
	TSet<float> RotationRandomizeFromSet;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bExplicitNumberOfBoxes = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Number of Boxes", ToolTip = "Overrides the default calculation to customize the number of boxes to create", UIMin = 1, UIMax = 1000, Units = "times", EditCondition = "bExplicitNumberOfBoxes"))
	int32 NumberOfBoxes = 1;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bVaryBoxSizePercent = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Size Variance", ToolTip = "When enabled, each box will vary from the base scale randomly by up to the given percentages in each direction", EditCondition = "bVaryBoxSizePercent", Units = "Percent"))
	FVector2D VarySizePercent = FVector2D(0);

	//////////////////// ALIGNMENT / POSITION /////////////////////////////
	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bSpecifyVAlignment = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Vertical Alignment", ToolTip = "Customize the alignment of the boxes on the Y axis to the parent building", EditCondition = "bSpecifyVAlignment"))
	EBuildingVAlignmentChoices VAlignment = EBuildingVAlignmentChoices::Bottom;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bSpecifyVerticalSpawnRange = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Vertical Spawn Amount", ToolTip = "The percent of the building height for which you want boxes generated", EditCondition = "bSpecifyVerticalSpawnRange", Units = "Percent", ClampMin = 1, ClampMax = 100))
	int VerticalSpawnPercent = 98;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bSpecifyHAlignment = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Horizontal Alignment", ToolTip = "Customize the alignment of the boxes on the X axis to the parent building", EditCondition = "bSpecifyHAlignment"))
	EBuildingHAlignmentChoices HAlignment = EBuildingHAlignmentChoices::Center;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bSpecifyHAlignmentOffset = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Horizontal Placement", ToolTip = "Specify an offset in centimeters (X/Y axis)", EditCondition = "bSpecifyHAlignmentOffset", Units = "Percent", ClampMin = 1, ClampMax = 150))
	FVector2D HOffset = FVector2D(500);

	//////////////////// MIRRORING ///////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Mirror X", ToolTip = "Mirror the box configuration in the X axis"))
	bool MirrorX = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Mirror Y", ToolTip = "Mirror the box configuration in the Y axis"))
	bool MirrorY = false;

	//////////////////// REPEATING ///////////////////////////////////
	UPROPERTY(EditAnywhere, meta = (DisplayName = "Use Repeat Options", ToolTip = "Enables settings for repeating box meshes"))
	bool bSpecifyRepeat = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Repeat in X", ToolTip = "Repeat the Box configuration in the X axis as many times the boxes will fit", EditCondition = "bSpecifyRepeat", EditConditionHides))
	bool RepeatXFill = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Repeat in Y", ToolTip = "Repeat the Box configuration in the Y axis as many times the boxes will fit", EditCondition = "bSpecifyRepeat", EditConditionHides))
	bool RepeatYFill = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Repeat X Exact", ToolTip = "Repeat the Box configuration in the X axis n times, the box will automatically be scaled to fit", Units = "times", EditCondition = "bSpecifyRepeat", EditConditionHides, UIMin = 2, UIMax = 100))
	int32 RepeatXTimes = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Repeat Y Exact", ToolTip = "Repeat the Box configuration in the Y axis n times, the box will automatically be scaled to fit", Units = "times", EditCondition = "bSpecifyRepeat", EditConditionHides, UIMin = 2, UIMax = 100))
	int32 RepeatYTimes = 0;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bHasFraming = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Framing Options", ToolTip = "Framing will appear on the exposed faces of the box", EditCondition = "bHasFraming"))
	FLatticeGridOptions FramingOptions;
};

USTRUCT(BlueprintType)
struct PROCEDURALBUILDINGS_API FDynamicBuildingPanelOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Thickness", ToolTip = "The thickness of the panels in Meters ", ClampMin = "0.1", UIMax = "20.0"))
	float Thickness = 60;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Sides Standoff", ToolTip = "The distance the side panels sit from the box surface in Meters", UIMin = "0.0", UIMax = "20.0"))
	float SideStandoff = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Roof Standoff", ToolTip = "The distance the top panel sits from the box surface in Meters", UIMin = "0.0", UIMax = "20.0"))
	float RoofStandoff = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Overhang Depth", ToolTip = "How far the panel extends past the underlying geometry (in meters), this extension will be present on all open sides", UIMin = "0.0", UIMax = "30.0"))
	float Overhang = 200;

	//////////////////// ROOF / FLOOR ///////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Panel Roof", ToolTip = "Create Roof Panel (no windows)"))
	bool bPanelRoof = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Panel Floor", ToolTip = "Create Floor Panel (no windows)"))
	bool bPanelFloor = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Panel N", ToolTip = "Panel in the North (X+) direction"))
	bool bPanelNorth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Panel E", ToolTip = "Panel in the East (Y+) direction"))
	bool bPanelEast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Panel S", ToolTip = "Panel in the South (X-) direction"))
	bool bPanelSouth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Panel W", ToolTip = "Panel in the West (Y-) direction"))
	bool bPanelWest = false;

	//////////////////// WINDOWS ///////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Windows Uniform", ToolTip = "Windows will be uniform for each panel side"))
	bool bWindowsUniform = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window N", ToolTip = "Add Windows for North (X+) panel", EditCondition = "bPanelNorth==true", EditConditionHides))
	bool bWindowNorth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window E", ToolTip = "Add Windows for the East (Y+) panel", EditCondition = "bPanelEast==true", EditConditionHides))
	bool bWindowEast = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window S", ToolTip = "Add Windows for the South (X-) panel", EditCondition = "bPanelSouth==true", EditConditionHides))
	bool bWindowSouth = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window W", ToolTip = "Add Windows for the West (Y-) panel", EditCondition = "bPanelWest==true", EditConditionHides))
	bool bWindowWest = false;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Rows/Columns Mode", ToolTip = "Windows will be aligned horizontally along rows, or vertically along columns"))
	EBuildingRowCol WindowGridMode = EBuildingRowCol::Row;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Boolean Mode", ToolTip = "The windows can be cut into the panel or added to the panel"))
	EGeometryScriptBooleanOperation WindowBoolMode = EGeometryScriptBooleanOperation::Subtract;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Depth", ToolTip = "The depth of the window (0) means depth of mesh"))
	float WindowDepth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Rows Determined by Floors", ToolTip = "The number of rows of windows will be determined by the number of floors in the section of the building"))
	bool bWindowRowsMatchesFloors = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Windows Per Row", ToolTip = "The max windows per row (0) adds as many as there are room for", ClampMin = 0, UIMax = 200))
	int32 WindowsPerRow = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Rows", ToolTip = "The number of rows (in row mode) or columns (in column mode) (0) adds as many as there are room for", ClampMin = 0, ClampMax = 100, EditCondition = "bWindowRowsMatchesFloors == false"))
	int32 WindowNumRows = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Horizontal Spacing", ToolTip = "Minimum Spacing between windows in a given row", UIMin = "0.1", UIMax = "5.0"))
	float WindowHSpacing = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Horizontal Spacing Varaince", ToolTip = "Spacing will vary by this much", UIMin = "0.0", UIMax = "5.0"))
	float WindowHSpacingVariance = 300;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Horizontal Alignment", ToolTip = "How rows of windows align themselves horizontally"))
	EBuildingHAlignmentChoices WindowHAlignment = EBuildingHAlignmentChoices::Random;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Vertical Spacing", ToolTip = "Minimum Spacing between rows of windows", UIMin = "0.1", UIMax = "50.0", EditCondition = "bWindowRowsMatchesFloors == false"))
	float WindowVSpacing = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Size", ToolTip = "The long dimension of windows (Meters), size will be constrained to parent geometry", UIMin = "0.5", UIMax = "250.0"))
	FVector2D WindowSize = FVector2D(300, 100);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Size Variance", ToolTip = "The variance of size (Meters)", UIMin = "0.5", UIMax = "250.0"))
	FVector2D WindowSizeVariance = FVector2D(150, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Window Edge Trim", ToolTip = "Ensure all windows are this distance from the edge of the panel (Meters)", UIMin = "0.0", UIMax = "5.0"))
	float WindowEdgeTrim = 80;


	//////////////////// NON BLUEPRINT MEMBERS ///////////////////////////////////
	float ThicknessCm;
	float SideStandoffCm;
	float RoofStandoffCm;
	float OverhangCm;

	void RefreshComputedValues();
	TSet<FVector> GetSidePanelVectors();
	TArray<FVector> GetSideVectors();
	TArray<FVector> GetWindowVectors();
	bool HasPanelAtVector(const FVector& CurrentPanel);
	bool HasPanelToLeft(const FVector& CurrentPanel);
	bool HasPanelToRight(const FVector& CurrentPanel);
	bool HasPanelAdjacent(const FVector& CurrentPanel);

	FVector GetPanelToLeftVector(const FVector& CurrentPanel);
	FVector GetPanelToRightVector(const FVector& CurrentPanel);
	FVector GetPanelAdjacentVector(const FVector& CurrentPanel);

	FRotator GetPanelRotation(const FVector& CurrentPanel);
	FVector GetPanelVectorRelativeRot(const FVector& CurrentPanel, const float DeltaRotation);

	float GetPanelDistanceFromCenter(const FVector& CurrentPanel, const FVector& BoxSize);
	FVector GetPanelBaseSize(const FVector& CurrentPanel, const FVector& BoxSize);
	float GetPanelOverlapSize(const FVector& CurrentPanel);
	struct FSizeAndTransform GetPanelSizeAndTransform(const FVector& CurrentPanel, const FVector& BoxSize);
	FTransform GetPanelBoxTransform(const FVector& CurrentPanel, const FVector& BoxSize);
	struct FSizeAndTransform GetRoofSizeAndTransform(const FVector& BoxSize);
	struct FSizeAndTransform GetFloorSizeAndTransform(const FVector& BoxSize);
};

/**
 * 
 */
UCLASS()
class PROCEDURALBUILDINGS_API ADynamicBuilding : public ADynamicMeshActor
{
	GENERATED_BODY()

		// Get the root componentent which is a UDynamicMesh
		// GetDynamicMeshComponent();

		// If you want a temporary "compute" UDynamicMesh call
		// AllocateComputeMesh();

		// when you are done with it you can release it back to the pool
		// ReleaseComputeMesh();

		// TODO on initialization we need to pick random values for values set to "random"
		// TODO add button for "refresh values set to 'random'" and "randomize all"

		// It may be wise to move to components, each part of the building can be a component of a specific type
		// this would allow for modularity.
		// For instance Our building Actor can contain a BuildingBox component that creates a general cube building
		// or you could start with a BuildingCylinder component that creates a cylinder, etc.
		// A separate box-level component could then find component by class to figure out which component is the base building and derivce its settings from that.
		
public:
	ADynamicBuilding(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Building", meta = (DisplayName = "Auto Rebuild", ToolTip = "Triggers automatic mesh rebuild each time a value is changed"))
	bool mAutoRebuild = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building", meta = (DisplayName = "Seed", ToolTip = "Options set to random will be consistent across edit until seed is changed"))
	int32 RandomSeed = 0;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Building", meta = (DisplayName = "Building Size", ToolTip = "Size of main building object in Meters"))
	FVector mBuildingSize = FVector(10000);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|UVs", meta = (DisplayName = "Materials", ToolTip = "Materials to apply"))
	TMap<EBuildingMaterialSlots, UMaterialInterface*> MaterialSlots;

	//////////////////// UV ///////////////////////////////////
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|UVs", meta = (DisplayName = "UV Scale Mode", ToolTip = "UV Scaling Mode"))
	EBuildingUVScaleMode UVScaleMode = EBuildingUVScaleMode::Fixed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|UVs", meta = (DisplayName = "UV Size", ToolTip = "UV Scale Size - The UV coordinates will repeat after this distance on the mesh", Unit = "Centimeter", EditCondition = "UVScaleMode == EBuildingUVScaleMode::Fixed"))
	float UVSize = 2000.f; // 20 meters

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building|UVs", meta = (DisplayName = "UV Origin Mode", ToolTip = "UV Origin - Detemrines where on the mesh the center of the UV coordinates will be, setting this to random will create a non-uniform appearance in the exture layout"))
	EBuildingUVOriginMode UVOriginMode = EBuildingUVOriginMode::MinCoordinate;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Building|Boxes", meta = (DisplayName = "Box Options", ToolTip = "Options for boxes", NoResetToDefault))
	FDynamicBuildingGenericBoxOptions mBoxOptions;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Building|Boxes", meta = (DisplayName = "Panel Options", ToolTip = "Options for sides of boxes (panels)", NoResetToDefault))
	FDynamicBuildingPanelOptions mPanelOptions;

	// Rebuild all meshes 
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Building|Actions", meta = (DisplayName = "Apply Changes"))
	void ReceiveRebuildAll();

	// Combine and export the dynamic meshes to a Static Mesh Asset 
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Building|Actions", meta = (DisplayName = "Export Static Mesh"))
	void ReceieveExportMesh();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

private:


	//The Function Pointer Variable Type
	//Functions take in 0 parameters and return void
	// A pointer is defined via: &ADynamicBuilding::GenerateBuilding;
	typedef void (ADynamicBuilding::* FunctionPtrType)(void); 

	UPROPERTY()
	TArray<UMaterialInterface*> MaterialSet;

	// UDynamicMeshComponent* BoxComponent; TODO REMOVE ME
	//TSet<UDynamicMeshComponent*> MeshComponentPool;
	//TArray<FunctionPtrType> mBuildFunctions; // build functions 
	//TArray<FunctionPtrType> mApplyFunctions;
	//TArray<UDynamicMeshComponent*> mBuildingBoxes;
	//TArray<UDynamicMeshComponent*> mBuildingPanels;

	UFUNCTION(BlueprintCallable)
	virtual void Generate();
	/*
	
	
	virtual void GenerateVerticalExtrusions();
	virtual void GenerateVerticalBooleans();
	*/

	// virtual void RandomizeAll() // uses this->RandomizationSettings()
	// virtual void RandomizeBuilding(), RandomizeBoxes() ... 
};


