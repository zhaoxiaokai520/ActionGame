// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Misc/Guid.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Input/Reply.h"
#include "Widgets/SCompoundWidget.h"
#include "Models/IPFileManager.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"
#include "Models/ContentItemInfo.h"
#include "Browser/SContentBrowserFilterBar.h"

class FFileGroupTreeItem;
class SContentBrowserCommandBar;
class FContentItem;

/**
 * Implements a Slate widget for browsing active game sessions.
 */
class SContentBrowser
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SContentBrowser) { }
	SLATE_END_ARGS()

public:

	/** Destructor. */
	~SContentBrowser();

public:

	/**
	 * Construct this widget.
	 *
	 * @param InArgs The declaration data for this widget.
	 */
	void Construct( const FArguments& InArgs, TSharedRef<IPFileManager> InSessionManager);
	TArray<TSharedPtr<FContentItemInfo>> GetItems();
	static TSharedPtr< class SContentBrowser > Get() { return ContentInstance; }
	static void SetInstance(TSharedPtr< class SContentBrowser > inst);

protected:

	/**
	 * Fully expands the specified tree view item.
	 *
	 * @param Item The tree view item to expand.
	 */
	void ExpandItem(const TSharedPtr<FContentItem>& Item);

	///** Filters the session tree. */
	//void FilterSessions();

	/** Adds items for this session in the tree. */
	void AddInstanceItemToTree(TSharedPtr<FContentItem>& SessionItem, const TSharedPtr<FContentItem>& InstanceItem);

	/** Reloads the sessions list. */
	void ReloadSessions();

private:
	/** Callback for getting the tool tip text of a session tree row. */
	FText HandleSessionTreeRowGetToolTipText(TSharedPtr<FContentItem> Item) const;

	/** Callback for session tree view selection changes. */
	void HandleSessionTreeViewExpansionChanged(TSharedPtr<FContentItem> TreeItem, bool bIsExpanded);

	/** Callback for generating a row widget in the session tree view. */
	TSharedRef<ITableRow> HandleContentListGenerateRow(TSharedPtr<FContentItemInfo> Message, const TSharedRef<STableViewBase>& OwnerTable);

	void HandleLogListItemScrolledIntoView(TSharedPtr<FContentItemInfo> Item, const TSharedPtr<ITableRow>& TableRow);

	FText HandleContentListGetHighlightText() const;
	/** Callback for getting the children of a node in the session tree view. */
	void HandleSessionTreeViewGetChildren(TSharedPtr<FContentItem> Item, TArray<TSharedPtr<FContentItem>>& OutChildren);

	/** Callback for session tree view selection changes. */
	void HandleSessionTreeViewSelectionChanged(const TSharedPtr<FContentItem> Item, ESelectInfo::Type SelectInfo);

	/** Callback for clicking the 'Terminate Session' button. */
	FReply HandleTerminateSessionButtonClicked();

	/** Callback for getting the enabled state of the 'Terminate Session' button. */
	bool HandleTerminateSessionButtonIsEnabled() const;

	void HandlAddModule(const FString& moduleName);

	void ReloadLog(bool FullyReload);

private:
	static TSharedPtr< class SContentBrowser > ContentInstance;

	/** Holds an unfiltered list of available log messages. */
	TArray<TSharedPtr<FContentItemInfo>> AvailableItems;

	/** Holds the session manager. */
	TSharedPtr<IPFileManager> SessionManager;

	/** Holds the filter bar. */
	TSharedPtr<SContentBrowserFilterBar> FilterBar;

	/** Holds the command bar. */
	TSharedPtr<SContentBrowserCommandBar> CommandBar;

	/** Whether to ignore events from the session manager. */
	bool IgnoreSessionManagerEvents;

	/** Whether to ignore events from the session tree view. */
	bool updatingTreeExpansion;

	TSharedPtr<SListView<TSharedPtr<FContentItemInfo>>> ContentListView;
	/** Holds the filtered list of content item. */
	TArray<TSharedPtr<FContentItemInfo>> ContentItems;

	/** Holds a flag indicating whether the log list should auto-scroll to the last item. */
	bool ShouldScrollToLast;

private:

	/** The session tree item that holds this application's session. */
	TSharedPtr<FFileGroupTreeItem> AppGroupItem;

	/** The session tree item that holds other user's sessions. */
	TSharedPtr<FFileGroupTreeItem> OtherGroupItem;

	/** The session tree item that holds the owner's remote sessions. */
	TSharedPtr<FFileGroupTreeItem> OwnerGroupItem;

	/** The session tree item that holds other user's standalone instances. */
	TSharedPtr<FFileGroupTreeItem> StandaloneGroupItem;

	/** This app's instance session */
	TWeakPtr<FContentItem> ThisAppInstance;

	/** True if we should set the default selection the next time the tree view if refreshed. */
	bool bCanSetDefaultSelection;

	/** Holds the highlight text. */
	FString HighlightText;
};
