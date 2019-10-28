// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SContentBrowserFilterBar.h"
#include "SlateOptMacros.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SCheckBox.h"
#include "EditorStyleSet.h"
#include "Widgets/Input/SSearchBox.h"

#define LOCTEXT_NAMESPACE "SContentBrowserFilterBar"


/* SSessionConsoleFilterBar interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SContentBrowserFilterBar::Construct(const FArguments& InArgs)
{
	OnFilterChanged = InArgs._OnFilterChanged;

	// initialize verbosity filters
	AddVerbosityFilter(ELogVerbosity::Fatal, LOCTEXT("FatalVerbosityFilterTooltip", "Fatal errors").ToString(), "Icons.Error");
	AddVerbosityFilter(ELogVerbosity::Error, LOCTEXT("ErrorVerbosityFilterTooltip", "Errors").ToString(), "Icons.Error");
	AddVerbosityFilter(ELogVerbosity::Warning, LOCTEXT("WarningVerbosityFilterTooltip", "Warnings").ToString(), "Icons.Warning");
	AddVerbosityFilter(ELogVerbosity::Log, LOCTEXT("LogVerbosityFilterTooltip", "Log Messages").ToString(), "Icons.Info");
	AddVerbosityFilter(ELogVerbosity::Display, LOCTEXT("DisplayVerbosityFilterTooltip", "Display Messages").ToString(), "Icons.Info");
	AddVerbosityFilter(ELogVerbosity::Verbose, LOCTEXT("VerboseVerbosityFilterTooltip", "Verbose Messages").ToString(), "Icons.Info");
	AddVerbosityFilter(ELogVerbosity::VeryVerbose, LOCTEXT("VeryVerboseVerbosityFilterTooltip", "Very Verbose Messages").ToString(), "Icons.Info");

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			[
				// search box
				SAssignNew(FilterStringTextBox, SSearchBox)
					.HintText(LOCTEXT("SearchBoxHint", "Search log messages"))
					.OnTextChanged(this, &SContentBrowserFilterBar::HandleFilterStringTextChanged)
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				// highlight only check box
				SAssignNew(HighlightOnlyCheckBox, SCheckBox)
					.Padding(FMargin(6.0, 2.0))
					.OnCheckStateChanged(this, &SContentBrowserFilterBar::HandleHighlightOnlyCheckBoxCheckStateChanged)
					.ToolTipText(LOCTEXT("HighlightOnlyCheckBoxTooltip", "Only highlight the search text instead of filtering the list of log messages"))
					[
						SNew(STextBlock)
							.Text(LOCTEXT("HighlightOnlyCheckBoxLabel", "Highlight Only"))
					]
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(16.0f, 0.0f, 0.0f, 0.0f)
			[
				// category filter
				SNew(SComboButton)
					.ButtonContent()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("CategoryComboButtonText", "Categories"))
					]
					.ContentPadding(FMargin(6.0f, 2.0f))
					.MenuContent()
					[
						SAssignNew(CategoriesListView, SListView<FSessionConsoleCategoryFilterPtr>)
							.ItemHeight(24)
							.ListItemsSource(&CategoriesList)
							.OnGenerateRow(this, &SContentBrowserFilterBar::HandleCategoryFilterGenerateRow)
					]
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			.HAlign(HAlign_Right)
			.Padding(4.0f, 0.0f, 0.0f, 0.0f)
			[
				// verbosity filter
				SNew(SComboButton)
					.ButtonContent()
					[
						SNew(STextBlock)
							.Text(LOCTEXT("VerbosityComboButtonText", "Verbosities"))
					]
					.ContentPadding(FMargin(6.0f, 2.0f))
					.MenuContent()
					[
						SAssignNew(VerbositiesListView, SListView<FSessionConsoleVerbosityFilterPtr>)
							.ItemHeight(24.0f)
							.ListItemsSource(&VerbositiesList)
							.OnGenerateRow(this, &SContentBrowserFilterBar::HandleVerbosityFilterGenerateRow)
					]
			]
	];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


bool SContentBrowserFilterBar::FilterLogMessage(const TSharedRef<FContentItemInfo>& LogMessage)
{
	// create or update category counter
	int32& CategoryCounter = CategoryCounters.FindOrAdd(LogMessage->Category);

	if (CategoryCounter == 0)
	{
		AddCategoryFilter(LogMessage->Category);
	}

	++CategoryCounter;

	// update the verbosity counter
	++VerbosityCounters.FindOrAdd(LogMessage->Verbosity);

	// filter the log message
	if (DisabledCategories.Contains(LogMessage->Category) ||
		DisabledVerbosities.Contains(LogMessage->Verbosity))
	{
		return false;
	}

	if (HighlightOnlyCheckBox->IsChecked() || FilterStringTextBox->GetText().IsEmpty())
	{
		return true;
	}

	return (LogMessage->Text.Contains(FilterStringTextBox->GetText().ToString()));
}


void SContentBrowserFilterBar::ResetFilter()
{
	CategoriesList.Reset();
	CategoryCounters.Reset();
	VerbosityCounters.Reset();

	CategoriesListView->RequestListRefresh();
}


/* SSessionConsoleFilterBar implementation
 *****************************************************************************/

