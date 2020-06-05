// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/SLBaseIndividual.h"
#include "SLIndividualComponent.generated.h"

// Delegate notification when the component is being destroyed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSLComponentDestroyedSignature, USLIndividualComponent*, DestroyedComponent);

/**
* Component storing the semantic individual information of its owner
*/
UCLASS( ClassGroup=(SL), meta=(BlueprintSpawnableComponent), DisplayName = "SL Individual Component")
class USEMLOG_API USLIndividualComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	USLIndividualComponent();

	// Called before destroying the object.
	virtual void BeginDestroy() override;

	// Called after the C++ constructor and after the properties have been initialized, including those loaded from config.
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	// Called when a property is changed in the editor
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

	//// Called when a component is created (not loaded) (after post init).This can happen in the editor or during gameplay
	//virtual void OnComponentCreated() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Set owner and individual
	bool Init(bool bReset = false);

	// Check if component is initialized
	bool IsInit() const { return bIsInit; };

	// Load individual
	bool Load(bool bReset = false);

	// Check if component is loaded
	bool IsLoaded() const { return bIsLoaded; };

	// Get the semantic individual object
	USLBaseIndividual* GetIndividualObject() const { return SemanticIndividual; };

	// Get the semantic individual using a cast class (nullptr if cast is unsuccessfull)
	template <typename ClassType>
	ClassType* GetCastedIndividualObject() const {	return Cast<ClassType>(SemanticIndividual); };

	/* Functionalities */
	// Save data to owners tag
	bool ExportToTag(bool bOverwrite = false);

	// Load data from owners tag
	bool ImportFromTag(bool bOverwrite = false);

	// Toggle between original and mask material is possible
	bool ToggleVisualMaskVisibility();

private:
	// Private init implementation
	bool InitImpl();

	// Private load implementation
	bool LoadImpl();

public:
	// Called when the component is destroyed
	FSLComponentDestroyedSignature OnDestroyed;

private:
	// Semantic data
	UPROPERTY(VisibleAnywhere, Category = "Semantic Logger")
	USLBaseIndividual* SemanticIndividual;

	// State of the component
	UPROPERTY(VisibleAnywhere, Category = "Semantic Logger")
	uint8 bIsInit : 1;

	UPROPERTY(VisibleAnywhere, Category = "Semantic Logger")
	uint8 bIsLoaded: 1;

	// Manually convert the semantic individual to the chosen type
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	TSubclassOf<class USLBaseIndividual> ConvertTo;

	/* Button workarounds */
	// Ovewrite any changes
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	uint8 bOverwriteEditChanges : 1;

	// Save data to tag
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	uint8 bExportToTagButton : 1;

	// Load data from tag
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	uint8 bImportFromTagButton : 1;

	// Switch between viewing the original and the visual mask color
	UPROPERTY(EditAnywhere, Category = "Semantic Logger|Manual Edit")
	uint8 bToggleVisualMaskMaterial : 1;
};
