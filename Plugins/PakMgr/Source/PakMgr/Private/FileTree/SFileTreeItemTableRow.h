// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/Attribute.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Styling/SlateColor.h"
#include "Widgets/SWidget.h"
#include "Layout/Margin.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Views/STableRow.h"
#include "Models/FileItemInfo.h"
#include "SlateOptMacros.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Images/SImage.h"

class Error;

/**
 * Implements a row widget for the session console log.
 */
class SFileTreeitemTableRow
	: public SMultiColumnTableRow<TSharedPtr<FFileItemInfo>>
{
public:

	SLATE_BEGIN_ARGS(SFileTreeitemTableRow) { }
		SLATE_ATTRIBUTE(FText, HighlightText)
		SLATE_ARGUMENT(TSharedPtr<FFileItemInfo>, FileItemInfo)
	SLATE_END_ARGS()

public:

	/**
	 * Constructs the widget.
	 *
	 * @param InArgs The construction arguments.
	 */
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		HighlightText = InArgs._HighlightText;
		FileItemInfo = InArgs._FileItemInfo;

		SMultiColumnTableRow<TSharedPtr<FFileItemInfo>>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

public:

	// SMultiColumnTableRow interface

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == "Instance")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 1.0f, 4.0f, 0.0f))
				.HAlign(HAlign_Left)
				[
					SNew(SBorder)
						.BorderBackgroundColor(this, &SFileTreeitemTableRow::HandleGetBorderColor)
						//.BorderImage(FEditorStyle::GetBrush("ErrorReporting.Box"))
						.ColorAndOpacity(FLinearColor(0.25f, 0.25f, 0.25f))
						.Padding(FMargin(6.0f, 3.0f))
						.Content()
						[
							SNew(STextBlock)
								//.Font(FEditorStyle::GetFontStyle("BoldFont"))
								.Text(FText::FromString(FileItemInfo->InstanceName))
						]
				];
		}
		else if (ColumnName == "Message")
		{
			return SNew(SBox)
				.Padding(FMargin(4.0f, 0.0f))
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
						.ColorAndOpacity(HandleGetTextColor())
						.HighlightText(HighlightText)
						.Text(FText::FromString(FileItemInfo->Text.Replace(TEXT("\n"), TEXT(" | ")).Replace(TEXT("\r"), TEXT(""))))
				];
		}
		else if (ColumnName == "TimeSeconds")
		{
			static const FNumberFormattingOptions FormatOptions = FNumberFormattingOptions()
				.SetMinimumFractionalDigits(3)
				.SetMaximumFractionalDigits(3);

			return SNew(SBox)
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				.Padding(FMargin(0.0f, 0.0f, 8.0f, 0.0f))
				[
					SNew(STextBlock)
						.ColorAndOpacity(HandleGetTextColor())
						.Text(FText::AsNumber(FileItemInfo->TimeSeconds, &FormatOptions))
				];
		}
		else if (ColumnName == "Verbosity")
		{
			const FSlateBrush* Icon;

			if ((FileItemInfo->Verbosity == ELogVerbosity::Error) ||
				(FileItemInfo->Verbosity == ELogVerbosity::Fatal))
			{
				//Icon = FEditorStyle::GetBrush("Icons.Error");
			}
			else if (FileItemInfo->Verbosity == ELogVerbosity::Warning)
			{
				//Icon = FEditorStyle::GetBrush("Icons.Warning");
			}
			else
			{
				//Icon = FEditorStyle::GetBrush("Icons.Info");
			}

			return SNew(SBox)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
						.Image(Icon)
				];
		}

		return SNullWidget::NullWidget;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:

	/** Gets the border color for this row. */
	FSlateColor HandleGetBorderColor() const
	{
		return FLinearColor((GetTypeHash(FileItemInfo->InstanceId) & 0xff) * 360.0f / 256.0f, 0.8f, 0.3f, 1.0f).HSVToLinearRGB();
	}

	/** Gets the text color for this log entry. */
	FSlateColor HandleGetTextColor() const
	{
		if ((FileItemInfo->Verbosity == ELogVerbosity::Error) ||
			(FileItemInfo->Verbosity == ELogVerbosity::Fatal))
		{
			return FLinearColor::Red;
		}
		else if (FileItemInfo->Verbosity == ELogVerbosity::Warning)
		{
			return FLinearColor::Yellow;
		}
		else
		{
			return FSlateColor::UseForeground();
		}
	}

private:

	/** Holds the highlight string for the log message. */
	TAttribute<FText> HighlightText;

	/** Holds a reference to the log message that is displayed in this row. */
	TSharedPtr<FFileItemInfo> FileItemInfo;
};
