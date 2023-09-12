


#include "LatticeGrid.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "GeometryScript/MeshBooleanFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"
#include "GeometryScript/MeshModelingFunctions.h"
#include "GeometryScript/MeshNormalsFunctions.h"
#include "GeometryScript/MeshRepairFunctions.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshTransformFunctions.h"
#include "GeometryScript/MeshUVFunctions.h"

LatticeGrid::LatticeGrid() {
}

LatticeGrid::LatticeGrid(FLatticeGridOptions* Options)
	: Options(Options)
{}

void LatticeGrid::ApplyLattice(UDynamicMesh* Mesh, TArray<UMaterialInterface*>& MaterialSet)
{
	UE_LOG(LogTemp, Display, TEXT("LatticeGrid Start"));
	if (Mesh == nullptr) {
		UE_LOG(LogTemp, Error, TEXT("LatticeGrid Mesh = nullptr"));
		return;
	}
	if (!Mesh->IsValidLowLevel()) {
		UE_LOG(LogTemp, Error, TEXT("LatticeGrid Mesh !IsValidLowLevel"));
		return;
	}
	if (Options == nullptr) {
		UE_LOG(LogTemp, Error, TEXT("LatticeGrid Options = nullptr"));
		return;
	}
	FRandomStream RandomStream = FRandomStream(Options->RandomSeed + LatticeGrid::RANDOM_OFFSET);
	FBox Box = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(Mesh);

	FVector FaceCenter = Box.GetCenter() - FVector(Box.GetSize().X * 0.5, 0.f, 0.f);
	FVector2D FaceSize = FVector2D(Box.GetSize().Y, Box.GetSize().Z); // X=width, Y=height
	
	FVector2D LatticeArea = FaceSize; // X=width, Y=height
	if (Options->bHasBorder) {
		LatticeArea -= FVector2D(Options->BorderSize.Y * 2, Options->BorderSize.X * 2);
	}
	else {
		LatticeArea = FaceSize;
	}
	
	// Setup size ranges
	float BaseWidth = FMath::Max(Options->Size.X, 1.f);
	float BaseHeight = FMath::Max(Options->Size.Y, 1.f);
	float BaseDepth = FMath::Max(Options->Size.Z, 1.f);

	float VaryWidth = Options->SizeVariance.X;
	float VaryHeight = Options->SizeVariance.Y;
	float VaryDepth = Options->SizeVariance.Z;

	auto HThicknessRange = TRange<float>::Inclusive(BaseWidth, BaseWidth + VaryWidth);
	auto VThicknessRange = TRange<float>::Inclusive(BaseHeight, BaseHeight + VaryHeight);
	auto DepthRange = TRange<float>::Inclusive(BaseDepth, BaseDepth + VaryDepth);

	// Setup spacing ranges
	float BaseSpacingWidth = FMath::Max(Options->Spacing.X, 1);
	float BaseSpacingHeight = FMath::Max(Options->Spacing.Y, 1);

	float VarySpacingWidth = Options->SpacingVariance.X;
	float VarySpacingHeight = Options->SpacingVariance.Y;

	auto HSpacingRange = TRange<float>::Inclusive(BaseSpacingWidth, BaseSpacingWidth + VarySpacingWidth);
	auto VSpacingRange = TRange<float>::Inclusive(BaseSpacingHeight, BaseSpacingHeight + VarySpacingHeight);

	// a value of 0 for rows or columns means "as many as will fit"
	int32 MaxRows = Options->Rows < 1 ? LatticeGrid::MAX_LATTICES : Options->Rows;
	MaxRows = Options->bHasRows ? MaxRows : 0;

	int32 MaxCols = Options->Columns < 1 ? LatticeGrid::MAX_LATTICES : Options->Columns;
	MaxCols = Options->bHasColumns ? MaxCols : 0;

	// we'll be reusing this a good number of times
	FTransform ZeroTransform = FTransform();

	// =======================================================================
	// =================== BUILD LATTICE =====================================
	// =======================================================================

	// Bottom left coordinate of the mesh face
	FVector2D BottomLeft = FVector2D(-(LatticeArea.X * 0.5), -(LatticeArea.Y * 0.5));

	// what materials should we assign
	const bool HasGlobalMaterial = Options->MaterialSlots.Contains(ELatticeMaterialSlots::All);
	const bool HasFramingMaterial = Options->MaterialSlots.Contains(ELatticeMaterialSlots::Framing_All);
	const bool HasBorderMaterial = Options->MaterialSlots.Contains(ELatticeMaterialSlots::Border_All);

	float UsedWidth = 0.f;
	float UsedHeight = 0.f;
	// TODO these could all be moved as class members and reused for the duration of the class instance...
	UDynamicMesh* CombinedMesh = NewObject<UDynamicMesh>();
	UDynamicMesh* BorderHMesh = NewObject<UDynamicMesh>();
	UDynamicMesh* BorderVMesh = NewObject<UDynamicMesh>();
	UDynamicMesh* RowsMesh = NewObject<UDynamicMesh>();
	UDynamicMesh* ColsMesh = NewObject<UDynamicMesh>();
	UDynamicMesh* ToolMesh = NewObject<UDynamicMesh>();

	// =================== BUILD ROWS ======================================
	for (int Row = 0; Row < MaxRows; Row++) {
		float Width = LatticeArea.X;
		float Thickness = FMath::Min(RandomStream.FRandRange(VThicknessRange.GetLowerBoundValue(), VThicknessRange.GetUpperBoundValue()), LatticeArea.Y);
		float Depth = FMath::Max(RandomStream.FRandRange(DepthRange.GetLowerBoundValue(), DepthRange.GetUpperBoundValue()), 1.f);
		float VSpacing = FMath::Min(RandomStream.FRandRange(VSpacingRange.GetLowerBoundValue(), VSpacingRange.GetUpperBoundValue()), LatticeArea.Y);

		UsedHeight += VSpacing + Thickness;

		UE_LOG(LogTemp, Display, TEXT("LatticeGrid Row[%i] - AvailHeight: %f, UsedHeight: %f, Width: %f, Thick: %f, Depth: %f, VSpace: %f"), Row, LatticeArea.Y, UsedHeight, Width, Thickness, Depth, VSpacing);

		FVector RowOrigin = FVector(0.f, 0.f, UsedHeight - (Thickness * 0.5));

		// exit early if we'll exceed the usable area
		if (UsedHeight > LatticeArea.Y) {
			UsedHeight -= VSpacing + Thickness;
			break;
		}

		ToolMesh->Reset();

		// create a rectangle to extrude, this will create a box with it's back face missing
		auto RectOptions = FGeometryScriptPrimitiveOptions();
		FTransform RectTransform = FTransform(FQuat(FRotator(0.f, -90.f, -90.f)));
		RectTransform.SetTranslation(RowOrigin);
		UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
			ToolMesh,
			RectOptions,
			RectTransform,
			Width,
			Thickness,
			2,
			2);

		// extrude outward from the center (negative x direction)
		auto ExtrudeOptions = FGeometryScriptMeshExtrudeOptions();
		ExtrudeOptions.bSolidsToShells = false;
		ExtrudeOptions.ExtrudeDirection = FVector::BackwardVector;
		ExtrudeOptions.ExtrudeDistance = Depth;
		UGeometryScriptLibrary_MeshModelingFunctions::ApplyMeshExtrude( // THIS WAS CHANGED IN 5.1
			ToolMesh,
			ExtrudeOptions
		);

		UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
			RowsMesh,
			ToolMesh, // mesh to append
			ZeroTransform
		);
	}

	// =================== BUILD COLUMNS ======================================
	for (int Col = 0; Col < MaxCols; Col++) {
		float Height = LatticeArea.Y;
		float Thickness = FMath::Min(RandomStream.FRandRange(HThicknessRange.GetLowerBoundValue(), HThicknessRange.GetUpperBoundValue()), LatticeArea.X);
		float Depth = FMath::Max(RandomStream.FRandRange(DepthRange.GetLowerBoundValue(), DepthRange.GetUpperBoundValue()), 1.f);
		float HSpacing = FMath::Min(RandomStream.FRandRange(HSpacingRange.GetLowerBoundValue(), HSpacingRange.GetUpperBoundValue()), LatticeArea.X);

		UsedWidth += HSpacing + Thickness;

		UE_LOG(LogTemp, Display, TEXT("LatticeGrid Col[%i] - AvailWidth: %f, UsedWidth: %f, Height: %f, Thick: %f, Depth: %f, HSpace: %f"), Col, LatticeArea.X, UsedWidth, Height, Thickness, Depth, HSpacing);

		// TODO this is wrong, the mesh is automatically centered on the parent mesh when placed.
		// so we don't want to apply spacing to the first element before it's placed.
		// also in this configuration there is no spacing after the last element...
		// we want ||  <column> <space> <column> <space> <column>  ||
		FVector ColOrigin = FVector(0.f, UsedWidth - (Thickness * 0.5), 0.f);

		// exit early if we'll exceed the usable area
		if (UsedWidth > LatticeArea.X) {
			UsedWidth -= HSpacing + Thickness;
			break;
		}

		ToolMesh->Reset();

		auto RectOptions = FGeometryScriptPrimitiveOptions();
		// create a rectangle for our boolean
		FTransform RectTransform = FTransform(FQuat(FRotator(0.f, -90.f, -90.f)));
		RectTransform.SetTranslation(ColOrigin);
		UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
			ToolMesh,
			RectOptions,
			RectTransform,
			Thickness,
			Height,
			0,
			0);

		// extrude outward from the center (negative x direction)
		auto ExtrudeOptions = FGeometryScriptMeshExtrudeOptions();
		ExtrudeOptions.bSolidsToShells = false;
		ExtrudeOptions.ExtrudeDirection = FVector::BackwardVector;
		ExtrudeOptions.ExtrudeDistance = Depth;
		UGeometryScriptLibrary_MeshModelingFunctions::ApplyMeshExtrude( // THIS WAS CHANGED IN 5.1
			ToolMesh,
			ExtrudeOptions
		);

		UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
			ColsMesh,
			ToolMesh, // mesh to append
			ZeroTransform
		);

	}

	// =================== TRANSFORM ROWS/COLS TO BE CENTERED ===============================
	// center in the Z axis
	UGeometryScriptLibrary_MeshTransformFunctions::TranslateMesh(RowsMesh, FVector(0.f, 0.f, UsedHeight * 0.5 * -1));

	// center in the Y axis
	UGeometryScriptLibrary_MeshTransformFunctions::TranslateMesh(ColsMesh, FVector(0.f, UsedWidth * 0.5 * -1, 0.f));

	// =================== FRAMING MATERIALS ================================================
	auto FramingMatOps = TArray<TTuple<UDynamicMesh*, int8>>();
	if ((Options->bHasRows || Options->bHasColumns) && !HasGlobalMaterial && Options->MaterialSlots.Contains(ELatticeMaterialSlots::Framing_All)) {
		// get the next available material id
		int8 MatId = MaterialSet.Num();
		// add our material to the MaterialSet array, this array will be set on the component as the list of active materials.
		MaterialSet.Add(Options->MaterialSlots[ELatticeMaterialSlots::Framing_All]);
		// apply the same material id to both meshes the `Framing_All` means we are applying the same material to all framing.
		FramingMatOps.Add(MakeTuple(RowsMesh, MatId));
		FramingMatOps.Add(MakeTuple(ColsMesh, MatId));
		UE_LOG(LogTemp, Display, TEXT("LatticeGrid MatId[%i] (Framing_All)"), MatId);
	}
	if (Options->bHasRows && !HasGlobalMaterial && !HasFramingMaterial && Options->MaterialSlots.Contains(ELatticeMaterialSlots::Framing_Horizontal)) {
		int8 MatId = MaterialSet.Num();
		MaterialSet.Add(Options->MaterialSlots[ELatticeMaterialSlots::Framing_Horizontal]);
		FramingMatOps.Add(MakeTuple(RowsMesh, MatId));
		UE_LOG(LogTemp, Display, TEXT("LatticeGrid MatId[%i] (Framing_Horizontal)"), MatId);
	}
	if (Options->bHasColumns && !HasGlobalMaterial && !HasFramingMaterial && Options->MaterialSlots.Contains(ELatticeMaterialSlots::Framing_Vertical)) {
		int8 MatId = MaterialSet.Num();
		MaterialSet.Add(Options->MaterialSlots[ELatticeMaterialSlots::Framing_Vertical]);
		FramingMatOps.Add(MakeTuple(ColsMesh, MatId));
		UE_LOG(LogTemp, Display, TEXT("LatticeGrid MatId[%i] (Framing_Vertical)"), MatId);
	}

	// apply material id's to the meshes provided.
	for (const auto& MatOps : FramingMatOps) {
		UDynamicMesh* MatMesh = MatOps.Get<0>();
		int8 MatId = MatOps.Get<1>();

		// change the default material id, to the assigned material.
		UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(
			MatMesh,
			0,
			MatId
		);

		FBox Bounds = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(MatMesh);
		FTransform UVTransform = GetMeshUVTransform(Bounds, Options->UVScaleMode, Options->UVOriginMode, &RandomStream, Options->UVSize);
		UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
			MatMesh,
			MatId,
			UVTransform, 
			2); // min island tri count
	}


	// =================== UNION ROWS AND COLUMNS ================================================
	// Combine the Rows/Cols Meshes
	UDynamicMesh* CombinedLatticeMesh = RowsMesh;
	// By unioning the two lattice meshes (rows/columns) we'll ensure there is no Z fighting at the intersections.
	UE_LOG(LogTemp, Display, TEXT("LatticeGrid - ApplyMeshBoolean"));
	UGeometryScriptLibrary_MeshBooleanFunctions::ApplyMeshBoolean(
		CombinedLatticeMesh,      // target mesh
		ZeroTransform,  // target mesh transform
		ColsMesh,      // tool mesh
		ZeroTransform,  // location of the tool mesh
		EGeometryScriptBooleanOperation::Union,  // subtract, intersect, union
		FGeometryScriptMeshBooleanOptions()  // fill-holes, simplify, etc
	);

	// =======================================================================
	// =================== BUILD BORDER ======================================
	// =======================================================================
	if (Options->bHasBorder) {
		UE_LOG(LogTemp, Display, TEXT("LatticeGrid - Building Border"));

		float HThickness = Options->BorderSize.X;
		float VThickness = Options->BorderSize.Y;
		float BorderDepth = Options->BorderSize.Z;

		float BorderWidth = FaceSize.X;
		float BorderHeight = FaceSize.Y - (HThickness * 2);

		// left right pieces
		const FVector LeftOrigin = FVector(0.f, -(FaceSize.X * 0.5) + (VThickness * 0.5), 0.f);
		const FVector RightOrigin = FVector(0.f, (FaceSize.X * 0.5) - (VThickness * 0.5), 0.f);
		auto LeftRight = TArray<FVector>();
		LeftRight.Add(LeftOrigin);
		LeftRight.Add(RightOrigin);

		// top bottom pieces
		const FVector TopOrigin = FVector(0.f, 0.f, (FaceSize.Y * 0.5) - (HThickness * 0.5));
		const FVector BottomOrigin = FVector(0.f, 0.f, -(FaceSize.Y * 0.5) + (HThickness * 0.5));
		auto TopBottom = TArray<FVector>();
		TopBottom.Add(TopOrigin);
		TopBottom.Add(BottomOrigin);

		UE_LOG(LogTemp, Display, TEXT("LatticeGrid Border - Width: %f, Height: %f, HThickness: %f, VThickness: %f, Depth: %f, HSpace: %f"), BorderWidth, BorderHeight, HThickness, VThickness, BorderDepth);

		// create left/right extrusions
		for (const auto& SideOrigin : LeftRight) {
			ToolMesh->Reset();
			auto RectOptions = FGeometryScriptPrimitiveOptions();
			// create a rectangle for our boolean
			FTransform RectTransform = FTransform(FQuat(FRotator(0.f, -90.f, -90.f))); // orient so height is along the +Y axis and face points FVector::BackwardVector (-X)
			RectTransform.SetTranslation(SideOrigin);
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
				ToolMesh,
				RectOptions,
				RectTransform,
				VThickness,
				BorderHeight,
				0,
				0);

			// extrude outward from the center (negative x direction)
			auto ExtrudeOptions = FGeometryScriptMeshExtrudeOptions();
			ExtrudeOptions.bSolidsToShells = false;
			ExtrudeOptions.ExtrudeDirection = FVector::BackwardVector;
			ExtrudeOptions.ExtrudeDistance = BorderDepth;
			UGeometryScriptLibrary_MeshModelingFunctions::ApplyMeshExtrude( // THIS WAS CHANGED IN 5.1
				ToolMesh,
				ExtrudeOptions
			);

			UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
				BorderVMesh,
				ToolMesh, // mesh to append
				ZeroTransform
			);
		}

		// create left/right extrusions
		for (const auto& TopBotOrigin : TopBottom) {
			ToolMesh->Reset();
			auto RectOptions = FGeometryScriptPrimitiveOptions();
			// create a rectangle for our boolean
			FTransform RectTransform = FTransform(FQuat(FRotator(0.f, -90.f, -90.f))); // orient so height is along the +Y axis and face points FVector::BackwardVector (-X)
			RectTransform.SetTranslation(TopBotOrigin);
			UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendRectangleXY(
				ToolMesh,
				RectOptions,
				RectTransform,
				BorderWidth,
				HThickness,
				0,
				0);

			// extrude outward from the center (negative x direction)
			auto ExtrudeOptions = FGeometryScriptMeshExtrudeOptions();
			ExtrudeOptions.bSolidsToShells = false;
			ExtrudeOptions.ExtrudeDirection = FVector::BackwardVector;
			ExtrudeOptions.ExtrudeDistance = BorderDepth;
			UGeometryScriptLibrary_MeshModelingFunctions::ApplyMeshExtrude( // THIS WAS CHANGED IN 5.1
				ToolMesh,
				ExtrudeOptions
			);

			UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
				BorderHMesh,
				ToolMesh, // mesh to append
				ZeroTransform
			);
		}

		auto BorderMatOps = TArray<TTuple<UDynamicMesh*, int8>>();
		if (!HasGlobalMaterial && Options->MaterialSlots.Contains(ELatticeMaterialSlots::Border_All)) {
			int8 MatId = MaterialSet.Num();
			MaterialSet.Add(Options->MaterialSlots[ELatticeMaterialSlots::Border_All]);
			BorderMatOps.Add(MakeTuple(BorderHMesh, MatId));
			BorderMatOps.Add(MakeTuple(BorderVMesh, MatId));
			UE_LOG(LogTemp, Display, TEXT("LatticeGrid MatId[%i] (Border_All)"), MatId);
		}
		if (!HasGlobalMaterial && !HasBorderMaterial && Options->MaterialSlots.Contains(ELatticeMaterialSlots::Border_Horizontal)) {
			int8 MatId = MaterialSet.Num();
			MaterialSet.Add(Options->MaterialSlots[ELatticeMaterialSlots::Border_Horizontal]);
			BorderMatOps.Add(MakeTuple(BorderHMesh, MatId));
			UE_LOG(LogTemp, Display, TEXT("LatticeGrid MatId[%i] (Border_Horizontal)"), MatId);
		}
		if (!HasGlobalMaterial && !HasBorderMaterial && Options->MaterialSlots.Contains(ELatticeMaterialSlots::Border_Vertical)) {
			int8 MatId = MaterialSet.Num();
			MaterialSet.Add(Options->MaterialSlots[ELatticeMaterialSlots::Border_Vertical]);
			BorderMatOps.Add(MakeTuple(BorderHMesh, MatId));
			UE_LOG(LogTemp, Display, TEXT("LatticeGrid MatId[%i] (Border_Vertical)"), MatId);
		}

		for (const auto& MatOps : BorderMatOps) {
			UDynamicMesh* MatMesh = MatOps.Get<0>();
			int8 MatId = MatOps.Get<1>();

			// change the default material id, to the assigned material.
			UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(
				MatMesh,
				0,
				MatId
			);

			FBox Bounds = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(MatMesh);
			FTransform UVTransform = GetMeshUVTransform(Bounds, Options->UVScaleMode, Options->UVOriginMode, &RandomStream, Options->UVSize);
			UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
				MatMesh,
				MatId,
				UVTransform,
				2); // min island tri count
		}
	}

	// =======================================================================
	// =================== MERGE MESHES ======================================
	// =======================================================================
	auto MergeMeshes = TArray<UDynamicMesh*>();
	MergeMeshes.Add(CombinedLatticeMesh);
	MergeMeshes.Add(BorderHMesh);
	MergeMeshes.Add(BorderVMesh);

	for (auto& MergeMesh : MergeMeshes) {
		UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
			CombinedMesh,
			MergeMesh,
			ZeroTransform
		);
	}

	// =================== Apply Global Material ======================================
	int8 GlobalMatId = -1;
	if (HasGlobalMaterial) {
		GlobalMatId = MaterialSet.Num();
		MaterialSet.Add(Options->MaterialSlots[ELatticeMaterialSlots::All]);
		UE_LOG(LogTemp, Display, TEXT("LatticeGrid MatId[%i] (All)"), GlobalMatId);
	}
	else if (Options->MaterialSlots.IsEmpty()) {
		UE_LOG(LogTemp, Warning, TEXT("LatticeGrid MaterialSlots empty, MaterialID 0 will be applied to mesh as a default"));
		GlobalMatId = 0;
	}

	if (GlobalMatId != -1) {
		// change the default material id, to the assigned material.
		UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(
			CombinedMesh,
			0,
			GlobalMatId
		);

		FBox Bounds = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(CombinedMesh);
		FTransform UVTransform = GetMeshUVTransform(Bounds, Options->UVScaleMode, Options->UVOriginMode, &RandomStream, Options->UVSize);
		UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
			CombinedMesh,
			GlobalMatId,
			UVTransform,  // uses GetScale3D() to determine the size of the projection, you usually want this the size of the mesh itself.
			2); // min island tri count


	}

	// =================== Add Lattice to Provided Mesh ==============================
	FTransform MeshTransform = FTransform(FaceCenter);
	UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
		Mesh,
		CombinedMesh,
		MeshTransform
	);

	UE_LOG(LogTemp, Display, TEXT("Lattice - Done"));
}

