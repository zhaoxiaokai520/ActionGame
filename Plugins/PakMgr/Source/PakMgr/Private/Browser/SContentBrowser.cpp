// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Browser/SContentBrowser.h"
#include "Misc/MessageDialog.h"
#include "Misc/App.h"
#include "SlateOptMacros.h"
#include "EditorStyleSet.h"
#include "Templates/SharedPointer.h"
#include "Models/FContentItem.h"
#include "Browser/SContentItemRow.h"


#define LOCTEXT_NAMESPACE "SContentBrowser"


/* SContentBrowser statics
 *****************************************************************************/
TSharedPtr< SContentBrowser > SContentBrowser::ContentInstance = NULL;

TArray<TSharedPtr<FContentItemInfo>> SContentBrowser::GetItems()
{
	return ContentItems;
}

void SContentBrowser::SetInstance(TSharedPtr< class SContentBrowser > inst)
{
	ContentInstance = inst;
}


/* SContentBrowser structors
 *****************************************************************************/

SContentBrowser::~SContentBrowser()
{

}


/* SContentBrowser interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SContentBrowser::Construct( const FArguments& InArgs, TSharedRef<IPFileManager> InSessionManager)
{
	SessionManager = InSessionManager;
	IgnoreSessionManagerEvents = false;
	updatingTreeExpansion = false;
	bCanSetDefaultSelection = true;

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			// session tree
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(0.0f)
			[
				SAssignNew(ContentListView, SListView<TSharedPtr<FContentItemInfo>>)
				.ItemHeight(24.0f)
				.ListItemsSource(&ContentItems)
				.SelectionMode(ESelectionMode::Multi)
				.OnGenerateRow(this, &SContentBrowser::HandleContentListGenerateRow)
				.OnItemScrolledIntoView(this, &SContentBrowser::HandleLogListItemScrolledIntoView)
				.HeaderRow
				(
					SNew(SHeaderRow)

					+ SHeaderRow::Column("Verbosity")
					.DefaultLabel(LOCTEXT("ContentBrowserListViewItemHander", " "))
					.FixedWidth(24.0f)

					+ SHeaderRow::Column("Name")
					.DefaultLabel(LOCTEXT("ContentBrowserListViewItemName", "Name"))
					.FillWidth(1.0f)

					//+ SHeaderRow::Column("TimeSeconds")
					//.DefaultLabel(LOCTEXT("LogListTimestampColumnHeader", "Seconds"))
					//.FillWidth(0.10f)

					//+ SHeaderRow::Column("Message")
					//.DefaultLabel(LOCTEXT("LogListTextColumnHeader", "Message"))
					//.FillWidth(0.70f)
				)
			]
		]
	];

	ReloadSessions();

	SessionManager->OnAddModule().AddSP(this, &SContentBrowser::HandlAddModule);

	updatingTreeExpansion = true;
	updatingTreeExpansion = false;
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

/* SContentBrowser implementation
 *****************************************************************************/
void SContentBrowser::ReloadLog(bool FullyReload)
{
	// reload log list
	if (FullyReload)
	{
		AvailableItems.Reset();

		const auto& SelectedInstances = SessionManager->GetSelectedInstances();

		//for (const auto& Instance : SelectedInstances)
		//{
		//	const TArray<TSharedPtr<FContentItemInfo>>& InstanceLog = Instance->GetLog();

		//	for (const auto& LogMessage : InstanceLog)
		//	{
		//		AvailableItems.HeapPush(LogMessage, FFileLogMessage::TimeComparer());
		//	}
		//}
	}

	ContentItems.Reset();

	// filter log list
	FilterBar->ResetFilter();

	for (const auto& contentItemInfo : AvailableItems)
	{
		if (FilterBar->FilterLogMessage(contentItemInfo.ToSharedRef()))
		{
			ContentItems.Add(contentItemInfo);
		}
	}

	// refresh list view
	ContentListView->RequestListRefresh();

	if (ContentItems.Num() > 0)
	{
		ContentListView->RequestScrollIntoView(ContentItems.Last());
	}
}

 void SContentBrowser::AddInstanceItemToTree(TSharedPtr<FContentItem>& SessionItem, const TSharedPtr<FContentItem>& InstanceItem)
{
	// add instance to group or session
	InstanceItem->SetParent(SessionItem);
	SessionItem->AddChild(InstanceItem.ToSharedRef());
}

