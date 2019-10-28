// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PakMgrCommands.h"

#define LOCTEXT_NAMESPACE "FPakMgrModule"

void FPakMgrCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "PakMgr", "Bring up PakMgr window", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
