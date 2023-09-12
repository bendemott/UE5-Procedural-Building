// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicCube.h"
#include "UDynamicMesh.h"
#include "GeometryScript/MeshPrimitiveFunctions.h"
#include "GeometryScript/MeshBasicEditFunctions.h"
#include "MeshTransformUtilities.h"

void UDynamicCube::SetSize(const FVector& Size)
{
	mSize = Size;
}

void UDynamicCube::AddToOrigin(const FVector& Delta)
{
	mOrigin += Delta;
}

void UDynamicCube::AddToSize(const FVector& Delta)
{
	mSize += Delta;
}

void UDynamicCube::AddToSizeInDirection(const float Size, const FVector& Direction)
{
	FVector Normalized = Direction.GetSafeNormal();
	FVector ChangeInSize = Size * Normalized.GetAbs();
	// the change to the origin is half the size changed, in the opposite direction
	FVector ChangeInOrigin = Normalized * Size * 0.5f * -1.f;
	AddToSize(ChangeInSize);
	AddToOrigin(ChangeInOrigin);
}

void UDynamicCube::SetTranslation(const FVector& Position)
{
	mTranslation = Position;
}

void UDynamicCube::AddToTranslation(const FVector& Delta)
{
	mTranslation += Delta;
}

void UDynamicCube::SetRotation(const FRotator& Rotation) 
{
	mRotation = Rotation;
}

void UDynamicCube::SetRotation(const FVector& Facing)
{
	mRotation = Facing.Rotation();
}

void UDynamicCube::SetOrigin(const FVector& Origin)
{
	mOrigin = Origin;
}

void UDynamicCube::SetGeometryOptions(const FGeometryScriptPrimitiveOptions Options)
{
	mGeometryOptions = Options;
}

void UDynamicCube::SetOriginMode(const EGeometryScriptPrimitiveOriginMode Origin)
{
	mOriginMode = Origin;
}

FVector UDynamicCube::GetSize()
{
	return mSize;
}

FVector UDynamicCube::GetTranslation()
{
	// an origin of x=-5 is a translation of x=5
	return (mOrigin * -1) + mTranslation;
}

FRotator UDynamicCube::GetRotation()
{
	return mRotation;
}

void UDynamicCube::SetTransform(const FTransform& Transform)
{
	mRotation = Transform.GetRotation().Rotator();
	mTranslation = Transform.GetTranslation();
	mScale = Transform.GetScale3D();
}

FTransform UDynamicCube::GetTransform()
{
	return FTransform(
		FQuat(GetRotation()),
		GetTranslation(),
		mScale
	);
}

void UDynamicCube::GenerateMesh(UDynamicMesh* Mesh, const bool ApplyTransform)
{
	// construct our mesh
	FTransform Transform = ApplyTransform ? GetTransform() : FTransform();

	UGeometryScriptLibrary_MeshPrimitiveFunctions::AppendBox(
		Mesh,
		mGeometryOptions,
		Transform,
		mSize.X,
		mSize.Y,
		mSize.Z,
		0,
		0,
		0,
		mOriginMode
	);
}

void UDynamicCube::Reset() 
{
	mTranslation = FVector::Zero();
	mRotation = FRotator(0.f, 0.f, 0.f);
	mSize = FVector(100.f, 100.f, 100.f);
	mScale = FVector(1.f);
	mGeometryOptions = FGeometryScriptPrimitiveOptions();
	mOriginMode = EGeometryScriptPrimitiveOriginMode::Base;
}