

#pragma once

#include "BuildingEnums.h"
#include "CoreMinimal.h"
#include "UDynamicMesh.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "BooleanGrid.generated.h"

USTRUCT(BlueprintType)
struct PROCEDURALBUILDINGS_API FBooleanGridOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Random Seed", ToolTip = "Randomness Seed"))
	int32 RandomSeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Depth", ToolTip = "The depth of the boolean cut, 0 indicates no depth limit"))
	float Depth = 0.f;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bSpecifyMaxBooleansPerRow = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Max Booleans Per Row", ToolTip = "Each row will contain at max this many booleans", EditCondition = "bSpecifyMaxBooleansPerRow"))
	int32 MaxBooleansPerRowOrColumn = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Shape Mode", ToolTip = "Shape of the Boolean"))
	EBuildingBooleanShapes BooleanShape = EBuildingBooleanShapes::Rectangle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Rows/Columns Mode", ToolTip = "Booleans will be aligned horizontally along rows, or vertically along columns"))
	EBuildingRowCol BooleanGridMode = EBuildingRowCol::Row;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Horizontal Spacing", ToolTip = "Distance horizontally between each boolean"))
	float HorizontalSpacing = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Boolean Horizontal Spacing Varaince"))
	float HorizontalSpacingVariance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Vertical Spacing", ToolTip = "Distance from the top edge of one boolean to the bottom edge of the next"))
	float VerticalSpacing = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Boolean Row Horizontal Alignment", ToolTip = "How rows of booleans align themselves horizontally to the parent geometry"))
	EBuildingHAlignmentChoices HorizontalAlignment = EBuildingHAlignmentChoices::Random;

	UPROPERTY(EditAnywhere, meta = (InlineEditConditionToggle))
	bool bSpecifyMaxRowsColumns = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Max Rows/Columns", ToolTip = "Max number of rows or columns", EditCondition = "bSpecifyMaxRowsColumns"))
	int32 MaxRowCols = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Size Min", ToolTip = "The min dimension of booleans (Meters), size will be constrained to parent geometry"))
	FVector2D BooleanSizeMin = FVector2D(300.f, 100.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Size Max", ToolTip = "The max dimension of booleans (Meters), size will be constrained to parent geometry"))
	FVector2D BooleanSizeMax = FVector2D(0.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (DisplayName = "Safe Edge", ToolTip = "Ensure all booleans are at least this far within the geometry"))
	float SafeEdge = 50;
};



/**
 * 
 * HOWTO:
 *	- 
 */
class PROCEDURALBUILDINGS_API BooleanGrid
{

public:
	// the absolute limit of booleans per row 
	static const int32 MAX_ROWS = 500;
	static const int32 MAX_ROW_BOOLEANS = 500;

	BooleanGrid();
	BooleanGrid(FBooleanGridOptions* Options);
	TTuple<int, int> GetWidthHeghtVectorIndex();
	int32 GetMaxRowsOrColumns(const FVector& MeshSize);
	TTuple<TRange<float>, TRange<float>> GetBooleanSizeRange(); 
	void ApplyBooleans(UDynamicMesh* Mesh, EGeometryScriptBooleanOperation BoolMode = EGeometryScriptBooleanOperation::Subtract);
	~BooleanGrid();

private:
	struct FBooleanGridOptions* Options;
};