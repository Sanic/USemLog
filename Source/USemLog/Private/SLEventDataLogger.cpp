// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "SLEventDataLogger.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "EngineUtils.h"
#include "OwlExperimentStatics.h"
#include "SLOverlapArea.h"
#include "SLGoogleCharts.h"
#include "Ids.h"

// Constructor
USLEventDataLogger::USLEventDataLogger()
{
}

// Destructor
USLEventDataLogger::~USLEventDataLogger()
{
}

// Init Logger
void USLEventDataLogger::Init(const FString& InLogDirectory,
	const FString& InEpisodeId,
	EOwlExperimentTemplate TemplateType,
	bool bInWriteTimelines)
{
	LogDirectory = InLogDirectory;
	EpisodeId = InEpisodeId;
	OwlDocTemplate = TemplateType;
	bWriteTimelines = bInWriteTimelines;

	// Create the document template
	ExperimentDoc = CreateEventsDocTemplate(TemplateType, InEpisodeId);
}

// Start logger
void USLEventDataLogger::Start()
{
	// Subscribe for semantic contact events
	USLEventDataLogger::ListenToSemanticContactRelatedEvents();
}

// Finish logger
void USLEventDataLogger::Finish()
{
	if (!ExperimentDoc.IsValid())
		return;

	// Add finished events to doc
	for (const auto& Ev : FinishedEvents)
	{
		Ev->AddToOwlDoc(ExperimentDoc.Get());
	}

	// Add stored unique timepoints to doc
	ExperimentDoc->AddTimepointIndividuals();

	// Add stored unique objects to doc
	ExperimentDoc->AddObjectIndividuals();

	// Add experiment individual to doc
	ExperimentDoc->AddExperimentIndividual();

	// Write events to file
	WriteToFile();
}

// Register for semantic contact events
void USLEventDataLogger::ListenToSemanticContactRelatedEvents()
{
	// Iterate all contact trigger components, and bind to their events publisher
	for (TObjectIterator<USLOverlapArea> Itr; Itr; ++Itr)
	{
		if (Itr->SLContactPub.IsValid())
		{
			Itr->SLContactPub->OnSemanticContactEvent.BindUObject(
				this, &USLEventDataLogger::OnSemanticContactEvent);
		}

		if (Itr->SLSupportedByPub.IsValid())
		{
			Itr->SLSupportedByPub->OnSupportedByEvent.BindUObject(
				this, &USLEventDataLogger::OnSemanticSupportedByEvent);
		}
	}
}

// Called when a semantic contact is finished
void USLEventDataLogger::OnSemanticContactEvent(TSharedPtr<FSLContactEvent> Event)
{
	UE_LOG(LogTemp, Error, TEXT(">> %s::%d OOooOOoOOoo"), TEXT(__FUNCTION__), __LINE__);
	FinishedEvents.Add(Event);
}

// Called when a semantic supported by event is finished
void USLEventDataLogger::OnSemanticSupportedByEvent(TSharedPtr<FSLSupportedByEvent> Event)
{
	UE_LOG(LogTemp, Warning, TEXT(">> %s::%d"), TEXT(__FUNCTION__), __LINE__);
	FinishedEvents.Add(Event);
}

// Write to file
bool USLEventDataLogger::WriteToFile()
{
	// Write events timelines to file
	if (bWriteTimelines)
	{
		FSLGoogleCharts::WriteTimelines(FinishedEvents, LogDirectory, EpisodeId);
	}

	if (!ExperimentDoc.IsValid())
		return false;

	// Write experiment to file
	FString FullFilePath = FPaths::ProjectDir() +
		LogDirectory + TEXT("/Episodes/") + EpisodeId + TEXT("_ED.owl");
	FPaths::RemoveDuplicateSlashes(FullFilePath);
	return FFileHelper::SaveStringToFile(ExperimentDoc->ToString(), *FullFilePath);
}

// Create events doc (experiment) template
TSharedPtr<FOwlExperiment> USLEventDataLogger::CreateEventsDocTemplate(EOwlExperimentTemplate TemplateType, const FString& InDocId)
{
	// Create unique semlog id for the document
	const FString DocId = InDocId.IsEmpty() ? FIds::NewGuidInBase64Url() : InDocId;

	// Fill document with template values
	if (TemplateType == EOwlExperimentTemplate::Default)
	{
		return FOwlExperimentStatics::CreateDefaultExperiment(DocId);
	}
	else if (TemplateType == EOwlExperimentTemplate::IAI)
	{
		return FOwlExperimentStatics::CreateUEExperiment(DocId);
	}
	return MakeShareable(new FOwlExperiment());
}

// Finish the pending events at the current time
void USLEventDataLogger::FinishPendingEvents(const float EndTime)
{
	for (const auto& PE : PendingEvents)
	{
		PE->End = EndTime;
		FinishedEvents.Emplace(PE);
	}
	PendingEvents.Empty();
}