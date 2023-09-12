

#include "DynamicBuilding.h"
#include <functional> // std::hash
#include <string>
#include "BuildingEnums.h"
#include "Math/UnitConversion.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "GeometryScript/MeshTransformFunctions.h"
#include "GeometryScript/MeshQueryFunctions.h"
#include "GeometryScript/MeshUVFunctions.h"
#include "GeometryScript/MeshMaterialFunctions.h"
#include "DynamicMesh/MeshTransforms.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h" // FDynamicMeshMaterialAttribute
#include "DynamicMesh/DynamicMesh3.h"
#include "UDynamicMesh.h"
#include "DynamicCube.h"
#include "BooleanGrid.h"
#include "LatticeGrid.h"
#include "UVUtilities.h"


void ADynamicBuilding::ReceiveRebuildAll()
{
	// compose rebuilding.
	// TODO: eventually the set of calls to compose constructing the building will be based on different
	// options, I'll dynamically call the methods of construction depending on what features are selected
	// (or something like that so it's more flexible in general)
    Generate();
    UE_LOG(LogTemp, Warning, TEXT("Procedural Generation Complete"));
}

void ADynamicBuilding::ReceieveExportMesh()
{
    UE_LOG(LogTemp, Warning, TEXT("Export mesh TODO"));
    
}


ADynamicBuilding::ADynamicBuilding(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = false;
}


