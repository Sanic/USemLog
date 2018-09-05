// Copyright 2018, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "OwlDoc.h"

/**
* Abstract class ensuring every event can be represented as an Owl Node;
*/
class ISLEvent
{
public:
	// Default constructor
	ISLEvent() {};
	
	// Init constructor
	ISLEvent(const FString& InId, const float InStart, const float InEnd) 
		: Id(InId), Start(InStart), End(InEnd) {};

	// Init without end constructor
	ISLEvent(const FString& InId, const float InStart)
		: Id(InId), Start(InStart) {};

	// Virtual destructor
	virtual ~ISLEvent() {};

	// Unique id of the event
	FString Id;

	// Start time of the event
	float Start;

	// End time of the event
	float End;

	// Create owl representation of the event
	virtual FOwlNode ToOwlNode() const = 0;

	// Add the owl representation of the event to the owl document
	virtual void AddToOwlDoc(FOwlDoc* OutDoc) = 0;

	// Get event context data as string (ToString equivalent)
	virtual FString Context()  const = 0;
};