// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SFileTreeToolbar.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "FileTree/FileTreeCommands.h"


/* SSessionConsoleToolbar interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SFileTreeToolbar::Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& CommandList)
{
	FFileTreeCommands::Register();

	// create the toolbar
	FToolBarBuilder Toolbar(CommandList, FMultiBoxCustomization::None);
	{
		Toolbar.AddToolBarButton(FFileTreeCommands::Get().SelCon);
		Toolbar.AddSeparator();
		Toolbar.AddToolBarButton(FFileTreeCommands::Get().AddM);
		Toolbar.AddToolBarButton(FFileTreeCommands::Get().GenRef);
		Toolbar.AddToolBarButton(FFileTreeCommands::Get().SortRef);
		Toolbar.AddToolBarButton(FFileTreeCommands::Get().SearchFiles);
	}

	ChildSlot
	[
		SNew(SBorder)
			//.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0.0f)
			[
				Toolbar.MakeWidget()
			]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION
