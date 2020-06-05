// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "Data/SLIndividualManager.h"
#include "Data/SLIndividualComponent.h"
#include "Data/SLIndividualUtils.h"

#include "EngineUtils.h"
#include "Kismet2/ComponentEditorUtils.h"

#include "Engine/StaticMeshActor.h"
#include "Animation/SkeletalMeshActor.h"

// Sets default values
ASLIndividualManager::ASLIndividualManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	bIsInit = false;
}

// Called when the game starts or when spawned
void ASLIndividualManager::BeginPlay()
{
	Super::BeginPlay();	
}

// Called every frame
void ASLIndividualManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Load components from world
int32 ASLIndividualManager::Init(bool bReset)
{
	if (bReset)
	{
		bIsInit = false;
		int32 NumClearedComp = ClearIndividualComponents();
		UE_LOG(LogTemp, Log, TEXT("%s::%d Reset: %ld components cleared.."),
			*FString(__FUNCTION__), __LINE__, NumClearedComp);
	}

	int32 NumComponentsLoaded = 0;
	if (!bIsInit)
	{
		if (GetWorld())
		{
			for (TActorIterator<AActor> ActItr(GetWorld()); ActItr; ++ActItr)
			{
				if (USLIndividualComponent* IC = GetIndividualComponent(*ActItr))
				{
					if (IC->Init())
					{
						if (!IC->Load())
						{
							UE_LOG(LogTemp, Warning, TEXT("%s::%d Individual component %s could not be loaded.."),
								*FString(__FUNCTION__), __LINE__, *IC->GetOwner()->GetName());
						}

						if (RegisterIndividualComponent(IC))
						{
							NumComponentsLoaded++;
						}
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("%s::%d Individual component %s could not be init.. the manager will not register it.."),
							*FString(__FUNCTION__), __LINE__, *IC->GetOwner()->GetName());
					}
				}
			}
			bIsInit = true;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("%s::%d Init: %ld components loaded.."),
		*FString(__FUNCTION__), __LINE__, NumComponentsLoaded);
	return NumComponentsLoaded;
}

// Add new semantic data components to the actors in the world
int32 ASLIndividualManager::AddIndividualComponents()
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	if (GetWorld())
	{
		for (TActorIterator<AActor> ActItr(GetWorld()); ActItr; ++ActItr)
		{
			if (USLIndividualComponent* IC = AddNewIndividualComponent(*ActItr))
			{
				if (RegisterIndividualComponent(IC))
				{
					Num++;
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d Could not access the world.."), *FString(__FUNCTION__), __LINE__);
	}
	return Num;
}

// Add new semantic data components to the selected actors
int32 ASLIndividualManager::AddIndividualComponents(const TArray<AActor*>& Actors)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& Act : Actors)
	{
		if (USLIndividualComponent* IC = AddNewIndividualComponent(Act))
		{
			if (RegisterIndividualComponent(IC))
			{
				Num++;
			}
		}
	}
	return Num;
}

// Remove all semantic data components from the world
int32 ASLIndividualManager::DestroyIndividualComponents()
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = RegisteredIndividualComponents.Num();
	for (const auto& IC : RegisteredIndividualComponents)
	{
		DestroyIndividualComponent(IC);
	}

	// Clear cached individuals
	ClearIndividualComponents();

	return Num;
}

// Remove selected semantic data components
int32 ASLIndividualManager::DestroyIndividualComponents(const TArray<AActor*>& Actors)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& Act : Actors)
	{
		if (USLIndividualComponent** FoundIC = IndividualComponentOwners.Find(Act))
		{
			UnregisterIndividualComponent(*FoundIC);
			DestroyIndividualComponent(*FoundIC);
			Num++;
		}
	}

	return Num;
}

// Reload components data
int32 ASLIndividualManager::ReloadIndividualComponents()
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& IC : RegisteredIndividualComponents)
	{
		bool bReset = true;
		if (IC->Load(bReset))
		{
			Num++;
		}
		else
		{
			//UnregisterIndividualComponent(IC);
			UE_LOG(LogTemp, Warning, TEXT("%s::%d Could not reload individual component %s .."),
				*FString(__FUNCTION__), __LINE__, *IC->GetOwner()->GetName());
		}
	}
	return Num;
}