void ADynamicBuilding::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{

    if (mAutoRebuild && PropertyChangedEvent.Property != nullptr)
    {
        const FProperty* Property = PropertyChangedEvent.Property;
        const FName PropertyName(Property->GetFName());
        FName Category = FName(*(Property->GetMetaData(FName("Category"))));
        //UE_LOG(LogTemp, Warning, TEXT("Property Changed"));
        Generate(); // todo refactor to set timer...

        // GET_MEMBER_NAME_CHECKED(ADynamicBuilding, mWidth)

    }
    Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ADynamicBuilding::Generate()
{
    UDynamicMeshComponent* component = GetDynamicMeshComponent();
    UDynamicMesh* Mesh = component->GetDynamicMesh();
    if (Mesh == nullptr) {
        return;
    }
    if (!Mesh->IsValidLowLevel()) {
        return;
    }

    // Reset Mesh Geometry...
    Mesh->Reset();


    // Reset material slots
    MaterialSet.Empty();

    // all seed based randomization is based on FRandomStream
    // this keeps random parts of the generation consistent unless the seed is changed.
    FRandomStream RandomStream = FRandomStream(RandomSeed);

    FGeometryScriptPrimitiveOptions options = FGeometryScriptPrimitiveOptions();
    options.bFlipOrientation = false;
    options.PolygroupMode = EGeometryScriptPrimitivePolygroupMode::PerFace;
    options.UVMode = EGeometryScriptPrimitiveUVMode::Uniform;

    FTransform transform = FTransform();

    uint32 steps = 0; // TODO?

    FVector InEngineUnits = mBuildingSize;

    // Create the building core geometry
    UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
        Mesh,
        options,
        transform,
        InEngineUnits.X,
        InEngineUnits.Y,
        InEngineUnits.Z,
        steps,
        steps,
        steps,
        EGeometryScriptPrimitiveOriginMode::Base,
        nullptr
    );

    FBox BuildingBounds = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(Mesh);

    // ============== Mesh MATERIALS ==========================

    // ------------- TOP / BOTTOM ----------------------------
    // todo move this to a function
    if (!MaterialSlots.Contains(EBuildingMaterialSlots::All)
        && MaterialSlots.Contains(EBuildingMaterialSlots::Top_Bottom)) {
        // allocate a new material id
        int32 TopBotMatId = MaterialSet.Num();
        MaterialSet.Add(MaterialSlots[EBuildingMaterialSlots::Top_Bottom]);
        UE_LOG(LogTemp, Display, TEXT("Building MatId[%i] (Top_Bottom)"), TopBotMatId);

        // find geometry by face and assign material id
        TSet<FVector> UpDownVectors = TSet<FVector>();
        UpDownVectors.Add(FVector::UpVector);
        UpDownVectors.Add(FVector::DownVector);
        Mesh->EditMesh([&](FDynamicMesh3& Mesh)
        {
            // may need to Mesh.EnableAttributes();
            // Mesh.Attributes()->EnableMaterialID();
            using namespace UE::Geometry;
            FDynamicMeshMaterialAttribute* MaterialIDs = Mesh.Attributes()->GetMaterialID();
            if (MaterialIDs != nullptr) {
                // iterate through all the triangles of the mesh
                for (int32 TriangleID : Mesh.TriangleIndicesItr())
                {
                    FVector3d TriNormal = Mesh.GetTriNormal(TriangleID);
                    if (!UpDownVectors.Contains(TriNormal)) {
                        continue;
                    }
                    MaterialIDs->SetValue(TriangleID, TopBotMatId);
                }
            }

        }, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);


        FTransform UVTransform = UUVUtilities::GetMeshUVTransform(BuildingBounds, UVScaleMode, UVOriginMode, &RandomStream, UVSize);
        UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
            Mesh,
            TopBotMatId,
            UVTransform,  // uses GetScale3D() to determine the size of the projection, you usually want this the size of the mesh itself.
            2); // min island tri count
    }

    // ------------- SIDES -----------------------------------
    if (!MaterialSlots.Contains(EBuildingMaterialSlots::All)
        && MaterialSlots.Contains(EBuildingMaterialSlots::All_Sides)) {
        // allocate a new material id
        int32 SidesMatId = MaterialSet.Num();
        MaterialSet.Add(MaterialSlots[EBuildingMaterialSlots::All_Sides]);
        UE_LOG(LogTemp, Display, TEXT("Building MatId[%i] (All_Sides)"), SidesMatId);

        // find geometry by face and assign material id
        TSet<FVector> SideVectors = TSet<FVector>(mPanelOptions.GetSideVectors());
        Mesh->EditMesh([&](FDynamicMesh3& Mesh)
        {
            // may need to Mesh.EnableAttributes();
            // Mesh.Attributes()->EnableMaterialID();
            using namespace UE::Geometry;
            FDynamicMeshMaterialAttribute* MaterialIDs = Mesh.Attributes()->GetMaterialID();
            if (MaterialIDs != nullptr) {
                // iterate through all the triangles of the mesh
                for (int32 TriangleID : Mesh.TriangleIndicesItr())
                {
                    FVector3d TriNormal = Mesh.GetTriNormal(TriangleID);
                    if (!SideVectors.Contains(TriNormal)) {
                        continue;
                    }
                    MaterialIDs->SetValue(TriangleID, SidesMatId);
                }
            }

        }, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);


        FTransform UVTransform = UUVUtilities::GetMeshUVTransform(BuildingBounds, UVScaleMode, UVOriginMode, &RandomStream, UVSize);
        UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
            Mesh,
            SidesMatId,
            UVTransform,  // uses GetScale3D() to determine the size of the projection, you usually want this the size of the mesh itself.
            2); // min island tri count
    }

    int32 BuildingAllMatId = -1;
    if (MaterialSlots.Contains(EBuildingMaterialSlots::All)) {
        BuildingAllMatId = MaterialSet.Num();
        MaterialSet.Add(MaterialSlots[EBuildingMaterialSlots::All]);
        UE_LOG(LogTemp, Display, TEXT("Building MatId[%i] (All)"), BuildingAllMatId);
    }
    else if (MaterialSlots.IsEmpty()) {
        UE_LOG(LogTemp, Warning, TEXT("LatticeGrid MaterialSlots empty, MaterialID 0 will be applied to mesh as a default"));
        BuildingAllMatId = 0;
    }

    // map the whole cube
    if (BuildingAllMatId != -1) {
        // change the default material id, to the assigned material.
        UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(
            Mesh,
            0,
            BuildingAllMatId
        );

        FTransform UVTransform = UUVUtilities::GetMeshUVTransform(BuildingBounds, UVScaleMode, UVOriginMode, &RandomStream, UVSize);
        UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
            Mesh,
            BuildingAllMatId,
            UVTransform,  // uses GetScale3D() to determine the size of the projection, you usually want this the size of the mesh itself.
            2); // min island tri count
    }

    // ===========================================================================================================
    // ===========================================================================================================
    // ===========================================================================================================

    const float FloorHeight = mBoxOptions.FloorHeight;
    const float VerticalSpacing = mBoxOptions.VerticalSpacing;
    const int32 BuildingXSize = mBuildingSize.X;
    const int32 BuildingYSize = mBuildingSize.Y;
    const int32 BuildingHeight = mBuildingSize.Z;
    
    const TRange<int32> NumFloorsRange = TRange<int32>::Inclusive(
        FMath::Max(mBoxOptions.NumFloors, 1),
        mBoxOptions.NumFloorsVariance + FMath::Max(mBoxOptions.NumFloors, 1)
    );

    
    FVector2D BoxScale = FVector2D(mBoxOptions.Scale.X * 0.01, mBoxOptions.Scale.Y * 0.01);
    FVector2D BoxVarySize = mBoxOptions.VarySizePercent * 0.01;

    FVector2D BoxBaseSize = FVector2D(BuildingXSize * BoxScale.X, BuildingYSize * BoxScale.Y);

    const TRange<int32> BoxSizeXRange = TRange<int32>::Inclusive(
        FMath::Max(BoxBaseSize.X, 1),
        FMath::Max(BoxBaseSize.X + (BoxBaseSize.X * BoxVarySize.X), 1)
    );

    const TRange<int32> BoxSizeYRange = TRange<int32>::Inclusive(
        FMath::Max(BoxBaseSize.Y, 1),
        FMath::Max(BoxBaseSize.Y + (BoxBaseSize.Y * BoxVarySize.Y), 1)
    );

    const TRange<int32> BoxHeightRange = TRange<int32>::Inclusive(
        NumFloorsRange.GetLowerBoundValue() * FloorHeight, 
        NumFloorsRange.GetUpperBoundValue() * FloorHeight
    );

    int32 MaxNumBoxes = 1;
    int32 UsableBuildingHeight = BuildingHeight;
    if (mBoxOptions.bExplicitNumberOfBoxes) {
        MaxNumBoxes = mBoxOptions.NumberOfBoxes;
    } 
    else if (mBoxOptions.bSpecifyVerticalSpawnRange) {
        UsableBuildingHeight = (mBoxOptions.VerticalSpawnPercent * 0.01) * BuildingHeight;
        MaxNumBoxes = FMath::Max(UsableBuildingHeight / BoxHeightRange.GetUpperBoundValue(), 1);
    }
    else {
        MaxNumBoxes = FMath::Max(BuildingHeight / BoxHeightRange.GetUpperBoundValue(), 1);
    }

    UE_LOG(LogTemp, Warning, TEXT("GenerateBoxes - MaxNumBoxes = %i"), MaxNumBoxes);

// TODO switch to seed based iteration / generation
// TODO consider mirror, repeat, vertical alignment, vertical spawn percent.
// TODO horizontal alignment, offset
// TODO switch percents to be meters
// TODO panels need a control-arm holding them to the building
// TODO switch the building itself to be composed of meshes - one big mesh creates intersections, and other problems.
// TODO consider new settings min-building-core-size, building-core-shrinks-with-floors
// TODO add material channels
// TODO add a delay to rebuilding
// TODO refactor panel logic
// TODO refactor roof logic
// TODO create Templated Container for repition modes
//   

    double CumulativeBoxHeight = 0.f;
    FTransform EmptyTransform = FTransform();

    UDynamicMesh* BoxesMesh = AllocateComputeMesh();
    UDynamicMesh* BoxMesh = AllocateComputeMesh();
    UDynamicMesh* FloorMesh = AllocateComputeMesh();
    UDynamicMesh* RoofMesh = AllocateComputeMesh();
    UDynamicMesh* PanelMesh = AllocateComputeMesh();
    UDynamicMesh* TempMesh = AllocateComputeMesh();
    
    auto BoxMeshes = TArray<UDynamicMesh*>();
    BoxMeshes.Add(BoxMesh);
    BoxMeshes.Add(FloorMesh);
    BoxMeshes.Add(RoofMesh);
    BoxMeshes.Add(PanelMesh);
    

    for (int BoxNum = 0; BoxNum < MaxNumBoxes; BoxNum++) {
        // reset our temp mesh so it contains no geometry
        BoxMesh->Reset();
        FloorMesh->Reset();
        RoofMesh->Reset();
        PanelMesh->Reset();

        TSet<FVector> SidePanelVectors = mPanelOptions.GetSidePanelVectors();

        // ================ BOX SIZE / NUM FLOORS ===================
        FVector BoxSizeActual = FVector::Zero(); // holds the calculated size of the box after modifiers like scaling variation and randomness have been applied
        int NumFloors = mBoxOptions.NumFloors;
        if (mBoxOptions.NumFloorsVariance > 0) {
            NumFloors = RandomStream.RandRange(NumFloorsRange.GetLowerBoundValue(), NumFloorsRange.GetUpperBoundValue());
            BoxSizeActual.Z = FloorHeight * NumFloors;
        }
        else {
            BoxSizeActual.Z = BoxHeightRange.GetUpperBoundValue();
        }

        // vary the box size by percent
        if (mBoxOptions.bVaryBoxSizePercent) {
            BoxSizeActual.X = RandomStream.FRandRange(BoxSizeXRange.GetLowerBoundValue(), BoxSizeXRange.GetUpperBoundValue());
            BoxSizeActual.Y = RandomStream.FRandRange(BoxSizeYRange.GetLowerBoundValue(), BoxSizeYRange.GetUpperBoundValue());
        }
        else {
            BoxSizeActual.X = BoxBaseSize.X;
            BoxSizeActual.Y = BoxBaseSize.Y;
        }

        // ================ FLOOR PANEL ===========================
        FBox FloorBounds;
        if (mPanelOptions.bPanelFloor) {
            FSizeAndTransform Floor = mPanelOptions.GetFloorSizeAndTransform(BoxSizeActual);
            UDynamicCube* Cube = NewObject<UDynamicCube>(this);
            Cube->SetSize(Floor.Size);
            Cube->GenerateMesh(FloorMesh);

            /*
            UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
                BoxMesh,
                FloorMesh,
                Floor.Transform
            );*/

            // TODO this logic is replaced by bounding size logic
            //BoxHeight += Floor.Size.Z;
            //BoxHeight += Floor.Transform.GetTranslation().Z;

            // transform the floor in place, it is now in the correct relative position.
            UGeometryScriptLibrary_MeshTransformFunctions::TransformMesh(FloorMesh, Floor.Transform);
            // GetMeshBoundingBox - this means the implementation of creating the floor mesh can change and we'll still know how to 
            // space/stack things properly.
            FloorBounds = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(FloorMesh);
        }

        // ================ BOX GEOMETRY ==========================
        UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
            BoxMesh,
            FGeometryScriptPrimitiveOptions(),
            FTransform(FVector(0.f, 0.f, FloorBounds.Max.Z)),
            BoxSizeActual.X,
            BoxSizeActual.Y,
            BoxSizeActual.Z,
            0,
            0,
            0
        );

        FBox BoxBounds = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(BoxMesh);

        // ============== BOX MATERIALS ==========================
        
        // ------------- TOP / BOTTOM ----------------------------
        // todo move this to a function
        if (!mBoxOptions.MaterialSlots.Contains(EBuildingBoxMaterialSlots::All)
            && mBoxOptions.MaterialSlots.Contains(EBuildingBoxMaterialSlots::Top_Bottom)) {
            // allocate a new material id
            int32 TopBotMatId = MaterialSet.Num();
            MaterialSet.Add(mBoxOptions.MaterialSlots[EBuildingBoxMaterialSlots::Top_Bottom]);
            UE_LOG(LogTemp, Display, TEXT("Box MatId[%i] (All_Sides)"), TopBotMatId);

            // find geometry by face and assign material id
            TSet<FVector> UpDownVectors = TSet<FVector>();
            UpDownVectors.Add(FVector::UpVector);
            UpDownVectors.Add(FVector::DownVector);
            BoxMesh->EditMesh([&](FDynamicMesh3& Mesh)
            {
                // may need to Mesh.EnableAttributes();
                // Mesh.Attributes()->EnableMaterialID();
                using namespace UE::Geometry;
                FDynamicMeshMaterialAttribute* MaterialIDs = Mesh.Attributes()->GetMaterialID();
                if (MaterialIDs != nullptr) {
                    // iterate through all the triangles of the mesh
                    for (int32 TriangleID : Mesh.TriangleIndicesItr())
                    {
                        FVector3d TriNormal = Mesh.GetTriNormal(TriangleID);
                        if (!UpDownVectors.Contains(TriNormal)) {
                            continue;
                        }
                        MaterialIDs->SetValue(TriangleID, TopBotMatId);
                    }
                }

            }, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);


            FTransform UVTransform = UUVUtilities::GetMeshUVTransform(BoxBounds, mBoxOptions.UVScaleMode, mBoxOptions.UVOriginMode, &RandomStream, mBoxOptions.UVSize);
            UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
                BoxMesh,
                TopBotMatId,
                UVTransform,  // uses GetScale3D() to determine the size of the projection, you usually want this the size of the mesh itself.
                2); // min island tri count
        }

        // ------------- SIDES -----------------------------------
        if (!mBoxOptions.MaterialSlots.Contains(EBuildingBoxMaterialSlots::All)
            && mBoxOptions.MaterialSlots.Contains(EBuildingBoxMaterialSlots::All_Sides)) {
            // allocate a new material id
            int32 SidesMatId = MaterialSet.Num();
            MaterialSet.Add(mBoxOptions.MaterialSlots[EBuildingBoxMaterialSlots::All_Sides]);
            UE_LOG(LogTemp, Display, TEXT("Box MatId[%i] (All_Sides)"), SidesMatId);

            // find geometry by face and assign material id
            TSet<FVector> SideVectors = TSet<FVector>(mPanelOptions.GetSideVectors());
            BoxMesh->EditMesh([&](FDynamicMesh3& Mesh)
            {
                // may need to Mesh.EnableAttributes();
                // Mesh.Attributes()->EnableMaterialID();
                using namespace UE::Geometry;
                FDynamicMeshMaterialAttribute* MaterialIDs = Mesh.Attributes()->GetMaterialID();
                if (MaterialIDs != nullptr) {
                    // iterate through all the triangles of the mesh
                    for (int32 TriangleID : Mesh.TriangleIndicesItr())
                    {
                        FVector3d TriNormal = Mesh.GetTriNormal(TriangleID);
                        if (!SideVectors.Contains(TriNormal)) {
                            continue;
                        }
                        MaterialIDs->SetValue(TriangleID, SidesMatId);
                    }
                }

            }, EDynamicMeshChangeType::GeneralEdit, EDynamicMeshAttributeChangeFlags::Unknown, false);


            FTransform UVTransform = UUVUtilities::GetMeshUVTransform(BoxBounds, mBoxOptions.UVScaleMode, mBoxOptions.UVOriginMode, &RandomStream, mBoxOptions.UVSize);
            UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
                BoxMesh,
                SidesMatId,
                UVTransform,  // uses GetScale3D() to determine the size of the projection, you usually want this the size of the mesh itself.
                2); // min island tri count
        }

        int8 GlobalMatId = -1;
        if (mBoxOptions.MaterialSlots.Contains(EBuildingBoxMaterialSlots::All)) {
            GlobalMatId = MaterialSet.Num();
            MaterialSet.Add(mBoxOptions.MaterialSlots[EBuildingBoxMaterialSlots::All]);
            UE_LOG(LogTemp, Display, TEXT("Box MatId[%i] (All)"), GlobalMatId);
        }
        else if (mBoxOptions.MaterialSlots.IsEmpty()) {
            UE_LOG(LogTemp, Warning, TEXT("Box MaterialSlots empty, MaterialID 0 will be applied to mesh as a default"));
            GlobalMatId = 0;
        }

        // map the whole cube
        if (GlobalMatId != -1) {
            // change the default material id, to the assigned material.
            UGeometryScriptLibrary_MeshMaterialFunctions::RemapMaterialIDs(
                BoxMesh,
                0,
                GlobalMatId
            );

            FTransform UVTransform = UUVUtilities::GetMeshUVTransform(BoxBounds, mBoxOptions.UVScaleMode, mBoxOptions.UVOriginMode, &RandomStream, mBoxOptions.UVSize);
            UGeometryScriptLibrary_MeshUVFunctions::SetMeshUVsFromBoxProjection(
                BoxMesh,
                GlobalMatId,
                UVTransform,  // uses GetScale3D() to determine the size of the projection, you usually want this the size of the mesh itself.
                2); // min island tri count
        }

        


        // TODO add OPTIONAL chamfer

        

        // ================ BOX LATTICE ==========================
        if (mBoxOptions.bHasFraming) {
            LatticeGrid Lattice = LatticeGrid(&(mBoxOptions.FramingOptions));
            // HACK!!!!!!!!!
            // The logic for the lattice is not currently capable of being drawn on any side of the mesh, therefore
            // we must rotate the mesh so the lattice can be applied on each side.
            FVector DefaultFacing = FVector::ForwardVector;
            FVector CurrentFacing = DefaultFacing;
            for (auto& Direction : mPanelOptions.GetSideVectors()) {
                // const float Angle = FMath::Acos(FVector::DotProduct(Direction, CurrentFacing));
                FRotator DeltaRotation = (Direction.Rotation() - CurrentFacing.Rotation());
                DeltaRotation.Normalize();

                if (DeltaRotation.Yaw != 0.f) {
                    UGeometryScriptLibrary_MeshTransformFunctions::TransformMesh(BoxMesh, FTransform(FQuat(DeltaRotation)));
                }

                UE_LOG(LogTemp, Warning, TEXT("Box Rotation - Angle: %f, Current: %s, Direction: %s"), DeltaRotation.Yaw, *(CurrentFacing.ToString()), *(Direction.ToString()));
                CurrentFacing = Direction;
                Lattice.ApplyLattice(BoxMesh, MaterialSet);
            }

            // restore the orientation of the box to its default
            FRotator RestoreRotation = (CurrentFacing.Rotation() - DefaultFacing.Rotation());
            RestoreRotation.Normalize();
            UGeometryScriptLibrary_MeshTransformFunctions::TransformMesh(BoxMesh, FTransform(FQuat(RestoreRotation)));

        }

        // ================ ROOF PANEL ===========================
        FBox RoofBounds;
        if (mPanelOptions.bPanelRoof) {
            FSizeAndTransform Roof = mPanelOptions.GetRoofSizeAndTransform(BoxSizeActual);
            UDynamicCube* Cube = NewObject<UDynamicCube>(this);
            Cube->SetSize(Roof.Size);
            Cube->GenerateMesh(RoofMesh);

            //UE_LOG(LogTemp, Warning, TEXT("GenerateBoxes[%i]  Roof.Transform: %s Roof.Size: %s"), BoxNum, *(Roof.Transform.GetTranslation().ToString()), *(Roof.Size.ToString()));

            Roof.Transform.AddToTranslation(FVector(0.f, 0.f, BoxBounds.Max.Z));

            // transform the roof in place, it is now in the correct relative position.
            UGeometryScriptLibrary_MeshTransformFunctions::TransformMesh(RoofMesh, Roof.Transform);
            // GetMeshBoundingBox - this means the implementation of creating the floor mesh can change and we'll still know how to 
            // space/stack things properly.
            RoofBounds = UGeometryScriptLibrary_MeshQueryFunctions::GetMeshBoundingBox(RoofMesh);
        }

        //UE_LOG(LogTemp, Warning, TEXT("GenerateBoxes[%i]  mPanelOptions.PanelFaces.Num(): %i"), BoxNum, mPanelOptions.PanelFaces.Num());

            // ================ SIDE PANELS ===================
        int32 WindowSeed = RandomSeed + BoxNum;
        for (auto& Face : SidePanelVectors) {
            
            /*
            * Panel Faces:
            * ------------
            * We can build panel faces on each of the 4 sides of our boxes
            * The panels have a facing direction, defined by the constants of FVector like FVector::ForwardVector, FVector::LeftVector, etc.
            * If you place yourself in the center of the buildings cube, at coordinate 0,0 and you look forward, that panels facing direction is FVector::ForwardVector
            * If you look at your left, the panel against that side of the building (from the center) would be FVector::LeftVector
            * 
            * Each Panel can have a panel to its left or right, we use this information to determine how we join to the other panels.
            * If I'm looking to my left at the panel facing FVector::LeftVector, and I want to know if there is a panel to my left, I check for FVector::BackwardVector
            * If I want to know if there is a panel to my right I check for FVector::ForwardVector.
            * The convenience methods of `FDynamicBuildingPanelOptions` provide checks for these questions and help us know how to construct our panel.
            * 
            * The key thing to remember is the perspective of a panel and the meaning of left and right are always from a perspective that is at the center (inside) the building.
            * 
            * Panel Construction Methods:
            * ---------------------------
            * The logic that constructs each panel assumes that the perspective is FVector::ForwardVector (X+)
            * From this viewpoint the left side of the panel is (Y-) the right side is (Y+). Up is (Z+)
            * We stick to this convention to make it easy to reason about the coordinate system when constructing geometry procedurally.
            * After the panel is constructed, and modifiers like booleans are applied the panel is transformed and rotated into place on
            * the parent geometry (the box).
            * 
            * Origin:
            * -------
            * The origin of any geometry we spawn will be at the bottom-center of that geometry. So for our panel walls, the local 0,0 coordinate of that mesh is at the BOTTOM, 
            * and 1/2 the depth of the panel wall. If you were looking down at the top of the panel, you would see the origin exactly half way through the panel wall 
            * centered in both the X,Y directions.
            * This means that we have to compensate when we align or place our panel for the thickness of the panel, specifcally 1/2 the thickness to align to an outside face.
            */
            
            
            TempMesh->Reset();
            UDynamicCube* Cube = NewObject<UDynamicCube>(this);
            FSizeAndTransform Panel = mPanelOptions.GetPanelSizeAndTransform(Face, BoxSizeActual);
            Cube->SetSize(Panel.Size);
            //Cube->SetOriginMode(EGeometryScriptPrimitiveOriginMode::Base); // for the boolean logic to operator correctly must be centered.
            Cube->GenerateMesh(TempMesh);

            // ================ PANEL WINDOWS ===================
            // We have a panel mesh that is correctly centered about its origin, lets cut windows in it via boolean ops
            TUniquePtr<FBooleanGridOptions> BoolOptions = MakeUnique<FBooleanGridOptions>();
            // Setup boolean grid with options from our Windows properties
            BoolOptions->RandomSeed = WindowSeed;
            BoolOptions->Depth = mPanelOptions.WindowDepth;
            BoolOptions->bSpecifyMaxBooleansPerRow = (mPanelOptions.WindowsPerRow >= 1);
            BoolOptions->MaxBooleansPerRowOrColumn = mPanelOptions.WindowsPerRow;
            BoolOptions->BooleanShape = EBuildingBooleanShapes::Rectangle;
            BoolOptions->BooleanGridMode = mPanelOptions.WindowGridMode;
            BoolOptions->bSpecifyMaxRowsColumns = (mPanelOptions.bWindowRowsMatchesFloors || !(mPanelOptions.bWindowRowsMatchesFloors) && mPanelOptions.WindowNumRows > 0);
            BoolOptions->MaxRowCols = (mPanelOptions.bWindowRowsMatchesFloors) ? NumFloors : mPanelOptions.WindowNumRows;
            BoolOptions->BooleanSizeMin = mPanelOptions.WindowSize * 1;
            BoolOptions->BooleanSizeMax = (mPanelOptions.WindowSize + mPanelOptions.WindowSizeVariance) * 1;
            BoolOptions->SafeEdge = mPanelOptions.WindowEdgeTrim;
            BoolOptions->HorizontalSpacing = mPanelOptions.WindowHSpacing;
            BoolOptions->HorizontalSpacingVariance = mPanelOptions.WindowHSpacingVariance;
            BoolOptions->HorizontalAlignment = mPanelOptions.WindowHAlignment;
            // If the window spacing and number is the same as the number of floors calculate the spacing between windows based on max window size and max floor size.
            float SpaceBetweenWindows = FloorHeight - FMath::Max(BoolOptions->BooleanSizeMin.Y, BoolOptions->BooleanSizeMax.Y);
            BoolOptions->VerticalSpacing = mPanelOptions.bWindowRowsMatchesFloors ? SpaceBetweenWindows : mPanelOptions.WindowVSpacing;

            // TODO fix boolean logic so it can apply itself to mesh in any orientation so we don't have to perform a transform twice
            TUniquePtr<BooleanGrid> Booleans = MakeUnique<BooleanGrid>(BoolOptions.Get());
            Booleans->ApplyBooleans(TempMesh, mPanelOptions.WindowBoolMode);

            // If the windows are not uniform, increment our seed.
            if (!mPanelOptions.bWindowsUniform) {
                WindowSeed++;
            }
               
            // Apply the transform (this is a hack to force the transform to be applied)
            // this transform doesn't re-orient the panel to its final location... 
            // it just moves the origin so the panel will be offset correctly.
            UGeometryScriptLibrary_MeshTransformFunctions::TransformMesh(PanelMesh, Panel.Transform);

            // Get the transform relative to the parent box (this will rotate and orient the panel correctly)
            FTransform PanelBoxTransform = mPanelOptions.GetPanelBoxTransform(Face, BoxSizeActual);
            // APPLY THE TRANSFORM FOR THE PANEL HERE
            
            // add panel to PanelMesh
            UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
                PanelMesh,
                TempMesh,
                PanelBoxTransform
            );

            //UE_LOG(LogTemp, Warning, TEXT("GenerateBoxes[%i]  Panel: %s"), BoxNum, *(Face.ToString()));
        }
        
        // BoxTransform will control how the current box is attached to the overall structure
        FTransform BoxTransform = FTransform(FVector(0.f, 0.f, CumulativeBoxHeight));
  
        // Current Box Rotation
        FRotator BoxRotation = FRotator::ZeroRotator;
        if (mBoxOptions.ZRotation != 0.f) {
            if (mBoxOptions.RotationRandomizeInIncrements) {
                float RotMultiplier = 360.f / mBoxOptions.ZRotation;
                float RandMultiplier = RandomStream.RandRange(1, RotMultiplier);
                BoxRotation.Yaw = RandMultiplier * mBoxOptions.ZRotation;
            }
            else if (mBoxOptions.RotationRandomizeFromSet.Num()) {
                TSet<float>& RandSet = mBoxOptions.RotationRandomizeFromSet;
                int32 RandIndex = RandomStream.FRandRange(0, RandSet.GetMaxIndex());
                BoxRotation.Yaw = RandSet[FSetElementId::FromInteger(RandIndex)];
            }
            else {
                BoxRotation.Yaw = mBoxOptions.ZRotation;
            }
        }

        BoxTransform.SetRotation(FQuat(BoxRotation));

        auto BS = BoxBounds.GetSize();
        auto BC = BoxBounds.GetCenter();
        auto BE = BoxBounds.GetExtent();
        auto BMAX = BoxBounds.Max;

        // add TempMesh to BoxesMesh...
        UE_LOG(LogTemp, Warning, TEXT("GenerateBoxes[%i]  Size: %s Z Position: %f, S: %s, C: %s, E: %s, MAX: %s"), BoxNum, *(BoxSizeActual.ToString()), CumulativeBoxHeight, *(BS.ToString()), *(BC.ToString()), *(BE.ToString()), *(BMAX.ToString()));

        // the current total height of all boxes added together.
        // this value, is where the next box will spawn.
        CumulativeBoxHeight += (FMath::Max(RoofBounds.Max.Z, BoxBounds.Max.Z) + VerticalSpacing);

        // before we append the mesh make sure we aren't exceeding our usable space
        if (CumulativeBoxHeight >= UsableBuildingHeight) {
            UE_LOG(LogTemp, Warning, TEXT("GenerateBoxes - Stopping at Box [%i]  Height: %d Will Exceed Usable Building Height: %d"), BoxNum, CumulativeBoxHeight, BuildingHeight);
            break;
        }

        for (auto BMesh : BoxMeshes) {
            UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
                BoxesMesh,
                BMesh,
                BoxTransform // the box transform should place the box at the correct vertical position and rotation.
            );
        }


    } // end of Box creation loop

    for (auto BMesh : BoxMeshes) {
        ReleaseComputeMesh(BMesh);
    }
    BoxMeshes.Empty(); // don't need these anymore

    FVector BoxesOrigin = FVector();

    using VAlign = EBuildingVAlignmentChoices;

    VAlign Alignment = mBoxOptions.VAlignment;

    if (Alignment == VAlign::Random) {
        TArray<VAlign> RandChoices = { VAlign::Top, VAlign::Middle, VAlign::Bottom };

        //int RandIndex = CombinedSeed % (RandChoices.Num() - 1)

        // hash the internal name of our enum (just needs to be consistent across builds on the same platform)
        const char* TypeName = typeid(typename EBuildingVAlignmentChoices).name();
        std::string VAlignString = std::string(TypeName);
        std::hash<std::string> hasher;
        size_t VModeSeed = hasher(TypeName);
        uint32 VAlignSeed = static_cast<uint32>(VModeSeed + RandomSeed);
        uint32 RandIndex = VAlignSeed % (RandChoices.Num() - 1);
        Alignment = RandChoices[RandIndex];
    }

    switch (Alignment) {
    case EBuildingVAlignmentChoices::Middle:
        BoxesOrigin.Z = FMath::Max((BuildingHeight - CumulativeBoxHeight) / 2, 0);
        break;
    case EBuildingVAlignmentChoices::Top:
        BoxesOrigin.Z = FMath::Max(BuildingHeight - CumulativeBoxHeight, 0);
        break; 
    case EBuildingVAlignmentChoices::Bottom:
    default:
        BoxesOrigin.Z = 0;
    }

    FTransform BoxesTransform = FTransform();
    BoxesTransform.AddToTranslation(BoxesOrigin);

    // add BoxesMesh to Mesh...
    UGeometryScriptLibrary_MeshBasicEditFunctions::AppendMesh(
        Mesh,
        BoxesMesh,
        BoxesTransform
    );

    
    ReleaseComputeMesh(TempMesh);
    ReleaseComputeMesh(BoxesMesh);

    component->SetNumMaterials(0);
    component->ConfigureMaterialSet(MaterialSet);
    

    // TODO refactor and add a call to ReleaseAllComputerMeshes() to ensure there is never a memory leak
}

