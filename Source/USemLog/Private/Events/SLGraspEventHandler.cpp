// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "Events/SLGraspEventHandler.h"
#include "SLEntitiesManager.h"
#include "SLManipulatorListener.h"

// UUtils
#include "Ids.h"


// Set parent
void FSLGraspEventHandler::Init(UObject* InParent)
{
	if (!bIsInit)
	{
		// Make sure the mappings singleton is initialized (the handler uses it)
		if (!FSLEntitiesManager::GetInstance()->IsInit())
		{
			FSLEntitiesManager::GetInstance()->Init(InParent->GetWorld());
		}

		// Check if parent is of right type
		Parent = Cast<USLManipulatorListener>(InParent);
		if (Parent)
		{
			// Mark as initialized
			bIsInit = true;
		}
	}
}

// Bind to input delegates
void FSLGraspEventHandler::Start()
{
	if (!bIsStarted && bIsInit)
	{
		// Subscribe to the forwarded semantically annotated grasping broadcasts
		Parent->OnBeginManipulatorGrasp.AddRaw(this, &FSLGraspEventHandler::OnSLGraspBegin);
		Parent->OnEndManipulatorGrasp.AddRaw(this, &FSLGraspEventHandler::OnSLGraspEnd);

		// Mark as started
		bIsStarted = true;
	}
}

// Terminate listener, finish and publish remaining events
void FSLGraspEventHandler::Finish(float EndTime, bool bForced)
{
	if (!bIsFinished && (bIsInit || bIsStarted))
	{
		// Let parent first publish any pending (delayed) events
		if(!Parent->IsFinished())
		{
			Parent->Finish();
		}
		
		FinishAllEvents(EndTime);

		// TODO use dynamic delegates to be able to unbind from them
		// https://docs.unrealengine.com/en-us/Programming/UnrealArchitecture/Delegates/Dynamic
		// this would mean that the handler will need to inherit from UObject		

		// Mark finished
		bIsStarted = false;
		bIsInit = false;
		bIsFinished = true;
	}
}

// Start new grasp event
void FSLGraspEventHandler::AddNewEvent(const FSLEntity& Self, const FSLEntity& Other, float StartTime, const FString& InType)
{
	// Start a semantic grasp event
	TSharedPtr<FSLGraspEvent> Event = MakeShareable(new FSLGraspEvent(
		FIds::NewGuidInBase64Url(), StartTime,
		FIds::PairEncodeCantor(Self.Obj->GetUniqueID(), Other.Obj->GetUniqueID()),
		Self, Other, InType));
	// Add event to the pending array
	StartedEvents.Emplace(Event);
}

// Publish finished event
bool FSLGraspEventHandler::FinishEvent(AActor* Other, float EndTime)
{
	// Use iterator to be able to remove the entry from the array
	for (auto EventItr(StartedEvents.CreateIterator()); EventItr; ++EventItr)
	{
		// It is enough to compare against the other id when searching
		if ((*EventItr)->Item.Obj == Other)
		{
			// Ignore short events
			if ((EndTime - (*EventItr)->Start) > GraspEventMin)
			{
				// Set end time and publish event
				(*EventItr)->End = EndTime;
				OnSemanticEvent.ExecuteIfBound(*EventItr);
			}
			// Remove event from the pending list
			EventItr.RemoveCurrent();
			return true;
		}
	}
	return false;
}

// Terminate and publish pending events (this usually is called at end play)
void FSLGraspEventHandler::FinishAllEvents(float EndTime)
{
	// Finish events
	for (auto& Ev : StartedEvents)
	{
		// Ignore short events
		if ((EndTime - Ev->Start) > GraspEventMin)
		{
			// Set end time and publish event
			Ev->End = EndTime;
			OnSemanticEvent.ExecuteIfBound(Ev);
		}
	}
	StartedEvents.Empty();
}


// Event called when a semantic grasp event begins
void FSLGraspEventHandler::OnSLGraspBegin(const FSLEntity& Self, AActor* Other, float Time, const FString& Type)
{
	// Check that the objects are semantically annotated
	FSLEntity OtherItem = FSLEntitiesManager::GetInstance()->GetEntity(Other);
	if (OtherItem.IsSet())
	{
		AddNewEvent(Self, OtherItem, Time, Type);
	}
}

// Event called when a semantic grasp event ends
void FSLGraspEventHandler::OnSLGraspEnd(const FSLEntity& Self, AActor* Other, float Time)
{
	FinishEvent(Other, Time);
}
