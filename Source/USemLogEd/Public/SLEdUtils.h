// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "CoreMinimal.h"

/**
 * Static helpers functions for the semantic logger editor module tookit
 */
class USEMLOGED_API FSLEdUtils
{
public:
	// Write the semantic map
	static void WriteSemanticMap(UWorld* World, bool bOverwrite = false);

	// Get the semantic individual manager from the world, add new if none are available
	static class ASLIndividualManager* GetOrCreateIndividualManager(UWorld* World, bool bCreateNew = true);

	// Get the vis info manager form the world, add new one if none are available
	static class ASLIndividualVisualInfoManager* GetVisualInfoManager(UWorld* World, bool bCreateNew = true);


	/* Individual actor visual info */
	static bool CreateVisualInfoComponents(UWorld* World, bool bOverwrite = false);
	static bool CreateVisualInfoComponents(const TArray<AActor*>& Actors, bool bOverwrite = false);
	static bool RefreshVisualInfoComponents(UWorld* World);
	static bool RefreshVisualInfoComponents(const TArray<AActor*>& Actors);
	static bool RemoveVisualInfoComponents(UWorld* World);
	static bool RemoveVisualInfoComponents(const TArray<AActor*>& Actors);
	static bool ToggleVisualInfoComponents(UWorld* World);
	static bool ToggleVisualInfoComponents(const TArray<AActor*>& Actors);

	static bool UpdateVisualInfoComponents(UWorld* World);
	static bool UpdateVisualInfoComponents(const TArray<AActor*>& Actors);
	static bool ToggleLiveUpdateVisualInfoComponents(UWorld* World);
	static bool ToggleLiveUpdateVisualInfoComponents(const TArray<AActor*>& Actors);

	// Save components data to tags
	static int32 ExportToTag(UWorld* World, bool bOverwrite = false);
	static int32 ExportToTag(const TArray<AActor*>& Actors, bool bOverwrite = false);

	// Loads components data from tags
	static int32 ImportFromTag(UWorld* World, bool bOverwrite = false);
	static int32 ImportFromTag(const TArray<AActor*>& Actors, bool bOverwrite = false);

	/* Ids */
	static int32 WriteUniqueIds(UWorld* World, bool bOverwrite = false);
	static int32 WriteUniqueIds(const TArray<AActor*>& Actors, bool bOverwrite = false);
	static int32 RemoveUniqueIds(UWorld* World);
	static int32 RemoveUniqueIds(const TArray<AActor*>& Actors);

	/* Class names */
	static int32 WriteClassNames(UWorld* World, bool bOverwrite = false);
	static int32 WriteClassNames(const TArray<AActor*>& Actors, bool bOverwrite = false);
	static int32 RemoveClassNames(UWorld* World);
	static int32 RemoveClassNames(const TArray<AActor*>& Actors);

	/* Visual masks */
	static int32 WriteVisualMasks(UWorld* World, bool bOverwrite = false);
	static int32 WriteVisualMasks(const TArray<AActor*>& Actors, UWorld* World, bool bOverwrite = false);
	static int32 RemoveVisualMasks(UWorld* World);
	static int32 RemoveVisualMasks(const TArray<AActor*>& Actors);

	// Remove all tag keys
	static bool RemoveTagKey(UWorld* World, const FString& TagType, const FString& TagKey);
	static bool RemoveTagKey(const TArray<AActor*>& Actors, const FString& TagType, const FString& TagKey);

	// Remove all types
	static bool RemoveTagType(UWorld* World, const FString& TagType);
	static bool RemoveTagType(const TArray<AActor*>& Actors, const FString& TagType);

	// Add the semantic monitor components registered to tags to the actors
	static bool AddSemanticMonitorComponents(UWorld* World, bool bOverwrite = false);
	static bool AddSemanticMonitorComponents(const TArray<AActor*>& Actors, bool bOverwrite = false);

	// Enable overlaps on actors
	static bool EnableOverlaps(UWorld* World);
	static bool EnableOverlaps(const TArray<AActor*>& Actors);

	// Enable all materials for instanced static mesh rendering
	static void EnableAllMaterialsForInstancedStaticMesh();

private:
	/* Semantic data components */
	// Reset semantic individual component
	static bool LoadSemanticIndividualComponent(AActor* Actor);

	// Remove semantic individual component
	static bool RemoveSemanticIndividualComponent(AActor* Actor);

	// Show hide the visual mask 
	static bool ToggleVisualMaskVisibility(AActor* Actor);

	// Save semantic individual data to tag
	static bool ExportIndividualDataToTag(AActor* Actor, bool bOverwrite = false);

	// Save semantic individual data to tag
	static bool ImportIndividualDataFromTag(AActor* Actor, bool bOverwrite = false);


	/* Visual info components */
	// Refresh the visual info component
	static bool RefreshVisualInfoComponent(AActor* Actor);

	// Remove the visual info component
	static bool RemoveVisualInfoComponent(AActor* Actor);

	// Show hide semantic text information
	static bool ToggleVisualInfo(AActor* Actor);

	// Point text towards camera
	static bool UpdateVisualInfo(AActor* Actor);

	// Constantly point text towards camera
	static bool ToggleLiveUpdateVisualInfo(AActor* Actor);

	/* Helpers */
	// Get the individual component of the actor (nullptr if none found)
	FORCEINLINE static class USLIndividualComponent* GetIndividualComponent(AActor* Actor);

	// Get the visual info component of the actor (nullptr if none found)
	FORCEINLINE static class USLIndividualVisualInfoComponent* GetVisualInfoComponent(AActor* Actor);



	//// Generate unique visual masks using incremental heuristic
	//static void WriteIncrementallyGeneratedVisualMasks(UWorld* World, bool bOverwrite = false);

	//// Get already used visual mask colors
	//static TArray<FColor> GetConsumedVisualMaskColors(UWorld* World);

	//// Add unique mask color
	//static bool AddUniqueVisualMask(AActor* Actor, TArray<FColor>& ConsumedColors, bool bOverwrite = false);

	//// Generate a new unique color in hex, avoiding any from the used up array
	//static FColor NewRandomlyGeneratedUniqueColor(TArray<FColor>& ConsumedColors, int32 NumberOfTrials = 100, int32 MinManhattanDistance = 29);

	///* Color helpers */
	//// Get the manhattan distance between the colors
	//FORCEINLINE static int32 ColorManhattanDistance(const FColor& C1, const FColor& C2)
	//{
	//	return FMath::Abs(C1.R - C2.R) + FMath::Abs(C1.G - C2.G) + FMath::Abs(C1.B - C2.B);
	//}

	//// Check if the two colors are equal with a tolerance
	//FORCEINLINE static bool ColorEqual(const FColor& C1, const FColor& C2, uint8 Tolerance = 0)
	//{
	//	if (Tolerance < 1) { return C1 == C2; }
	//	return ColorManhattanDistance(C1, C2) <= Tolerance;
	//}

	//// Generate a random color
	//FORCEINLINE static FColor ColorRandomRGB()
	//{
	//	return FColor((uint8)(FMath::FRand() * 255.f), (uint8)(FMath::FRand() * 255.f), (uint8)(FMath::FRand() * 255.f));
	//}

	//// Backlog
	//// Add unique id to the tag of the actor if the actor is a known type
	//static bool AddUniqueIdToTag(AActor* Actor, bool bOverwrite = false);

	//// Get class name of actor (if not known use label name if bDefaultToLabelName is true)
	//static FString GetClassName(AActor* Actor, bool bDefaultToLabelName = false);
};

