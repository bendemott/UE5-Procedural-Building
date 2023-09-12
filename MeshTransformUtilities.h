// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "MeshTransformUtilities.generated.h"

/**
 * This class will contain static methods to deal with transform operations on the mesh.
 * The internal mesh representation is UDynamicMesh3 - we will avoid using this internal interface and provide convenience functions instead.
 */
UCLASS(meta = (ScriptName = "DynamicBuildings_MeshTransforms"))
class PROCEDURALBUILDINGS_API UMeshTransformUtilities : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
};
