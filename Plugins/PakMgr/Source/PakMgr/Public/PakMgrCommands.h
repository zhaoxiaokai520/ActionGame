// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "PakMgrStyle.h"

class FPakMgrCommands : public TCommands<FPakMgrCommands>
{
public:

	FPakMgrCommands()
		: TCommands<FPakMgrCommands>(TEXT("PakMgr"), NSLOCTEXT("Contexts", "PakMgr", "PakMgr Plugin"), NAME_None, FPakMgrStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > OpenPluginWindow;
};