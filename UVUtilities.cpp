


#include "UVUtilities.h"
#include "BuildingEnums.h"

FTransform UUVUtilities::GetMeshUVTransform(FBox& MeshBounds, EBuildingUVScaleMode ScaleMode, EBuildingUVOriginMode OriginMode, FRandomStream* Randomizer, float UVSize)
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
