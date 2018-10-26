// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "EventData/SLSupportedByEvent.h"
#include "SLOwlExperimentStatics.h"

	// Default constructor
FSLSupportedByEvent::FSLSupportedByEvent()
{
}

// Constructor with initialization 
FSLSupportedByEvent::FSLSupportedByEvent(const FString& InId, const float InStart, const float InEnd, const uint64 InPairId,
	const uint32 InSupportedObjId, const FString& InSupportedObjSemId, const FString& InSupportedObjClass,
	const uint32 InSupportingObjId, const FString& InSupportingObjSemId, const FString& InSupportingObjClass) :
	ISLEvent(InId, InStart, InEnd), PairId(InPairId),
	SupportedObjId(InSupportedObjId), SupportedObjSemId(InSupportedObjSemId), SupportedObjClass(InSupportedObjClass),
	SupportingObjId(InSupportingObjId), SupportingObjSemId(InSupportingObjSemId), SupportingObjClass(InSupportingObjClass)
{
}

// Constructor with initialization without end time 
FSLSupportedByEvent::FSLSupportedByEvent(const FString& InId, const float InStart, const uint64 InPairId,
	const uint32 InSupportedObjId, const FString& InSupportedObjSemId, const FString& InSupportedObjClass,
	const uint32 InSupportingObjId, const FString& InSupportingObjSemId, const FString& InSupportingObjClass) :
	ISLEvent(InId, InStart), PairId(InPairId),
	SupportedObjId(InSupportedObjId), SupportedObjSemId(InSupportedObjSemId), SupportedObjClass(InSupportedObjClass),
	SupportingObjId(InSupportingObjId), SupportingObjSemId(InSupportingObjSemId), SupportingObjClass(InSupportingObjClass)
{
}
/* Begin ISLEvent interface */
// Get an owl representation of the event
FOwlNode FSLSupportedByEvent::ToOwlNode() const
{
	// Create the contact event node
	FOwlNode EventIndividual = FSLOwlExperimentStatics::CreateEventIndividual(
		"log", Id, "SupportedBySituation");
	EventIndividual.AddChildNode(FSLOwlExperimentStatics::CreateStartTimeProperty("log", Start));
	EventIndividual.AddChildNode(FSLOwlExperimentStatics::CreateEndTimeProperty("log", End));
	EventIndividual.AddChildNode(FSLOwlExperimentStatics::CreateIsSupportedProperty("log", SupportedObjSemId));
	EventIndividual.AddChildNode(FSLOwlExperimentStatics::CreateIsSupportingProperty("log", SupportingObjSemId));
	return EventIndividual;
}

// Add the owl representation of the event to the owl document
void FSLSupportedByEvent::AddToOwlDoc(FOwlDoc* OutDoc)
{
	// Add timepoint and object individuals
	// We know that the document is of type FOwlExperiment,
	// we cannot use the safer dynamic_cast because RTTI is not enabled by default
	// if (FOwlEvents* EventsDoc = dynamic_cast<FOwlEvents*>(OutDoc))
	FOwlExperiment* EventsDoc = static_cast<FOwlExperiment*>(OutDoc);
	EventsDoc->AddTimepointIndividual(
		Start, FSLOwlExperimentStatics::CreateTimepointIndividual("log", Start));
	EventsDoc->AddTimepointIndividual(
		End, FSLOwlExperimentStatics::CreateTimepointIndividual("log", End));
	EventsDoc->AddObjectIndividual(
		SupportedObjId, FSLOwlExperimentStatics::CreateObjectIndividual("log", SupportedObjSemId, SupportedObjClass));
	EventsDoc->AddObjectIndividual(
		SupportingObjId, FSLOwlExperimentStatics::CreateObjectIndividual("log", SupportingObjSemId, SupportingObjClass));
	OutDoc->AddIndividual(ToOwlNode());
}

// Get event context data as string (ToString equivalent)
FString FSLSupportedByEvent::Context() const
{
	return FString::Printf(TEXT("SupportedByEvent - %lld"), PairId);
}

// Get the tooltip data
FString FSLSupportedByEvent::Tooltip() const
{
	return FString::Printf(TEXT("\'SupportingO\',\'%s\',\'Id\',\'%s\',\'SupportedO\',\'%s\',\'Id\',\'%s\',\'Id\',\'%s\'"),
		*SupportingObjClass, *SupportingObjSemId, *SupportedObjClass, *SupportedObjSemId, *Id);
}
/* End ISLEvent interface */