// Reload selected actor components data
int32 ASLIndividualManager::ReloadIndividualComponents(const TArray<AActor*>& Actors)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& Act : Actors)
	{
		if (USLIndividualComponent** FoundIC = IndividualComponentOwners.Find(Act))
		{
			bool bReset = true;
			if ((*FoundIC)->Load(bReset))
			{
				Num++;
			}
			else
			{
				//UnregisterIndividualComponent(*FoundIC);
				UE_LOG(LogTemp, Warning, TEXT("%s::%d Could not reload individual component %s .."),
					*FString(__FUNCTION__), __LINE__, *(*FoundIC)->GetOwner()->GetName());
			}
		}
	}
	return Num;
}

/* Functionalities */
// Toggle perceivable individuals mask materials
int32 ASLIndividualManager::ToggleMaskMaterialsVisibility()
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& IC : RegisteredIndividualComponents)
	{
		if (IC->ToggleVisualMaskVisibility())
		{
			Num++;
		}
	}
	return Num;
}

// Toggle selected perceivable individuals mask materials
int32 ASLIndividualManager::ToggleMaskMaterialsVisibility(const TArray<AActor*>& Actors)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& Act : Actors)
	{
		if (USLIndividualComponent** FoundIC = IndividualComponentOwners.Find(Act))
		{
			if ((*FoundIC)->ToggleVisualMaskVisibility())
			{
				Num++;
			}
		}
	}
	return Num;
}

// Write new unique identifiers 
int32 ASLIndividualManager::WriteUniqueIds(bool bOverwrite)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& IC : RegisteredIndividualComponents)
	{
		//if (FSLIndividualUtils::WriteId(IC, bOverwrite))
		//{
		//	Num++;
		//}
	}
	return Num;
}

// Write new unique identifiers to selection
int32 ASLIndividualManager::WriteUniqueIds(const TArray<AActor*>& Actors, bool bOverwrite)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& Act : Actors)
	{
		if (USLIndividualComponent** FoundIC = IndividualComponentOwners.Find(Act))
		{
			//if (FSLIndividualUtils::WriteId(*FoundIC, bOverwrite))
			//{
			//	Num++;
			//}
		}
	}
	return Num;
}

int32 ASLIndividualManager::RemoveUniqueIds()
{
	return int32();
}

int32 ASLIndividualManager::RemoveUniqueIds(const TArray<AActor*>& Actors)
{
	return int32();
}

// Write class names
int32 ASLIndividualManager::WriteClassNames(bool bOverwrite)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& IC : RegisteredIndividualComponents)
	{
		//if (FSLIndividualUtils::WriteClass(IC, bOverwrite))
		//{
		//	Num++;
		//}
	}
	return Num;
}


// Write class names to selection
int32 ASLIndividualManager::WriteClassNames(const TArray<AActor*>& Actors, bool bOverwrite)
{
	if (!bIsInit)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Init manager first.."), *FString(__FUNCTION__), __LINE__);
		return INDEX_NONE;
	}

	int32 Num = 0;
	for (const auto& Act : Actors)
	{
		if (USLIndividualComponent** FoundIC = IndividualComponentOwners.Find(Act))
		{
			//if (FSLIndividualUtils::WriteClass(*FoundIC, bOverwrite))
			//{
			//	Num++;
			//}
		}
	}
	return Num;
}

int32 ASLIndividualManager::RemoveClassNames()
{
	return int32();
}

int32 ASLIndividualManager::RemoveClassNames(const TArray<AActor*>& Actors)
{
	return int32();
}

// Write visual masks
int32 ASLIndividualManager::WriteVisualMasks(bool bOverwrite)
{
	// TODO use cached individuals
	return FSLIndividualUtils::WriteVisualMasks(GetWorld(), bOverwrite);
}

int32 ASLIndividualManager::WriteVisualMasks(const TArray<AActor*>& Actors, bool bOverwrite)
{
	// TODO use cached individuals
	return FSLIndividualUtils::WriteVisualMasks(Actors, GetWorld(), bOverwrite);
}

int32 ASLIndividualManager::RemoveVisualMasks()
{
	return int32();
}

int32 ASLIndividualManager::RemoveVisualMasks(const TArray<AActor*>& Actors)
{
	return int32();
}

int32 ASLIndividualManager::ExportToTag(bool bOverwrite)
{
	return int32();
}

int32 ASLIndividualManager::ExportToTag(const TArray<AActor*>& Actors, bool bOverwrite)
{
	return int32();
}

int32 ASLIndividualManager::ImportFromTag(bool bOverwrite)
{
	return int32();
}

int32 ASLIndividualManager::ImportFromTag(const TArray<AActor*>& Actors, bool bOverwrite)
{
	return int32();
}


