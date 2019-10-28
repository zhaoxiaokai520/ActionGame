// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Browser/SContentBrowserCommandBar.h"
#include "Widgets/SBoxPanel.h"


#define LOCTEXT_NAMESPACE "SContentBrowserCommandBar"


/* SContentBrowserCommandBar interface
 *****************************************************************************/

void SContentBrowserCommandBar::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		SNew(SHorizontalBox)
	];
}


#undef LOCTEXT_NAMESPACE
