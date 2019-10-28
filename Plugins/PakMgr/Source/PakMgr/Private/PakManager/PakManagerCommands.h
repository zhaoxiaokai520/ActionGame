// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "CoreMinimal.h"
#include "Framework/Commands/InputChord.h"
#include "EditorStyleSet.h"
#include "PakMgrStyle.h"
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "PakManagerCommands"

/**
 * The device details commands.
 */
class FPakManagerCommands
	: public TCommands<FPakManagerCommands>
{
public:

	/** Default constructor. */
	FPakManagerCommands()
		: TCommands<FPakManagerCommands>(
			"PakManager",
			NSLOCTEXT("Contexts", "PakManager", "Pak Manager"),
			NAME_None, FPakMgrStyle::GetStyleSetName()
		)
	{ }

public:

	// TCommands interface

	virtual void RegisterCommands() override
	{
		UI_COMMAND(GenPaks, "GenPaks", "Generate pak packages", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(GenMani, "GenMani", "Generate pak dependency manifest", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(MaxSize, "MaxSize", "max pak size", EUserInterfaceActionType::ToggleButton, FInputChord());

	}

public:

	TSharedPtr<FUICommandInfo> GenPaks;
	TSharedPtr<FUICommandInfo> GenMani;
	TSharedPtr<FUICommandInfo> MaxSize;
};

#undef LOCTEXT_NAMESPACE