TArray<FVector> FDynamicBuildingPanelOptions::GetSideVectors()
{
    TArray<FVector> Faces;
    Faces.Emplace(FVector::ForwardVector);
    Faces.Emplace(FVector::RightVector);
    Faces.Emplace(FVector::BackwardVector);
    Faces.Emplace(FVector::LeftVector);
    return Faces;
}

TSet<FVector> FDynamicBuildingPanelOptions::GetSidePanelVectors()
{
    TSet<FVector> PanelFaces;

    if (bPanelNorth) {
        PanelFaces.Add(FVector::ForwardVector);
    }
    if (bPanelEast) {
        PanelFaces.Add(FVector::RightVector);
    }
    if (bPanelSouth) {
        PanelFaces.Add(FVector::BackwardVector);
    }
    if (bPanelWest) {
        PanelFaces.Add(FVector::LeftVector);
    }

    return PanelFaces;
}

bool FDynamicBuildingPanelOptions::HasPanelAtVector(const FVector& CurrentPanel)
{
    return GetSidePanelVectors().Contains(CurrentPanel);
}

bool FDynamicBuildingPanelOptions::HasPanelToLeft(const FVector& CurrentPanel)
{
    return HasPanelAtVector(GetPanelToLeftVector(CurrentPanel));
}

