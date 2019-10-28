// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/InputChord.h"
#include "EditorStyleSet.h"
#include "PakMgrStyle.h"
#include "Framework/Commands/Commands.h"

#define LOCTEXT_NAMESPACE "SessionConsoleCommands"

/**
 * The device details commands.
 */
class FFileTreeCommands
	: public TCommands<FFileTreeCommands>
{
public:

	/** Default constructor. */
	FFileTreeCommands()
		: TCommands<FFileTreeCommands>(
			"FileTree",
			NSLOCTEXT("Contexts", "FileTree", "File Tree"),
			NAME_None, FPakMgrStyle::GetStyleSetName()
		)
	{ }

public:

	// TCommands interface

	virtual void RegisterCommands() override
	{
		UI_COMMAND(SelCon, "SelCon", "select content path", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(AddM, "AddM", "Pack module flag is used to seperate paks", EUserInterfaceActionType::Button, FInputChord());
		UI_COMMAND(GenRef, "GenRef", "prepare file list for pack paks", EUserInterfaceActionType::ToggleButton, FInputChord());
		UI_COMMAND(SortRef, "SortRef", "sort file list", EUserInterfaceActionType::ToggleButton, FInputChord());
		UI_COMMAND(SearchFiles, "SearchFiles", "search and filter files", EUserInterfaceActionType::ToggleButton, FInputChord());

	}

public:

	TSharedPtr<FUICommandInfo> SelCon;
	TSharedPtr<FUICommandInfo> AddM;
	TSharedPtr<FUICommandInfo> GenRef;
	TSharedPtr<FUICommandInfo> SortRef;
	TSharedPtr<FUICommandInfo> SearchFiles;
};

#undef LOCTEXT_NAMESPACE
