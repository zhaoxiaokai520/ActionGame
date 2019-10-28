// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateFwd.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/SCompoundWidget.h"
#include "Models/IPFileManager.h"


/**
 * 
 */
class SPakMgrPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPakMgrPanel)
	{}
	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);

private:
	void InitializeControllers();
	
	TSharedRef<SDockTab> HandleTabManagerSpawnTab(const FSpawnTabArgs& Args, FName TabIdentifier) const;

protected:
	static void FillWindowMenu(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager);

private:
	/** Holds the tab manager that manages the front-end's tabs. */
	TSharedPtr<FTabManager> TabManager;

	/** Holds a pointer to the session manager. */
	TSharedPtr<IPFileManager> PFileManager;
};