bool FDynamicBuildingPanelOptions::HasPanelToRight(const FVector& CurrentPanel)
{
    return HasPanelAtVector(GetPanelToRightVector(CurrentPanel));
}

bool FDynamicBuildingPanelOptions::HasPanelAdjacent(const FVector& CurrentPanel)
{
    return HasPanelAtVector(GetPanelAdjacentVector(CurrentPanel));
}

FVector FDynamicBuildingPanelOptions::GetPanelToLeftVector(const FVector& CurrentPanel)
{
    return GetPanelVectorRelativeRot(CurrentPanel, -90.f);
}

FVector FDynamicBuildingPanelOptions::GetPanelToRightVector(const FVector& CurrentPanel)
{
    return GetPanelVectorRelativeRot(CurrentPanel, 90.f);
}

FVector FDynamicBuildingPanelOptions::GetPanelAdjacentVector(const FVector& CurrentPanel)
{
    return GetPanelVectorRelativeRot(CurrentPanel, 180.f);
}

FVector FDynamicBuildingPanelOptions::GetPanelVectorRelativeRot(const FVector& CurrentPanel, const float DeltaRotation)
{
    FRotator PanelRot = GetPanelRotation(CurrentPanel);
    PanelRot.Yaw += DeltaRotation;
    PanelRot.Normalize();
    FVector Vec = PanelRot.Vector();
    return FVector(FMath::RoundToInt(Vec.X), FMath::RoundToInt(Vec.Y), FMath::RoundToInt(Vec.Z));
}

