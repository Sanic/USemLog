// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#include "SLPickAndPlaceListener.h"
#include "SLManipulatorListener.h"
#include "Animation/SkeletalMeshActor.h"
#include "SLEntitiesManager.h"
#include "GameFramework/PlayerController.h"

// Sets default values for this component's properties
USLPickAndPlaceListener::USLPickAndPlaceListener()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	
	bIgnore = false;

	// State flags
	bIsInit = false;
	bIsStarted = false;
	bIsFinished = false;
	
	CurrGraspedObj = nullptr;
	EventCheck = ESLPaPStateCheck::NONE;
	UpdateFunctionPtr = &USLPickAndPlaceListener::Update_NONE;

	/* PickUp */
	bLiftOffHappened = false;

	/* PutDown */
	RecentMovementBuffer.Reserve(RecentMovementBufferSize);
}

// Dtor
USLPickAndPlaceListener::~USLPickAndPlaceListener()
{
	if (!bIsFinished)
	{
		Finish(true);
	}
}

// Init listener
bool USLPickAndPlaceListener::Init()
{
	if (bIgnore)
	{
		return false;
	}

	if (!bIsInit)
	{
		// Init the semantic entities manager
		if (!FSLEntitiesManager::GetInstance()->IsInit())
		{
			FSLEntitiesManager::GetInstance()->Init(GetWorld());
		}

		// Check that the owner is part of the semantic entities
		SemanticOwner = FSLEntitiesManager::GetInstance()->GetEntity(GetOwner());
		if (!SemanticOwner.IsSet())
		{
			UE_LOG(LogTemp, Error, TEXT("%s::%d Owner is not semantically annotated.."), *FString(__func__), __LINE__);
			return false;
		}

		// Init state
		EventCheck = ESLPaPStateCheck::NONE;
		UpdateFunctionPtr = &USLPickAndPlaceListener::Update_NONE;

		bIsInit = true;
		return true;
	}
	return false;
}

// Start listening to grasp events, update currently overlapping objects
void USLPickAndPlaceListener::Start()
{
	if (!bIsStarted && bIsInit)
	{
		// Subscribe for grasp notifications from sibling component
		if(SubscribeForGraspEvents())
		{
			// Start update callback (will directly be paused until a grasp is active)
			GetWorld()->GetTimerManager().SetTimer(UpdateTimerHandle, this, &USLPickAndPlaceListener::Update, UpdateRate, true);
			GetWorld()->GetTimerManager().PauseTimer(UpdateTimerHandle);
			
			// Mark as started
			bIsStarted = true;
		}
	}
}


// Stop publishing grasp events
void USLPickAndPlaceListener::Finish(float EndTime, bool bForced)
{
	if (!bIsFinished && (bIsInit || bIsStarted))
	{
		// Finish any active event
		FinishActiveEvent(EndTime);

		// Mark as finished
		bIsStarted = false;
		bIsInit = false;
		bIsFinished = true;
	}
}

// Subscribe for grasp events from sibling component
bool USLPickAndPlaceListener::SubscribeForGraspEvents()
{
	if(USLManipulatorListener* Sibling = CastChecked<USLManipulatorListener>(
		GetOwner()->GetComponentByClass(USLManipulatorListener::StaticClass())))
	{
		Sibling->OnBeginManipulatorGrasp.AddUObject(this, &USLPickAndPlaceListener::OnSLGraspBegin);
		Sibling->OnEndManipulatorGrasp.AddUObject(this, &USLPickAndPlaceListener::OnSLGraspEnd);

		return true;
	}
	return false;
}

// Get grasped objects contact shape component
ISLContactShapeInterface* USLPickAndPlaceListener::GetContactShapeComponent(AActor* Actor) const
{
	if(Actor)
	{
		for (const auto& C : Actor->GetComponents())
		{
			if(C->Implements<USLContactShapeInterface>())
			{
				if(ISLContactShapeInterface* CSI = Cast<ISLContactShapeInterface>(C))
				{
					return CSI;
				}
			}
		}
	}
	return nullptr;
}