/* Private */
// Remove destroyed individuals from array
void ASLIndividualManager::OnIndividualComponentDestroyed(USLIndividualComponent* DestroyedComponent)
{
	UE_LOG(LogTemp, Error, TEXT("%s::%d Log message"), *FString(__FUNCTION__), __LINE__);
	if (UnregisterIndividualComponent(DestroyedComponent))
	{
		UE_LOG(LogTemp, Log, TEXT("%s::%d Unregistered externally destroyed component %s.."),
			*FString(__FUNCTION__), __LINE__, *DestroyedComponent->GetOwner()->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d Externally destroyed component %s is not registered, this should not happen.."),
			*FString(__FUNCTION__), __LINE__, *DestroyedComponent->GetOwner()->GetName());
	}
}

// Triggered by external destruction of semantic owner
void ASLIndividualManager::OnSemanticOwnerDestroyed(AActor* DestroyedActor)
{
	UE_LOG(LogTemp, Error, TEXT("%s::%d Log message"), *FString(__FUNCTION__), __LINE__);
}

// Find the individual component of the actor, return nullptr if none found
USLIndividualComponent* ASLIndividualManager::GetIndividualComponent(AActor* Actor)
{
	if (UActorComponent* AC = Actor->GetComponentByClass(USLIndividualComponent::StaticClass()))
	{
		return CastChecked<USLIndividualComponent>(AC);
	}
	return nullptr;
}

// Create and add new individual component
USLIndividualComponent* ASLIndividualManager::AddNewIndividualComponent(AActor* Actor)
{
	if (CanHaveIndividualComponents(Actor))
	{
		if (!GetIndividualComponent(Actor))
		{
			Actor->Modify();

			// Create an appropriate name for the new component (avoid duplicates)
			FName NewComponentName = *FComponentEditorUtils::GenerateValidVariableName(
				USLIndividualComponent::StaticClass(), Actor);

			// Get the set of owned components that exists prior to instancing the new component.
			TInlineComponentArray<UActorComponent*> PreInstanceComponents;
			Actor->GetComponents(PreInstanceComponents);

			// Create a new component
			USLIndividualComponent* NewComp = NewObject<USLIndividualComponent>(Actor, NewComponentName, RF_Transactional);

			// Make visible in the components list in the editor
			Actor->AddInstanceComponent(NewComp);
			//Actor->AddOwnedComponent(NewComp);

			NewComp->OnComponentCreated();
			NewComp->RegisterComponent();

			// Register any new components that may have been created during construction of the instanced component, but were not explicitly registered.
			TInlineComponentArray<UActorComponent*> PostInstanceComponents;
			Actor->GetComponents(PostInstanceComponents);
			for (UActorComponent* ActorComponent : PostInstanceComponents)
			{
				if (!ActorComponent->IsRegistered() && ActorComponent->bAutoRegister && !ActorComponent->IsPendingKill() && !PreInstanceComponents.Contains(ActorComponent))
				{
					ActorComponent->RegisterComponent();
				}
			}

			Actor->RerunConstructionScripts();

			if (NewComp->Init())
			{
				if (!NewComp->Load())
				{
					//UE_LOG(LogTemp, Warning, TEXT("%s::%d Individual component %s could not be loaded.. the manager will not register it.."),
					//	*FString(__FUNCTION__), __LINE__, *NewComp->GetOwner()->GetName());
					//return nullptr;
					UE_LOG(LogTemp, Warning, TEXT("%s::%d Individual component %s could not be loaded.."),
						*FString(__FUNCTION__), __LINE__, *NewComp->GetOwner()->GetName());
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("%s::%d Individual component %s could not be init.. the manager will not register it.."),
					*FString(__FUNCTION__), __LINE__, *NewComp->GetOwner()->GetName());
				return nullptr;
			}

			return NewComp;
		}
	}
	return nullptr;
}

// Check if actor type is supported for creating an individual component
bool ASLIndividualManager::CanHaveIndividualComponents(AActor* Actor)
{
	if (Actor->IsA(AStaticMeshActor::StaticClass()))
	{
		return true;
	}
	else if (Actor->IsA(ASkeletalMeshActor::StaticClass()))
	{
		return true;
	}
	return false;
}

// Remove individual component from owner
void ASLIndividualManager::DestroyIndividualComponent(USLIndividualComponent* Component)
{
	AActor* CompOwner = Component->GetOwner();
	CompOwner->Modify();
	CompOwner->RemoveInstanceComponent(Component);
	Component->ConditionalBeginDestroy();
}

