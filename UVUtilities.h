

#pragma once

#include "CoreMinimal.h"
#include "BuildingEnums.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UVUtilities.generated.h"

/**
 * 
 */
UCLASS(meta = (ScriptName = "BUildingScript_UVs"))
class PROCEDURALBUILDINGS_API UUVUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:

	//UFUNCTION(BlueprintCallable, Category = "ProceduralBuilding|UVs", meta = (ScriptMethod))
	// TODO make a blueprint version of this function
	static FTransform GetMeshUVTransform(FBox& MeshBounds, EBuildingUVScaleMode ScaleMode, EBuildingUVOriginMode OriginMode, FRandomStream* Randomizer, float UVSize);
};