void SContentBrowserFilterBar::AddCategoryFilter(const FName& Category)
{
	CategoriesList.Add(MakeShareable(
		new FContentConsoleCategoryFilter(Category,
			!DisabledCategories.Contains(Category),
			FOnContentConsoleCategoryFilterStateChanged::CreateSP(this, &SContentBrowserFilterBar::HandleCategoryFilterStateChanged)
		)
	));

	CategoriesListView->RequestListRefresh();
}


void SContentBrowserFilterBar::AddVerbosityFilter(ELogVerbosity::Type Verbosity, const FString& Name, const FName& Icon)
{
	VerbositiesList.Add(MakeShareable(
		new FContentConsoleVerbosityFilter(Verbosity, nullptr/*FEditorStyle::GetBrush(Icon)*/, true, Name,
			FOnContentConsoleVerbosityFilterStateChanged::CreateSP(this, &SContentBrowserFilterBar::HandleVerbosityFilterStateChanged))
		)
	);
}


/* SSessionConsoleFilterBar callbacks
 *****************************************************************************/

TSharedRef<ITableRow> SContentBrowserFilterBar::HandleCategoryFilterGenerateRow(FSessionConsoleCategoryFilterPtr Filter, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FSessionConsoleCategoryFilterPtr>, OwnerTable)
		[
			SNew(SCheckBox)
				.IsChecked(Filter.Get(), &FContentConsoleCategoryFilter::GetCheckStateFromIsEnabled)
				.Padding(FMargin(6.0, 2.0))
				.OnCheckStateChanged(Filter.Get(), &FContentConsoleCategoryFilter::EnableFromCheckState)
				[
					SNew(STextBlock)
						.Text(this, &SContentBrowserFilterBar::HandleCategoryFilterGetRowText, Filter)
				]
		];
}


FText SContentBrowserFilterBar::HandleCategoryFilterGetRowText(FSessionConsoleCategoryFilterPtr Filter) const
{
	return FText::Format(LOCTEXT("CategoryFilterRowFmt", "{0} ({1})"), FText::FromName(Filter->GetCategory()), FText::AsNumber(CategoryCounters.FindRef(Filter->GetCategory())));
}


void SContentBrowserFilterBar::HandleCategoryFilterStateChanged(const FName& Category, bool Enabled)
{
	if (Enabled)
	{
		DisabledCategories.Remove(Category);
	}
	else
	{
		DisabledCategories.AddUnique(Category);
	}

	OnFilterChanged.ExecuteIfBound();
}


void SContentBrowserFilterBar::HandleFilterStringTextChanged(const FText& NewText)
{
	OnFilterChanged.ExecuteIfBound();
}


void SContentBrowserFilterBar::HandleHighlightOnlyCheckBoxCheckStateChanged(ECheckBoxState CheckedState)
{
	OnFilterChanged.ExecuteIfBound();
}


FText SContentBrowserFilterBar::HandleVerbosityFilterGetRowText(FSessionConsoleVerbosityFilterPtr Filter) const
{
	return FText::Format(LOCTEXT("VerbosityFilterRowFmt", "{0} ({1})"), FText::FromString(Filter->GetName()), FText::AsNumber(VerbosityCounters.FindRef(Filter->GetVerbosity())));
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<ITableRow> SContentBrowserFilterBar::HandleVerbosityFilterGenerateRow(FSessionConsoleVerbosityFilterPtr Filter, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FSessionConsoleVerbosityFilterPtr>, OwnerTable)
		[
			SNew(SCheckBox)
				.IsChecked(Filter.Get(), &FContentConsoleVerbosityFilter::GetCheckStateFromIsEnabled)
				.Padding(FMargin(2.0f, 0.0f))
				.OnCheckStateChanged(Filter.Get(), &FContentConsoleVerbosityFilter::EnableFromCheckState)
				[
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2.0f, 0.0f)
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Center)
						[
							SNew(SImage)
								.Image(Filter->GetIcon())
						]

					+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2.0f, 0.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
								.Text(this, &SContentBrowserFilterBar::HandleVerbosityFilterGetRowText, Filter)
						]
				]
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SContentBrowserFilterBar::HandleVerbosityFilterStateChanged(ELogVerbosity::Type Verbosity, bool Enabled)
{
	if (Enabled)
	{
		DisabledVerbosities.Remove(Verbosity);
	}
	else
	{
		DisabledVerbosities.AddUnique(Verbosity);
	}

	OnFilterChanged.ExecuteIfBound();
}

FText SContentBrowserFilterBar::GetFilterText() const
{
	return FilterStringTextBox->GetText();
}


#undef LOCTEXT_NAMESPACE