float FDynamicBuildingPanelOptions::GetPanelDistanceFromCenter(const FVector& CurrentPanel, const FVector& BoxSize)
{
    float Dist = 0.f;
    if (CurrentPanel.X != 0.f) {
        Dist = SideStandoff + (BoxSize.X * 0.5f);
    }
    else {
        Dist = SideStandoff + (BoxSize.Y * 0.5f);
    }
    return Dist;
}

FVector FDynamicBuildingPanelOptions::GetPanelBaseSize(const FVector& CurrentPanel, const FVector& BoxSize)
{
    FVector Size = FVector(Thickness, 0.f, BoxSize.Z);
    // Forward and Backward Vector, Y is width - Left and Right Vector, X is width
    Size.Y = (FMath::Abs(CurrentPanel.X) > 0) ? BoxSize.Y : BoxSize.X;
    
    return Size;
}

float FDynamicBuildingPanelOptions::GetPanelOverlapSize(const FVector& CurrentPanel)
{
    // if the panel is FVector::ForwardVector or FVector::BackwardVector it overlaps,
    // if its FVector::LeftVector or FVector::RightVector it doesn't.
    return (FMath::Abs(CurrentPanel.X) > 0) ? Thickness : 0.f;
}

FSizeAndTransform FDynamicBuildingPanelOptions::GetPanelSizeAndTransform(const FVector& CurrentPanel, const FVector& BoxSize)
{
    
    auto ST = FSizeAndTransform();
    FTransform Transform = FTransform();
    FVector Translation = FVector::Zero();
    FVector Size = FVector::Zero();

    FVector BaseSize = GetPanelBaseSize(CurrentPanel, BoxSize);

    // in order to determine the width of the panel we need to know if the panel has a left and right neighbor
    // if the panel on the left or right has a neighbor, overhang is ignored.
    //float OverhangSize = 
    bool LeftSide = HasPanelToLeft(CurrentPanel);
    bool RightSide = HasPanelToRight(CurrentPanel);
    float Overlap = GetPanelOverlapSize(CurrentPanel);
    float RoofHeight = 0.f;
    float FloorHeight = 0.f;
    float LeftSize = 0.f;
    float RightSize = 0.f;
    
    Size.X = BaseSize.X;
    Size.Z = BaseSize.Z;

    // === UP/DOWN Size and Offset ==============================
    FSizeAndTransform Roof = GetRoofSizeAndTransform(BoxSize);

    FloorHeight += bPanelFloor ? Thickness : 0.f;
    RoofHeight += bPanelRoof ? Roof.Size.Z + Roof.Transform.GetTranslation().Z : 0.f;

    Size.Z += (FloorHeight + RoofHeight);

    // === LEFT/RIGHT Size and Offset ===========================
    LeftSize = (LeftSide) ? SideStandoff + Overlap : Overhang;
    RightSize = (RightSide) ? SideStandoff + Overlap : Overhang;

    // in localspace the panels Y axis is its width, X is its depth
    Size.Y = BaseSize.Y + LeftSize + RightSize;
    Transform.AddToTranslation(FVector::LeftVector * (LeftSize * 0.5));
    Transform.AddToTranslation(FVector::RightVector * (RightSize * 0.5));

    //UE_LOG(LogTemp, Warning, TEXT("DEBUG: CurrentPanel: %s, Left: %i, Right: %i, Overlap: %f LeftSize: %f, RightSize: %f"), *(CurrentPanel.ToString()), LeftSide, RightSide, Overlap, LeftSize, RightSize);

    ST.Size = Size;
    ST.Transform = Transform;

    return ST;
}