void SContentBrowser::ReloadSessions()
{
	//FilterSessions();
}


/* SContentBrowser event handlers
 *****************************************************************************/
FText SContentBrowser::HandleSessionTreeRowGetToolTipText(TSharedPtr<FContentItem> Item) const
{
	FTextBuilder ToolTipTextBuilder;

	if (Item->GetType() == EFileTreeNodeType::Instance)
	{
		
	}

	return ToolTipTextBuilder.ToText();
}


void SContentBrowser::HandleSessionTreeViewExpansionChanged(TSharedPtr<FContentItem> TreeItem, bool bIsExpanded)
{
	if ( updatingTreeExpansion || !TreeItem.IsValid())
	{
		return;
	}

	if (TreeItem->GetType() == EFileTreeNodeType::Instance)
	{
		return;
	}

	IgnoreSessionManagerEvents = true;
	{

	}
	IgnoreSessionManagerEvents = false;
}


TSharedRef<ITableRow> SContentBrowser::HandleContentListGenerateRow(TSharedPtr<FContentItemInfo> contentItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SContentItemRow, OwnerTable)
		.HighlightText(this, &SContentBrowser::HandleContentListGetHighlightText)
		.ContentItem(contentItem)
		.ToolTipText(FText::FromString(contentItem->Text));
}

void SContentBrowser::HandleLogListItemScrolledIntoView(TSharedPtr<FContentItemInfo> Item, const TSharedPtr<ITableRow>& TableRow)
{
	if (ContentItems.Num() > 0)
	{
		ShouldScrollToLast = ContentListView->IsItemVisible(ContentItems.Last());
	}
	else
	{
		ShouldScrollToLast = true;
	}
}

FText SContentBrowser::HandleContentListGetHighlightText() const
{
	return FText::FromString(HighlightText); //FilterBar->GetFilterText();
}


void SContentBrowser::HandleSessionTreeViewGetChildren(TSharedPtr<FContentItem> Item, TArray<TSharedPtr<FContentItem>>& OutChildren)
{
	if (Item.IsValid())
	{
		OutChildren = Item->GetChildren();
	}
}


void SContentBrowser::HandleSessionTreeViewSelectionChanged(const TSharedPtr<FContentItem> Item, ESelectInfo::Type SelectInfo)
{
	IgnoreSessionManagerEvents = true;
	{
		if (Item.IsValid())
		{
			if (Item->GetType() == EFileTreeNodeType::Instance)
			{
				const auto& InstanceInfo = StaticCastSharedPtr<FFileInstanceTreeItem>(Item)->GetInstanceInfo();

				if (InstanceInfo.IsValid())
				{
					// special handling for local application
				}
			}
		}

		{
			// check if any instances are no longer selected

		}
	}
	IgnoreSessionManagerEvents = false;
}


FReply SContentBrowser::HandleTerminateSessionButtonClicked()
{
	int32 DialogResult = FMessageDialog::Open(EAppMsgType::YesNo, LOCTEXT("TerminateSessionDialogPrompt", "Are you sure you want to terminate this session and its instances?"));

	if (DialogResult == EAppReturnType::Yes)
	{

	}

	return FReply::Handled();
}


bool SContentBrowser::HandleTerminateSessionButtonIsEnabled() const
{
	return true;
}

void SContentBrowser::HandlAddModule(const FString& moduleName)
{
	ContentItems.Add(MakeShareable(new FContentItemInfo(moduleName)));

	//ReloadLog(true);
	ContentListView->RequestListRefresh();
}

#undef LOCTEXT_NAMESPACE