// Called when grasp starts
void USLPickAndPlaceListener::OnSLGraspBegin(const FSLEntity& Self, AActor* Other, float Time, const FString& GraspType)
{
	if(CurrGraspedObj)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] Cannot set %s as grasped object.. manipulator is already grasping %s;"),
			*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName(), *CurrGraspedObj->GetName());
		return;
	}

	// Take into account only objects that have a contact shape component
	if(ISLContactShapeInterface* CSI = GetContactShapeComponent(Other))
	{
		CurrGraspedObj = Other;
		GraspedObjectContactShape = CSI;

		//UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] %s set as grasped object.."),
		//	*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName());

		PrevRelevantLocation = Other->GetActorLocation();
		PrevRelevantTime = GetWorld()->GetTimeSeconds();

		if(GraspedObjectContactShape->IsSupportedBySomething())
		{
			EventCheck = ESLPaPStateCheck::Slide;
			UpdateFunctionPtr = &USLPickAndPlaceListener::Update_Slide;
		}
		else
		{
			//UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] %s should be in a SupportedBy state.. aborting interaction.."),
			//	*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName());

			CurrGraspedObj = nullptr;
			GraspedObjectContactShape = nullptr;
			EventCheck = ESLPaPStateCheck::NONE;
			UpdateFunctionPtr = &USLPickAndPlaceListener::Update_NONE;
			return;
		}

		
		if(GetWorld()->GetTimerManager().IsTimerPaused(UpdateTimerHandle))
		{
			GetWorld()->GetTimerManager().UnPauseTimer(UpdateTimerHandle);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] This should not happen, timer should have been paused here.."),
				*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] %s does not have a ContactShapeInterface required to query the SupportedBy state..  aborting interaction.."),
			*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName());
	}
}

// Called when grasp ends
void USLPickAndPlaceListener::OnSLGraspEnd(const FSLEntity& Self, AActor* Other, float Time)
{
	if(CurrGraspedObj == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] This should not happen.. currently grasped object is nullptr while ending grasp with %s"),
			*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName());
		return;
	}

	if(CurrGraspedObj == Other)
	{
		CurrGraspedObj = nullptr;
		GraspedObjectContactShape = nullptr;
		EventCheck = ESLPaPStateCheck::NONE;
		UpdateFunctionPtr = &USLPickAndPlaceListener::Update_NONE;

		// Terminate active event
		FinishActiveEvent(Time);

		//UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] %s removed as grasped object.."),
		//	*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName());


		if(!GetWorld()->GetTimerManager().IsTimerPaused(UpdateTimerHandle))
		{
			GetWorld()->GetTimerManager().PauseTimer(UpdateTimerHandle);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] This should not happen, timer should have been running here.."),
				*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] End grasp with %s while %s is still grasped.. ignoring event.."),
			*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), *Other->GetName(), *CurrGraspedObj->GetName());
	}
}

// Object released, terminate active even
void USLPickAndPlaceListener::FinishActiveEvent(float CurrTime)
{
	if(EventCheck == ESLPaPStateCheck::Slide)
	{
		//UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## SLIDE ##############  [%f <--> %f]"),
		//	*FString(__func__), __LINE__, CurrTime, PrevRelevantTime, CurrTime);
		OnManipulatorSlideEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, CurrTime);
	}
	else if(EventCheck == ESLPaPStateCheck::PickUp)
	{
		if(bLiftOffHappened)
		{
			//UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## PICK UP ##############  [%f <--> %f]"),
			//	*FString(__func__), __LINE__, CurrTime, PrevRelevantTime, CurrTime);
			OnManipulatorSlideEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, CurrTime);
			bLiftOffHappened = false;
		}
	}
	else if(EventCheck == ESLPaPStateCheck::TransportOrPutDown)
	{
		//TODO
	}

	CurrGraspedObj = nullptr;
	EventCheck = ESLPaPStateCheck::NONE;
	UpdateFunctionPtr = &USLPickAndPlaceListener::Update_NONE;
}

// Backtrace and check if a put-down event happened
bool USLPickAndPlaceListener::HasPutDownEventHappened(const float CurrTime, const FVector& CurrObjLocation, uint32& OutPutDownEndIdx)
{
	OutPutDownEndIdx = RecentMovementBuffer.Num() - 1;
	while(OutPutDownEndIdx > 0 && CurrTime - RecentMovementBuffer[OutPutDownEndIdx].Key < PutDownMovementBacktrackDuration)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t\t\t [%f] [%f/%f] MinPutDownHeight"),
			*FString(__func__), __LINE__,
			GetWorld()->GetTimeSeconds(), 
			RecentMovementBuffer[OutPutDownEndIdx].Key,
			RecentMovementBuffer[OutPutDownEndIdx].Value.Z - CurrObjLocation.Z,
			MinPutDownHeight);

		if(RecentMovementBuffer[OutPutDownEndIdx].Value.Z - CurrObjLocation.Z > MinPutDownHeight)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f]  \t\t\t\t PUT DOWN HAPPENED"),
				*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
			return true;
		}
		OutPutDownEndIdx--;
	}
	UE_LOG(LogTemp, Error, TEXT("%s::%d [%f]  \t\t\t\t PUT DOWN HAS NOT HAPPENED"),
		*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
	return false;
}

