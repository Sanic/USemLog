// Copyright 2017, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "SLRuntimeManager.h"
#include "SLUtils.h"

// Sets default values
ASLRuntimeManager::ASLRuntimeManager()
{
	PrimaryActorTick.bCanEverTick = true;

	// Defaults
	LogDirectory = FPaths::GameDir() + "SemLog";
	EpisodeId = "AutoGenerated";
	
	bLogRawData = true;
	RawDataUpdateRate = 0.f;
	TimePassedSinceLastUpdate = 0.f;
	bWriteRawDataToFile = true;
	bBroadcastRawData = false;
	
	bLogEventData = true;
	bWriteEventDataToFile = true;
	bBroadcastEventData = false;
}

// Make sure the manager is started before event publishers call BeginPlay
void ASLRuntimeManager::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// No tick by default
	SetActorTickEnabled(false);

	// Generate episode Id if not manually entered
	if (EpisodeId.Equals("AutoGenerated"))
	{
		EpisodeId = FSLUtils::GenerateRandomFString(4);
	}

	// Setup raw data logger
	if (bLogRawData)
	{
		// Create raw data logger UObject
		RawDataLogger = NewObject<USLRawDataLogger>(this, TEXT("RawDataLogger"));

		// Init logger 
		RawDataLogger->Init(GetWorld(), 0.1f);

		// Set logging type
		if (bWriteRawDataToFile)
		{
			RawDataLogger->InitFileHandle(EpisodeId, LogDirectory);
		}

		if (bBroadcastRawData)
		{
			RawDataLogger->InitBroadcaster();
		}

		// Log the first entry (static and dynamic entities)
		RawDataLogger->LogFirstEntry();

		// Enable tick for raw data logging
		SetActorTickEnabled(true);
	}

	// Setup event data logger
	if (bLogEventData)
	{
		// Create event data logger UObject
		EventDataLogger = NewObject<USLEventDataLogger>(this, TEXT("EventDataLogger"));

		// Initialize the event data
		EventDataLogger->InitLogger(EpisodeId);

		// Start logger
		EventDataLogger->StartLogger(GetWorld()->GetTimeSeconds());
	}
}

// Called when the game starts or when spawned
void ASLRuntimeManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called when actor removed from game or game ended
void ASLRuntimeManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if(bLogEventData && EventDataLogger)
	{
		// Finish up the logger - Terminate idle events
		EventDataLogger->FinishLogger(GetWorld()->GetTimeSeconds());

		if (bWriteEventDataToFile)
		{
			EventDataLogger->WriteEventsToFile(LogDirectory);
		}

		if (bBroadcastEventData)
		{
			EventDataLogger->BroadcastFinishedEvents();
		}
	}
}

// Called every frame
void ASLRuntimeManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Increase duration
	TimePassedSinceLastUpdate += DeltaTime;

	if (RawDataUpdateRate < TimePassedSinceLastUpdate)
	{
		// Log the raw data of the dynamic entities
		RawDataLogger->LogDynamicEntities();
		TimePassedSinceLastUpdate = 0.f;
	}
}

// Add finished event
bool ASLRuntimeManager::AddFinishedEvent(TSharedPtr<FOwlNode> Event)
{
	if (bLogEventData && EventDataLogger)
	{		
		return EventDataLogger->InsertFinishedEvent(Event);
	}
	return false;
}

// Start an event
bool ASLRuntimeManager::StartEvent(TSharedPtr<FOwlNode> Event)
{
	if (bLogEventData && EventDataLogger)
	{
		// Add event start time
		Event->Properties.Emplace(FOwlTriple(
			"knowrob:startTime", 
			"rdf:resource",
			"&log;timepoint_" + FString::SanitizeFloat(GetWorld()->GetTimeSeconds())));
		return EventDataLogger->StartAnEvent(Event);
	}
	return false;
}

// Finish an event
bool ASLRuntimeManager::FinishEvent(TSharedPtr<FOwlNode> Event)
{
	if (bLogEventData && EventDataLogger)
	{
		// Add event end time
		Event->Properties.Emplace(FOwlTriple(
			"knowrob:endTime",
			"rdf:resource",
			"&log;timepoint_" + FString::SanitizeFloat(GetWorld()->GetTimeSeconds())));
		return EventDataLogger->FinishAnEvent(Event);
	}
	return false;
}

// Add metadata property
bool ASLRuntimeManager::AddMetadataProperty(TSharedPtr<FOwlTriple> Property)
{
	if (bLogEventData && EventDataLogger)
	{
		return EventDataLogger->AddMetadataProperty(Property);
	}
	return false;
}