LatticeGrid::~LatticeGrid()
{
}

FTransform LatticeGrid::GetMeshUVTransform(FBox& MeshBounds, EBuildingUVScaleMode ScaleMode, EBuildingUVOriginMode OriginMode, FRandomStream* Randomizer, float UVSize)
{
	// THe scale of the UV's when applying a projection is the size of the object itself for a 0-1 fit of uv's to the object.
	FVector MeshSize = MeshBounds.GetSize();
	FVector UVScale;

	const double X = MeshSize.X;
	const double Y = MeshSize.Y;
	const double Z = MeshSize.Z;

	switch (ScaleMode) {
	case EBuildingUVScaleMode::MaxExtent:
		UVScale = FVector(MeshSize.GetMax());
		break;
	case EBuildingUVScaleMode::MinExtent:
		UVScale = FVector(MeshSize.GetMin());
		break;
	case EBuildingUVScaleMode::MidExtent:
		// Find the median value in (x, y, z)
		if ((X < Y && Y < Z) || (Z < Y && Y < X)) {
			UVScale = FVector(Y);
		}
		else if ((Y < X && X < Z) || (Z < X && X < Y)) {
			UVScale = FVector(X);
		}
		else {
			UVScale = FVector(Z);
		}
		break;
	case EBuildingUVScaleMode::AvgExtent:
		UVScale = FVector((MeshSize.X + MeshSize.Y + MeshSize.Z) / 3.f);
		break;
	case EBuildingUVScaleMode::XExtent:
		UVScale = FVector(MeshSize.X);
		break;
	case EBuildingUVScaleMode::YExtent:
		UVScale = FVector(MeshSize.Y);
		break;
	case EBuildingUVScaleMode::ZExtent:
		UVScale = FVector(MeshSize.Z);
		break;
	case EBuildingUVScaleMode::Fixed:
		UVScale = FVector(UVSize);
		break;
	}

	FVector UVOrigin = FVector::Zero();

	if (Randomizer == nullptr) {
		TUniquePtr<FRandomStream> Randomizer = MakeUnique<FRandomStream>();
	}

	switch (OriginMode) {
	case EBuildingUVOriginMode::MaxCoordinate:
		UVOrigin = MeshBounds.Max;
		break;
	case EBuildingUVOriginMode::MinCoordinate:
		UVOrigin = MeshBounds.Min;
		break;
	case EBuildingUVOriginMode::Random:
		// Set the origin to be a random spot on the mesh
		UVOrigin = FVector(
			Randomizer->RandRange(MeshBounds.Min.X, MeshBounds.Max.X),
			Randomizer->RandRange(MeshBounds.Min.Y, MeshBounds.Max.Y),
			Randomizer->RandRange(MeshBounds.Min.Z, MeshBounds.Max.Z)
		);
		break;
	}

	FTransform UVTransform(UVOrigin);
	UVTransform.SetScale3D(UVScale);
	return UVTransform;
}