FTransform FDynamicBuildingPanelOptions::GetPanelBoxTransform(const FVector& CurrentPanel, const FVector& BoxSize)
{
    float HalfThickness = Thickness * 0.5;
    FRotator Rotation = GetPanelRotation(CurrentPanel);
    FVector Translation = CurrentPanel * (GetPanelDistanceFromCenter(CurrentPanel, BoxSize) + HalfThickness);

    return FTransform(
        FQuat(Rotation),
        Translation,
        FVector(1.f)
    );
}

FSizeAndTransform FDynamicBuildingPanelOptions::GetRoofSizeAndTransform(const FVector& BoxSize)
{
    auto ST = FSizeAndTransform();
    FTransform Transform = ST.Transform;
    FVector Translation = FVector::Zero();
    FVector Size = FVector::Zero();

    Size.Z = Thickness;
    Translation.Z += RoofStandoff;

    for (auto& Face : GetSideVectors()) {
        float FaceSize = GetPanelDistanceFromCenter(Face, BoxSize);
        //UE_LOG(LogTemp, Warning, TEXT("Roof Face: %s FaceSize: %f"), *(Face.ToString()), FaceSize);
        if (!HasPanelAtVector(Face)) {
            //FaceSize += SideStandoff;
            FaceSize += Overhang;
            FaceSize -= SideStandoff;
        }

        Size += (Face.GetAbs() * FaceSize);
        Translation += (Face * FaceSize * 0.5);
    }

    Transform.SetTranslation(Translation);

    ST.Size = Size;
    ST.Transform = Transform;
    return ST;
}

FSizeAndTransform FDynamicBuildingPanelOptions::GetFloorSizeAndTransform(const FVector& BoxSize)
{
    FSizeAndTransform ST = GetRoofSizeAndTransform(BoxSize);
    FVector Translation = ST.Transform.GetTranslation();
    Translation.Z = 0;
    ST.Transform.SetTranslation(Translation);
    return ST;
}

FRotator FDynamicBuildingPanelOptions::GetPanelRotation(const FVector& CurrentPanel)
{
    return CurrentPanel.Rotation();
}