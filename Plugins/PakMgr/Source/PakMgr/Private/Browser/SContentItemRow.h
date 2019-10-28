// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/Guid.h"
#include "Misc/Attribute.h"
#include "InputCoreTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Layout/Margin.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/SExpanderArrow.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Misc/App.h"
#include "SlateOptMacros.h"
#include "EditorStyleSet.h"
#include "Models/ContentItemInfo.h"

#define LOCTEXT_NAMESPACE "SContentItemRow"

/**
 * Implements a row widget for the Content browser tree.
 */
class SContentItemRow
	: public STableRow<TSharedPtr<FContentItemInfo>>
{
public:

	SLATE_BEGIN_ARGS(SContentItemRow) { }
		SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_ARGUMENT(TSharedPtr<STableViewBase>, OwnerTableView)
		SLATE_ARGUMENT(TSharedPtr<FContentItemInfo>, ContentItem)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 */
	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		HighlightText = InArgs._HighlightText;
		Item = InArgs._ContentItem;

		ChildSlot
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(STextBlock)
						.HighlightText(InArgs._HighlightText)
					.Text(FText::FromString(*InArgs._ContentItem->Name))
					]
			];

		STableRow<TSharedPtr<FContentItemInfo>>::ConstructInternal(
			STableRow<TSharedPtr<FContentItemInfo>>::FArguments()
			.ShowSelection(false)
			.Style(FEditorStyle::Get(), "DetailsView.TreeView.TableRow"),
			InOwnerTableView
		);
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

protected:

	// SWidget overrides

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			ToggleExpansion();

			return FReply::Handled();
		}
		else
		{
			return FReply::Unhandled();
		}
	}

private:

	/** Callback for getting the background image of the row's border. */
	const FSlateBrush* HandleBorderBackgroundImage() const
	{
		//if (IsHovered())
		//{
		//	return IsItemExpanded()
		//		? FEditorStyle::GetBrush("DetailsView.CategoryTop_Hovered")
		//		: FEditorStyle::GetBrush("DetailsView.CollapsedCategory_Hovered");
		//}
		//else
		//{
		//	return IsItemExpanded()
		//		? FEditorStyle::GetBrush("DetailsView.CategoryTop")
		//		: FEditorStyle::GetBrush("DetailsView.CollapsedCategory");
		//}
		return nullptr;
	}

	/** Callback for getting the text of the row border's tool tip. */
	FText HandleBorderToolTipText() const
	{
		FTextBuilder ToolTipTextBuilder;

		return ToolTipTextBuilder.ToText();
	}

	/** Callback for getting the name of the session. */
	FText HandleSessionNameText() const
	{
		return LOCTEXT("Session", "Name");
	}

private:

	/** The highlight string for this row. */
	TAttribute<FText> HighlightText;

	/** A reference to the tree item that is displayed in this row. */
	TSharedPtr<FContentItemInfo> Item;
};


#undef LOCTEXT_NAMESPACE
