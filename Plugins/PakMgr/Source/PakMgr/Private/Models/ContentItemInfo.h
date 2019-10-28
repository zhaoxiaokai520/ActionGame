// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "CoreMinimal.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

/**
 * Structure for Content item.
 */
struct FContentItemInfo
{
	/** Holds the log category. */
	FName Category;

	/** Holds the identifier of the engine instance that generated this log message. */
	FGuid InstanceId;

	/** Holds the name of the engine instance that generated this log message. */
	FString InstanceName;

	/** Holds the message text. */
	FString Text;

	/** Holds the time at which the message was generated. */
	FString Name;

	/** Holds the number of seconds from the start of the instance at which the message was generated. */
	double TimeSeconds;

	/** Holds the verbosity type. */
	ELogVerbosity::Type Verbosity;

public:

	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InInstanceId The identifier of the instance that generated the log message.
	 * @param InInstanceName The name of the engine instance that generated the log message.
	 * @param InTimeSeconds The number of seconds from the start of the instance at which the message was generated.
	 * @param InText The message text.
	 * @param InVerbosity The verbosity type.
	 * @param InCategory The log category.
	 */
	FContentItemInfo(const FString& InText)
		//: Category(InCategory)
		//, InstanceId(InInstanceId)
		//, InstanceName(InInstanceName)
		: Text(InText)
		//, TimeSeconds(InTimeSeconds)
		//, Verbosity(InVerbosity)
	{ 
		Name = FPaths::GetCleanFilename(InText);
	}

public:

	/**
	 * Implements a predicate to compare two log messages by log time.
	 */
	struct TimeComparer
	{
		bool operator()(const TSharedPtr<FContentItemInfo>& A, const TSharedPtr<FContentItemInfo>& B) const
		{
			return A->Text.Compare(B->Text) <= 0;
		}
	};
};
