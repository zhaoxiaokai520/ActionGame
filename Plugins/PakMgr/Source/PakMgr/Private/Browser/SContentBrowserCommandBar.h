// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Implements the session browser's command bar widget.
 */
class SContentBrowserCommandBar
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SContentBrowserCommandBar) { }
	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs The declaration data for this widget.
	 */
	void Construct( const FArguments& InArgs );
};