// Update callback
void USLPickAndPlaceListener::Update()
{
	// Call the state update function
	(this->*UpdateFunctionPtr)();
}

/* Update functions*/
// Default update function
void USLPickAndPlaceListener::Update_NONE()
{
	UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] This should not happen.."), *FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
}

// Check for slide events
void USLPickAndPlaceListener::Update_Slide()
{
	if(CurrGraspedObj == nullptr || GraspedObjectContactShape == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] This should not happen.."), *FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
		return;
	}

	const FVector CurrObjLocation = CurrGraspedObj->GetActorLocation();
	const float CurrTime = GetWorld()->GetTimeSeconds();
	const float CurrDistXY = FVector::DistXY(PrevRelevantLocation, CurrObjLocation);

	// Sliding events can only end when the object is not supported by the surface anymore
	if(!GraspedObjectContactShape->IsSupportedBySomething())
	{
		UE_LOG(LogTemp, Error, TEXT("%s::%d [%f]  \t\t **** END SupportedBy ****"), *FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());

		// Check if enough distance and time has passed for a sliding event
		if(CurrDistXY > MinSlideDistXY && CurrTime - PrevRelevantTime > MinSlideDuration)
		{
			const float ExactSupportedByEndTime = GraspedObjectContactShape->GetLastSupportedByEndTime();

			UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## SLIDE ##############  [%f <--> %f]"),
				*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PrevRelevantTime, ExactSupportedByEndTime);

			// Broadcast event
			OnManipulatorSlideEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, ExactSupportedByEndTime);

			// Only update if they were part of the sliding event
			PrevRelevantTime = ExactSupportedByEndTime;
			PrevRelevantLocation = CurrObjLocation;
		}

		bLiftOffHappened = false;
		EventCheck = ESLPaPStateCheck::PickUp;
		UpdateFunctionPtr = &USLPickAndPlaceListener::Update_PickUp;
	}
}

// Check for pick-up events
void USLPickAndPlaceListener::Update_PickUp()
{
	const FVector CurrObjLocation = CurrGraspedObj->GetActorLocation();
	const float CurrTime = GetWorld()->GetTimeSeconds();

	if(!GraspedObjectContactShape->IsSupportedBySomething())
	{
		if(bLiftOffHappened)
		{
			if(CurrObjLocation.Z - LiftOffLocation.Z > MaxPickUpHeight ||
				FVector::DistXY(LiftOffLocation, CurrObjLocation) > MaxPickUpDistXY)
			{

				UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## PICK UP ##############  [%f <--> %f]"),
					*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PrevRelevantTime, CurrTime);
				// Broadcast event
				OnManipulatorPickUpEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, CurrTime);

				// Start checking for the next possible events
				bLiftOffHappened = false;
				PrevRelevantTime = CurrTime;
				PrevRelevantLocation = CurrObjLocation;
				EventCheck = ESLPaPStateCheck::TransportOrPutDown;
				UpdateFunctionPtr = &USLPickAndPlaceListener::Update_TransportOrPutDown;
			}
		}
		else if(CurrObjLocation.Z - PrevRelevantLocation.Z > MinPickUpHeight)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f]  \t **** LiftOFF **** \t\t\t\t\t\t\t\t LIFTOFF"), *FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());

			// This is not going to be the start time of the PickUp event, we use the SupportedBy end time
			// we save the LiftOffLocation to check against the ending of the PickUp event by comparing distances against
			bLiftOffHappened = true;
			LiftOffLocation = CurrObjLocation;
		}
		else if(FVector::DistXY(LiftOffLocation, PrevRelevantLocation) > MaxPickUpDistXY)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f]  \t **** Skip PickUp **** \t\t\t\t\t\t\t\t SKIP PICKUP"), *FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
			EventCheck = ESLPaPStateCheck::TransportOrPutDown;
			UpdateFunctionPtr = &USLPickAndPlaceListener::Update_TransportOrPutDown;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f] \t\t **** START SupportedBy ****"), *FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());

		if(bLiftOffHappened)
		{
			UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## PICK UP ##############  [%f <--> %f]"),
				*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PrevRelevantTime, CurrTime);
			OnManipulatorPickUpEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, CurrTime);
		}

		// Start checking for next event
		PrevRelevantTime = CurrTime;
		PrevRelevantLocation = CurrObjLocation;
		EventCheck = ESLPaPStateCheck::Slide;
		UpdateFunctionPtr = &USLPickAndPlaceListener::Update_Slide;
	}
}

