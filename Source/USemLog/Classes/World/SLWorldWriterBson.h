// Copyright 2017-2020, Institute for Artificial Intelligence - University of Bremen
// Author: Andrei Haidu (http://haidu.eu)

#pragma once

#include "CoreMinimal.h"
#include "ISLWorldWriter.h"
#if SL_WITH_LIBMONGO_CXX
THIRD_PARTY_INCLUDES_START
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/types.hpp>
THIRD_PARTY_INCLUDES_END
#endif //SL_WITH_LIBMONGO_CXX

/**
 * Raw data logger to bson format
 */
class FSLWorldWriterBson : public ISLWorldWriter
{
public:
	// Default constr
	FSLWorldWriterBson();

	// Init constructor
	FSLWorldWriterBson(const FSLWorldWriterParams& InParams);

	// Destr
	virtual ~FSLWorldWriterBson();

	// Init
	virtual void Init(const FSLWorldWriterParams& InParams) override;

	// Finish
	virtual void Finish() override;

	// Called to write the data
	virtual void Write(float Timestamp,
		TArray<TSLEntityPreviousPose<AActor>>& ActorEntities,
		TArray<TSLEntityPreviousPose<USceneComponent>>& ComponentEntities,
		TArray<TSLEntityPreviousPose<USLSkeletalDataComponent>>& SkeletalEntities,
		FSLGazeData& GazeData,
		bool bCheckAndRemoveInvalidEntities = true) override;

private:
	// Set the file handle for the logger
	bool SetFileHandle(const FString& LogDirectory, const FString& InEpisodeId);

#if SL_WITH_LIBMONGO_CXX
	//// Add actors
	//void AddActors(bson_t& OutBsonEntitiesArr);

	//// Add components
	//void AddComponents(bson_t& OutBsonEntitiesArr);

	//// Get entry as Bson object
	//bson_t GetAsBsonEntry(const FString& InId,
	//	const FString& InClass,
	//	const FVector& InLoc,
	//	const FQuat& InQuat);

	//// Write entry to file
	//void WriteData(uint8* memorybuffer, int64 bufferlen);
#endif //SL_WITH_LIBMONGO_CXX

	// File handle to write the raw data to file
	IFileHandle* FileHandle;
};
