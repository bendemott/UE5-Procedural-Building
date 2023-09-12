

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UDynamicMesh.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "DynamicCube.generated.h"

/**
 * This is a wrapper around various static methods that the Procedural Mesh plugin provides.
 * Instead of having a bunch of different static methods to call, which becomes confusing, this class takes all the inputs and 
 * customizations to the given shape up-front and then applies it when `GetMesh()` is called.
 * 
 */
UCLASS()
class PROCEDURALBUILDINGS_API UDynamicCube : public UObject
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable)
		virtual void SetSize(const FVector& Size);

	UFUNCTION(BlueprintCallable)
		virtual void AddToSize(const FVector& Delta);

	UFUNCTION(BlueprintCallable)
		virtual void AddToOrigin(const FVector& Delta);

	UFUNCTION(BlueprintCallable)
		virtual void AddToSizeInDirection(const float Size, const FVector& Direction);

	UFUNCTION(BlueprintCallable)
		virtual void SetTranslation(const FVector& Position);

	UFUNCTION(BlueprintCallable)
		virtual void AddToTranslation(const FVector& Delta);

	UFUNCTION(BlueprintCallable)
		virtual void SetRotation(const FRotator& Rotation);

	virtual void SetRotation(const FVector& Facing);

	UFUNCTION(BlueprintCallable)
		virtual void SetOrigin(const FVector& Origin);

	UFUNCTION(BlueprintCallable)
		virtual void SetGeometryOptions(const FGeometryScriptPrimitiveOptions Options);

	UFUNCTION(BlueprintCallable)
		virtual void SetOriginMode(const EGeometryScriptPrimitiveOriginMode Origin);

	UFUNCTION(BlueprintCallable)
		virtual FVector GetSize();

	UFUNCTION(BlueprintCallable)
		virtual FVector GetTranslation();

	UFUNCTION(BlueprintCallable)
		virtual FRotator GetRotation();

	UFUNCTION(BlueprintCallable)
		virtual void SetTransform(const FTransform& Transform);

	UFUNCTION(BlueprintCallable)
		virtual FTransform GetTransform();

	UFUNCTION(BlueprintCallable)
		virtual void GenerateMesh(UDynamicMesh* Mesh, const bool ApplyTransform = true);

	UFUNCTION(BlueprintCallable)
		virtual void Reset();

	// todo taper
	// todo twist
	// todo rounded rectangle, etc

private:
	FVector mScale = FVector(1.f);
	FVector mSize = FVector(100.f, 100.f, 100.f);
	FVector mOrigin = FVector::Zero();
	FVector mTranslation = FVector::Zero();
	FRotator mRotation = FRotator();
	FGeometryScriptPrimitiveOptions mGeometryOptions;
	EGeometryScriptPrimitiveOriginMode mOriginMode = EGeometryScriptPrimitiveOriginMode::Base;
};