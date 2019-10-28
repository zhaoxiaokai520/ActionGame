// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/Commands/UICommandList.h"

#define LOCTEXT_NAMESPACE "SPakManagerToolbar"

/**
 * Implements the device toolbar widget.
 */
class SPakManagerToolbar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SPakManagerToolbar) { }
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 * @param CommandList The command list to use.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<FUICommandList>& CommandList );
};


#undef LOCTEXT_NAMESPACE