// Check for put-down or transport events
void USLPickAndPlaceListener::Update_TransportOrPutDown()
{
	const float CurrTime = GetWorld()->GetTimeSeconds();
	const FVector CurrObjLocation = CurrGraspedObj->GetActorLocation();

	if(GraspedObjectContactShape->IsSupportedBySomething())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s::%d [%f]  \t\t **** START SupportedBy ****"), *FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());

		// Check for the PutDown movement start time
		uint32 PutDownEndIdx = 0;
		if(HasPutDownEventHappened(CurrTime, CurrObjLocation, PutDownEndIdx))
		{
			float PutDownStartTime = -1.f;
			while(PutDownEndIdx > 0)
			{
				// Check when the 
				if(RecentMovementBuffer[PutDownEndIdx].Value.Z - CurrObjLocation.Z > MaxPutDownHeight
					|| FVector::Distance(RecentMovementBuffer[PutDownEndIdx].Value, CurrObjLocation) > MaxPutDownDistXY)
				{
					PutDownStartTime = RecentMovementBuffer[PutDownEndIdx].Key;

					UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## TRANSPORT ##############  [%f <--> %f]"),
						*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PrevRelevantTime, PutDownStartTime);
					OnManipulatorTransportEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, PutDownStartTime);


					UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## PUT DOWN ##############  [%f <--> %f]"),
						*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PutDownStartTime, CurrTime);
					OnManipulatorPutDownEvent.Broadcast(SemanticOwner, CurrGraspedObj, PutDownStartTime, CurrTime);
					break;
				}
				PutDownEndIdx--;
			}

			// If the limits are not crossed in the buffer the oldest available time is used (TODO, or should we ignore the action?)
			if(PutDownStartTime < 0)
			{
				UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] The limits were not crossed in the available data in the buffer, the oldest available time is used"),
					*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds());
				PutDownStartTime = RecentMovementBuffer[0].Key;

				UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## TRANSPORT ##############  [%f <--> %f]"),
					*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PrevRelevantTime, PutDownStartTime);
				OnManipulatorPutDownEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, PutDownStartTime);

				UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## PUT DOWN ##############  [%f <--> %f]"),
					*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PutDownStartTime, CurrTime);
				OnManipulatorPutDownEvent.Broadcast(SemanticOwner, CurrGraspedObj, PutDownStartTime, CurrTime);
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("%s::%d [%f] \t ############## TRANSPORT ##############  [%f <--> %f]"),
				*FString(__func__), __LINE__, GetWorld()->GetTimeSeconds(), PrevRelevantTime, CurrTime);
			OnManipulatorTransportEvent.Broadcast(SemanticOwner, CurrGraspedObj, PrevRelevantTime, CurrTime);
		}

		RecentMovementBuffer.Reset(RecentMovementBufferSize);

		PrevRelevantTime = CurrTime;
		PrevRelevantLocation = CurrObjLocation;
		EventCheck = ESLPaPStateCheck::Slide;
		UpdateFunctionPtr = &USLPickAndPlaceListener::Update_Slide;
	}
	else
	{
		// Cache recent movements 
		if(RecentMovementBuffer.Num() > 1)
		{
			RecentMovementBuffer.Push(TPair<float, FVector>(CurrTime, CurrObjLocation));

			// Remove values older than RecentMovementBufferDuration
			int32 Count = 0;
			while(RecentMovementBuffer.Last().Key - RecentMovementBuffer[Count].Key > RecentMovementBufferDuration)
			{
				Count++;
			}
			if(Count > 0)
			{
				RecentMovementBuffer.RemoveAt(0, Count, false);
			}
		}
		else
		{
			RecentMovementBuffer.Push(TPair<float, FVector>(CurrTime, CurrObjLocation));
		}
	}
}