// Cache component, bind delegates
bool ASLIndividualManager::RegisterIndividualComponent(USLIndividualComponent* Component)
{
	bool bSuccess = true;

	// Cache component
	if (!RegisteredIndividualComponents.Contains(Component))
	{
		RegisteredIndividualComponents.Emplace(Component);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Component %s is already registered, this should not happen.."),
			*FString(__FUNCTION__), __LINE__, *Component->GetOwner()->GetName());
		bSuccess = false;
	}

	// Cache components owner
	AActor* CompOwner = Component->GetOwner();
	if (!IndividualComponentOwners.Contains(CompOwner))
	{
		IndividualComponentOwners.Emplace(CompOwner, Component);
		if (!CompOwner->OnDestroyed.IsAlreadyBound(this, &ASLIndividualManager::OnSemanticOwnerDestroyed))
		{
			CompOwner->OnDestroyed.AddDynamic(this, &ASLIndividualManager::OnSemanticOwnerDestroyed);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Owner %s is already registered, this should not happen.."),
			*FString(__FUNCTION__), __LINE__, *CompOwner->GetName());
		bSuccess = false;
	}

	// Bind component events
	if (!Component->OnDestroyed.IsAlreadyBound(this, &ASLIndividualManager::OnIndividualComponentDestroyed))
	{
		Component->OnDestroyed.AddDynamic(this, &ASLIndividualManager::OnIndividualComponentDestroyed);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Component %s delegate is already bound, this should not happen.."),
			*FString(__FUNCTION__), __LINE__, *Component->GetOwner()->GetName());
		bSuccess = false;
	}

	return bSuccess;
}

// Remove component from cache, unbind delegates
bool ASLIndividualManager::UnregisterIndividualComponent(USLIndividualComponent* Component)
{
	bool bSuccess = true;

	if (!RegisteredIndividualComponents.Remove(Component))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Component %s was not registered, this should not happen.."),
			*FString(__FUNCTION__), __LINE__, *Component->GetOwner()->GetName());
		bSuccess = false;
	}

	AActor* CompOwner = Component->GetOwner();
	if (!IndividualComponentOwners.Remove(CompOwner))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Owner %s was not registered, this should not happen.."),
			*FString(__FUNCTION__), __LINE__, *CompOwner->GetName());
		bSuccess = false;
	}

	if (Component->OnDestroyed.IsAlreadyBound(this, &ASLIndividualManager::OnIndividualComponentDestroyed))
	{
		Component->OnDestroyed.RemoveDynamic(this, &ASLIndividualManager::OnIndividualComponentDestroyed);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Component %s delegate is not bound, this should not happen.."),
			*FString(__FUNCTION__), __LINE__, *Component->GetOwner()->GetName());
		bSuccess = false;
	}

	return bSuccess;
}

// Unregister all cached components
int32 ASLIndividualManager::ClearIndividualComponents()
{
	int32 NumClearedComponents = 0;
	for (const auto& C : RegisteredIndividualComponents)
	{
		//UnregisterIndividualComponent(C);

		if (C->OnDestroyed.IsAlreadyBound(this, &ASLIndividualManager::OnIndividualComponentDestroyed))
		{
			C->OnDestroyed.RemoveDynamic(this, &ASLIndividualManager::OnIndividualComponentDestroyed);
			NumClearedComponents++;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("%s::%d Component %s delegate is not bound, this should not happen.."),
				*FString(__FUNCTION__), __LINE__, *C->GetOwner()->GetName());
		}
	}

	//for (auto CItr(RegisteredIndividualComponents.CreateIterator()); CItr; ++CItr)
	//{
	//	UnregisterIndividualComponent(*CItr);
	//}

	if (NumClearedComponents != RegisteredIndividualComponents.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d Num of bound delegates (%ld) is out of sync with the num of registered components (%ld).."),
			*FString(__FUNCTION__), __LINE__, NumClearedComponents, RegisteredIndividualComponents.Num());
		NumClearedComponents = INDEX_NONE;
	}

	RegisteredIndividualComponents.Empty();
	IndividualComponentOwners.Empty();

	return NumClearedComponents;
}

bool ASLIndividualManager::IsIndividualComponentRegisteredFull(USLIndividualComponent* Component) const
{
	 return RegisteredIndividualComponents.Contains(Component)
		&& IndividualComponentOwners.Contains(Component->GetOwner())
		&& Component->OnDestroyed.IsAlreadyBound(this, &ASLIndividualManager::OnIndividualComponentDestroyed);
}

