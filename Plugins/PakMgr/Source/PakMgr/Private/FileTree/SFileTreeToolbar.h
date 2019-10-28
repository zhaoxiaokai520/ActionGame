// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Framework/Commands/UICommandList.h"

#define LOCTEXT_NAMESPACE "SFileTreeToolbar"

/**
 * Implements the device toolbar widget.
 */
class SFileTreeToolbar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SFileTreeToolbar) { }
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
