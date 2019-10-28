// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SPakManagerToolbar.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/MultiBox/MultiBoxDefs.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "EditorStyleSet.h"
#include "PakManager/PakManagerCommands.h"


/* SSessionConsoleToolbar interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPakManagerToolbar::Construct(const FArguments& InArgs, const TSharedRef<FUICommandList>& CommandList)
{
	FPakManagerCommands::Register();

	// create the toolbar
	FToolBarBuilder Toolbar(CommandList, FMultiBoxCustomization::None);
	{
		Toolbar.AddToolBarButton(FPakManagerCommands::Get().GenPaks);
		Toolbar.AddSeparator();
		Toolbar.AddToolBarButton(FPakManagerCommands::Get().GenMani);
		Toolbar.AddToolBarButton(FPakManagerCommands::Get().MaxSize);
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
