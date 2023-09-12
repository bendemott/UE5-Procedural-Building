// Fill out your copyright notice in the Description page of Project Settings.


#include "BooleanGrid.h"
#include "BuildingEnums.h"
#include "DynamicCube.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "GeometryScript/MeshModelingFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshRepairFunctions.h"

BooleanGrid::BooleanGrid()
{
	// TODO remove me?
}

BooleanGrid::BooleanGrid(FBooleanGridOptions* Options)
	: Options(Options)
{
}

TTuple<int32, int32> BooleanGrid::GetWidthHeghtVectorIndex() {
	int WidthIdx;
	int HeightIdx;
	if (Options->BooleanGridMode == EBuildingRowCol::Row) {
		WidthIdx = 1; // Y
		HeightIdx = 2; // Z
	}
	else {
		WidthIdx = 2; // Z
		HeightIdx = 1; // Y
	}

	return MakeTuple(WidthIdx, HeightIdx);
}

int32 BooleanGrid::GetMaxRowsOrColumns(const FVector& MeshSize)
{
	// booleans can be aligned along a grid that is horizontal or vertical.
	// in the case of ROWS they are horizontally aligned, in the case of COLUMNS they are vertically aligned
	// regardless of the alignment, we treat BooleanSizeMax.Y as the HEIGHT of the boolean and 
	// BooleanSizeMax.X as the WIDTH of the boolean

	// find the max row height, the user may have reversed min/max so find which ever is the max.
	float MaxHeight = FMath::Max(Options->BooleanSizeMin.Y, Options->BooleanSizeMax.Y);

	// If the user has not provided valid height parameters we won't render any booleans.
	if (MaxHeight <= 0) {
		return 0;
	}

	// if in ROW mode the Mesh Z axis is it's "height", if in COLUMN mode, the Mesh Y axis is it's "height"
	int32 HeightIdx = GetWidthHeghtVectorIndex().Get<1>();
	float MeshHeight = MeshSize[HeightIdx];
	float UsableHeight = MeshHeight - (Options->SafeEdge * 2);

	// limit the height of the boolean to be the safe area of the mesh, this means that at least 1 boolean can be cut.
	// Although, this isn't exactly what was asked for it makes more sense than not performing any booleans.
	// a boolean larger than the available space should be applied to the maximum space allowed (right?)
	MaxHeight = FMath::Min(MaxHeight, UsableHeight);

	// the total potential height of each row
	// the height of the boolean can be in a range, and also there can be vertical spacing that is applied in 
	// addition to that maximum potential height.
	// This means the we calculate the distance apart (vertically) each row of booleans is based on this 
	// combined height.
	float HeightAndSpacingPerRow = MaxHeight + Options->VerticalSpacing;
	
	// Find the number of rows or columns that can fit in our usable height
	// The outside of the first row, and the outside of the last row, can but up against the outside 
	// of the geometry, they just need to respect Options->SafeEdge
	// The distance between rows needs to include Options->VerticalSpacing
	// The while loop below finds the maximum allowable number of "rows" while preserving the above rules.
	int MaxRows = 0;
	for (int32 Count = 0; Count < MAX_ROWS; Count++) {
		if (Count * HeightAndSpacingPerRow > UsableHeight) {
			break;
		}
		MaxRows++;
	}

	// If the user has opted to limit the number of rows or columns, apply that limit
	if (Options->bSpecifyMaxRowsColumns && Options->MaxRowCols >= 1) {
		MaxRows = FMath::Min(MaxRows, Options->MaxRowCols);
	}

	return MaxRows;

}

TTuple<TRange<float>, TRange<float>> BooleanGrid::GetBooleanSizeRange()
{
	auto XRange = TRange<float>();
	auto YRange = TRange<float>();

	TArray<TRange<float>> Ranges;

	for (int32 Index = 0; Index < 2; ++Index)
	{
		Options->BooleanSizeMin[Index]; // 0 will be X, 1 will be Y
		Options->BooleanSizeMax[Index];

		float High = FMath::Max(Options->BooleanSizeMin[Index], Options->BooleanSizeMax[Index]);
		float Low = FMath::Min(Options->BooleanSizeMin[Index], Options->BooleanSizeMax[Index]);

		if (Low == 0.f) {
			Low = High;
		}

		auto Range = TRange<float>::Inclusive(Low, High);
		Ranges.Add(Range);
	}

	// return X-Width(range) Y-Height(range)
	return MakeTuple(Ranges[0], Ranges[1]);
}



void BooleanGrid::ApplyBooleans(UDynamicMesh* Mesh, EGeometryScriptBooleanOperation BoolMode)
{
	/*
	* 1.) We want to evenly space the booleans across rows if there is a boolean limit 
	*     So we need to generate candidate boolean sizes, then determine how many rows they fit over
	* 2.) The placement of the boolean should be random, in other words we shouldn't start centered on a row or at the left edge
	* 3.) If there is no limit on the number of booleans, generate booleans until all space is occupied
	* 
	* - Generate a random set of sizes for booleans
	* - Apply those booleans until the number of booleans per row is reached or there is no more width left
	* - given the width of the row, and the width of our booleans randomly place the booleans on the row, respected the SafeEdge distance.
	* - layout the next row, respecting the spacing parameter.
	* - once all rows are layed out, the rows should be vertically centered respecting the SafeEdge distance
	* - Note that the mesh passed in must have a vertical Origin of "CENTER"
	* TODO:
	* - allow mesh to be penetrated from any side.
	* - in the event the mesh size exceeds the size of the parent mesh, just limit it so at least 1 row is generated
	*/
	FRandomStream RandomStream = FRandomStream(Options->RandomSeed);

	FBox Box = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(Mesh);
	// TODO from Box we can determine the center coordinate of the mesh in each axis.
	FVector MeshSize = Box.GetSize();
	FVector MeshCenter = Box.GetCenter();

	auto WidthHeight = GetWidthHeghtVectorIndex();
	int32 WidthIdx = WidthHeight.Get<0>();
	int32 HeightIdx = WidthHeight.Get<1>();
	int32 DepthIdx = 0;
	float MeshWidth = MeshSize[WidthIdx];
	float MeshHeight = MeshSize[HeightIdx];
	float MeshDepth = MeshSize[DepthIdx];

	// get the computed number of rows (row oriented) or columns (column oriented) we have room for.
	int32 NumRows = GetMaxRowsOrColumns(MeshSize);
	auto Ranges = GetBooleanSizeRange();

	// We don't switch the meaning of ranges based on if we are ROW or COLUMN oriented
	TRange<float> WidthRange = Ranges.Get<0>();
	TRange<float> HeightRange = Ranges.Get<1>();

	float BoolDepth = 0.f;
	if (BoolMode != EGeometryScriptBooleanOperation::Union ) {
		// if we aren't doing a union, and the depth is zero, this means we mean to cut through the mesh entirely leaving 
		// penetrating hole in the mesh.
		// If however,... depth is set to a value, that means we want to cut to from the front face into the mesh at the given depth.
		BoolDepth = Options->Depth == 0.f ? MeshSize[DepthIdx] + 1 : Options->Depth; // cut all the way through the mesh
	}
	else {
		// if this is a union we want to extend the mesh from the surface outward.
		// we'll assume a value of 0 means to extrude the mesh outwards the same depth as the mesh itself.
		BoolDepth = Options->Depth == 0.f ? MeshSize[DepthIdx]: Options->Depth;
	}
	float MaxTotalWidth = MeshSize[WidthIdx] - (Options->SafeEdge * 2);
	float MaxTotalHeight = MeshSize[HeightIdx] - (Options->SafeEdge * 2);
	float DistBetweenRows = HeightRange.GetUpperBoundValue() + Options->VerticalSpacing;
	float RowHeightHalf = HeightRange.GetUpperBoundValue() * 0.5;
	float MeshHalfHeight = MeshHeight * 0.5;
	float RowsTotalHeight = NumRows * HeightRange.GetUpperBoundValue() + (((NumRows - 1) * Options->VerticalSpacing));
	float RowsTotalHeightHalf = RowsTotalHeight * 0.5;
	int32 MaxRowBooleans = Options->bSpecifyMaxBooleansPerRow ? Options->MaxBooleansPerRowOrColumn : BooleanGrid::MAX_ROW_BOOLEANS;

	UE_LOG(LogTemp, Warning, TEXT("Booleans - MeshSize: %s, MeshCenter: %s, NumRows: %i, MaxCols: %i, WidthIdx: %i, HeightIdx: %i"), *(MeshSize.ToString()), *(MeshCenter.ToString()), NumRows, MaxRowBooleans, WidthIdx, HeightIdx);

	FTransform DefaultTransform;
	FGeometryScriptMeshBooleanOptions BooleanOptions;
	BooleanOptions.bFillHoles = true;
	BooleanOptions.bSimplifyOutput = true;

	UDynamicMesh* BoolMesh = NewObject<UDynamicMesh>();

	// keep track of space between booleans for this row
	TArray<float> HorizontalSpacings;

	// iterate over booleans of this row to generate size and location.
	for (int Row = 0; Row < NumRows; Row++) {

		float UsedWidth = 0.f;
		TArray<TTuple<FVector, FVector>> RowBooleans; // holds location, size vectors
		for (int Col = 0; Col < MaxRowBooleans; Col++) { // Upper limit to avoid while loop and keep some sanity.
			
			// calculate boolean width and height
			float Width = FMath::Min(RandomStream.FRandRange(WidthRange.GetLowerBoundValue(), WidthRange.GetUpperBoundValue()), MaxTotalWidth);
			float Height = FMath::Min(RandomStream.FRandRange(HeightRange.GetLowerBoundValue(), HeightRange.GetUpperBoundValue()), MaxTotalHeight);
			float HSpacing = FMath::Min(RandomStream.FRandRange(Options->HorizontalSpacing, Options->HorizontalSpacing + Options->HorizontalSpacingVariance), MaxTotalHeight);

			FVector Location;
			FVector Size;
			Size[WidthIdx] = Width;
			Size[HeightIdx] = Height;
			Size[DepthIdx] = BoolDepth;

			// calculate boolean relative location on parent mesh (used as transform for parent)
			Location[WidthIdx] = UsedWidth + (Width * 0.5);
			Location[HeightIdx] = 0;

			if (BoolMode == EGeometryScriptBooleanOperation::Union) {
				// make the mesh extend outwards from the surface.
				// ensure the mesh penetrates the outside wall of our mesh by exactly 1 unit so the boolean works.
				float HalfDepth = BoolDepth * 0.5;
				Location[DepthIdx] = (HalfDepth) + (MeshDepth * 0.5) - 1.f; 
			}
			else {
				// Penetrate the from its surface inward, offset above the surface at least 1 unit
				float HalfDepth = BoolDepth * 0.5;
				Location[DepthIdx] = -HalfDepth + (MeshDepth * 0.5) + 1.f;
			}
			
			if ((UsedWidth + Width) <= MaxTotalWidth) {
				RowBooleans.Add(MakeTuple(Location, Size));
				// Add a spacer for the next item that will be added
				UsedWidth += (Width + HSpacing);
				HorizontalSpacings.Add(HSpacing);
				UE_LOG(LogTemp, Warning, TEXT("Booleans[%i][%i] -  UsedWidth: %f, MaxTotalWidth: %f Size: %s, Location: %s"), Row, Col, UsedWidth, MaxTotalWidth, *(Size.ToString()), *(Location.ToString()));
			}
			else {
				break;
			}
		}
	
		//float RowHRandOffset = (MaxTotalWidth - UsedWidth) * 0.5;
		//float RowHCenter = RandomStream.FRandRange(-RowHRandOffset, RowHRandOffset);
		// no idea why its exactly (Options->HorizontalSpacing * 0.5) ??? to make it align?
		float UsedWidthHalf = UsedWidth * 0.5;
		float RowHAdjust = MeshCenter[WidthIdx] - (UsedWidthHalf - (Options->HorizontalSpacing * 0.5));
		float RemainingSafeHSpacePerSide = (MaxTotalWidth - UsedWidth) * 0.5;

		switch (Options->HorizontalAlignment) {
		case EBuildingHAlignmentChoices::Left:
			RowHAdjust -= RemainingSafeHSpacePerSide;
			break;
		case EBuildingHAlignmentChoices::Right:
			RowHAdjust += RemainingSafeHSpacePerSide;
			break;
		case EBuildingHAlignmentChoices::Random:
			// calculate a random position for the row horizontally, within the allowable bounds
			RowHAdjust += RandomStream.FRandRange(-RemainingSafeHSpacePerSide, RemainingSafeHSpacePerSide);
		case EBuildingHAlignmentChoices::Center:
		default:
			break;
		}

		// find the vertical position of this row, assume the center point of all rows is 0
		// `-MeshHalfHeight` orients for a origin that is in the middle of the mesh vertically.
		float FirstRowHeightOnCenter = (RowsTotalHeight - RowHeightHalf) - RowsTotalHeightHalf;

		//float RowVMiddle = RowsTotalHeightHalf - (Row * DistBetweenRows);
		float RowVMiddle = FirstRowHeightOnCenter - (DistBetweenRows * Row);
		RowVMiddle += MeshCenter[HeightIdx]; // adjust for the offset of the mesh (0 could be vertical center, or -500 could be)

		UE_LOG(LogTemp, Warning, TEXT("Booleans[%i] - UsedWidth: %f, RowHAdjust: %f, HorizontalSpacing: %f"), Row, UsedWidth, RowHAdjust, Options->HorizontalSpacing);

		UDynamicCube* Cube = NewObject<UDynamicCube>();

		// debug
		/*
		Cube->Reset();
		Cube->SetTranslation(FVector(0.f, 0.f, MeshCenter.Z));
		Cube->SetSize(FVector(BoolDepth, UsedWidth, MaxTotalHeight));
		Cube->SetOriginMode(EGeometryScriptPrimitiveOriginMode::Center);
		Cube->GenerateMesh(Mesh);
		*/

		// perform booleans for the current row, row boolean sizes and transorm info is stored in `RowBooleans`
		for (auto& Tuple : RowBooleans) {
			FVector Location = Tuple.Get<0>();
			FVector Size = Tuple.Get<1>();
			Location[WidthIdx] += RowHAdjust;
			Location[HeightIdx] += RowVMiddle;

			// append the boolean as a cube to the "BoolMesh"
			Cube->SetSize(Size);
			Cube->SetTranslation(Location);
			Cube->SetOriginMode(EGeometryScriptPrimitiveOriginMode::Center);
			// generate the mesh on "BoolMesh"
			Cube->GenerateMesh(BoolMesh);

			//UE_LOG(LogTemp, Warning, TEXT("Booleans[%i] Performing Boolean - Size: %s, Location: %s, RowMiddle: %f"), Row, *(Size.ToString()), *(Location.ToString()), RowVMiddle);
			/*
			auto RectOptions = FGeometryScriptPrimitiveOptions();
			// create a rectangle for our boolean
			FTransform RectTransform = FTransform(FQuat(FRotator(0.f, -90.f, 90.f)));
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
				BoolMesh,
				RectOptions,
				RectTransform,
				Size[WidthIdx],
				Size[HeightIdx],
				2,
				2);

			// extrude it in the direction of our boolean
			auto ExtrudeOptions = FGeometryScriptMeshExtrudeOptions();
			ExtrudeOptions.bSolidsToShells = false;
			ExtrudeOptions.ExtrudeDirection = FVector::ForwardVector;
			ExtrudeOptions.ExtrudeDistance = Size[DepthIdx];
			UGeometryScriptLibrary_MeshModelingFunctions::ApplyMeshExtrude( // THIS WAS CHANGED IN 5.1
				BoolMesh,
				ExtrudeOptions
			);
			*/
		}
	}


	// finaly perform booleans... it's most efficient to apply the boolean mesh all at once
	
	FVector ToolOrigin = MeshCenter + (FVector::DownVector * MeshHeight * 0.5);

	UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
		Mesh,              // target mesh
		DefaultTransform,  // target mesh transform
		BoolMesh,          // tool mesh
		FTransform(ToolOrigin), // location of the tool mesh
		BoolMode,       // subtract, intersect, union
		BooleanOptions  // fill-holes, simplify, etc
	);



}

BooleanGrid::~BooleanGrid()
{
}