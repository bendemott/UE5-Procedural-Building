

#pragma once

#include "CoreMinimal.h"
#include "BuildingEnums.generated.h"

UENUM(BlueprintType)
enum class EBuildingBooleanShapes : uint8
{
	Rectangle
};

UENUM(BlueprintType)
enum class EBuildingRowCol : uint8
{
	Row,
	Column
};

UENUM(BlueprintType)
enum class EBuildingHAlignmentChoices : uint8
{
	Left,
	Right,
	Center, 
	Random 
};

UENUM(BlueprintType)
enum class EBuildingVAlignmentChoices : uint8
{
	Middle, // x-
	Top, // x+
	Bottom, // x center
	Random // randomly choose one of the above 
};


UENUM(BlueprintType)
enum class EDynamicBuildingSideChoices : uint8
{
	None,
	OneSide,
	TwoSides,
	ThreeSides,
	AllSides,
	ChooseSides
};

UENUM(BlueprintType)
enum class EDynamicBuildingRotationChoices : uint8
{
	None,
	Rot90,
	Alternate,
	Random
};

// TODO the idea here is you pick what kind of configuration mode you want.
// general is just the current settings, general settings that configure everything
// or you can pick a explicit sequential list of boxes
// or you can pick at random out of a list of boxes
// or you can specify ranges of floors, or percentages of the building, and generate different styles of boxes that way
UENUM(BlueprintType)
enum class EDynamicBuildingSettingsModeChoices : uint8
{
	General,
	Sequential,
	RandomChoice,
	RangeCriteria
};

UENUM(BlueprintType)
enum class EBuildingUVScaleMode : uint8
{
	MinExtent,
	MaxExtent,
	MidExtent,
	AvgExtent,
	XExtent,
	YExtent,
	ZExtent,
	Fixed
};


UENUM(BlueprintType)
enum class EBuildingUVOriginMode : uint8
{
	MinCoordinate,
	MaxCoordinate,
	Random
};