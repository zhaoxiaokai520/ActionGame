// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateFwd.h"
#include "Layout/Visibility.h"
#include "Input/Reply.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableViewBase.h"
#include "Widgets/Views/STableRow.h"
#include "Models/IFileInstanceInfo.h"
#include "Models/IFileInfo.h"
#include "Models/FileItemInfo.h"
#include "Models/IPFileManager.h"
#include "Framework/Commands/UICommandList.h"

/**
 * Implements the File Tree panel.
 *
 * This panel receives console log messages from a remote engine session and can also send
 * console commands to it.
 */
class SPakManager
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SPakManager) { }
	SLATE_END_ARGS()

public:

	/** Destructor. */
	~SPakManager();

public:

	/**
	 * Construct this widget
	 *
	 * @param InArgs The declaration data for this widget.
	 * @param InSessionManager The session manager to use.
	 */
	void Construct(const FArguments& InArgs, TSharedRef<IPFileManager> InSessionManager);

protected:

	/** Binds the device commands on our toolbar. */
	void BindCommands();

	/**
	 * Clears the log list view.
	 *
	 * @see CopyLog, ReloadLog, SaveLog
	 */
	void ClearLog();

	/**
	 * Copies the selected log messages to the clipboard.
	 *
	 * @see ClearLog, ReloadLog, SaveLog
	 */
	void CopyLog();

	/**
	 * Reloads the log messages for the currently selected engine instances.
	 *
	 * @param FullyReload Whether to fully reload the log entries or only re-apply filtering.
	 * @see ClearLog, CopyLog, SaveLog
	 */
	void ReloadLog(bool FullyReload);

	/**
	 * Saves all log messages to a file.
	 *
	 * @see ClearLog, CopyLog, ReloadLog
	 */
	void SaveLog();

	/**
	 * Sends the command entered into the input field.
	 *
	 * @param CommandString The command string to send.
	 */
	void SendCommand(const FString& CommandString);

protected:

	// SCompoundWidget overrides

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:

	/** Callback for executing the 'Clear' action. */
	void HandleGenPaksActionExecute();

	/** Callback for determining the 'Clear' action can execute. */
	bool HandleGenPaksActionCanExecute();

	/** Callback for executing the 'Copy' action. */
	void HandleGenManiActionExecute();

	/** Callback for determining the 'Copy' action can execute. */
	bool HandleGenManiActionCanExecute();

	/** Callback for executing the 'Save' action. */
	void HandleMaxSizeActionExecute();

	/** Callback for determining the 'Save' action can execute. */
	bool HandleMaxSizeActionCanExecute();

	/** Callback for promoting console command to shortcuts. */
	void HandleCommandBarPromoteToShortcutClicked(const FString& CommandString);

	/** Callback for submitting console commands. */
	void HandleCommandSubmitted(const FString& CommandString);

	/** Callback for changing the filter settings. */
	void HandleFilterChanged();

	/** Callback for scrolling a log item into view. */
	void HandleLogListItemScrolledIntoView(TSharedPtr<FFileItemInfo> Item, const TSharedPtr<ITableRow>& TableRow);

	/** Callback for generating a row widget for the log list view. */
	//TSharedRef<ITableRow> HandleLogListGenerateRow(TSharedPtr<FFileLogMessage> Message, const TSharedRef<STableViewBase>& OwnerTable);

	/** Callback for getting the highlight string for log messages. */
	FText HandleLogListGetHighlightText() const;

	/** Callback for selecting log messages. */
	void HandleLogListSelectionChanged(TSharedPtr<FFileItemInfo> InItem, ESelectInfo::Type SelectInfo);

	/** Callback for getting the enabled state of the console box. */
	bool HandleMainContentIsEnabled() const;

	/** Callback for determining the visibility of the 'Select a session' overlay. */
	EVisibility HandleSelectSessionOverlayVisibility() const;

	/** Callback for changing the engine instance selection. */
	void HandleSessionManagerInstanceSelectionChanged(const TSharedPtr<IFileInstanceInfo>& Instance, bool Selected);

	/** Callback for received log entries. */
	void HandleSessionManagerLogReceived(const TSharedRef<IFileInfo>& Session, const TSharedRef<IFileInstanceInfo>& Instance, const TSharedRef<FFileItemInfo>& Message);

	/** Callback for changing the selected session. */
	void HandleSessionManagerSelectedSessionChanged(const TSharedPtr<IFileInfo>& SelectedSession);

private:

	/** Holds an unfiltered list of available log messages. */
	TArray<TSharedPtr<FFileItemInfo>> AvailableLogs;

	/** Holds the find bar. */
	TSharedPtr<SSearchBox> FindBar;

	/** Holds the highlight text. */
	FString HighlightText;

	/** Holds the directory where the log file was last saved to. */
	FString LastLogFileSaveDirectory;

	/** Holds the log list view. */
 	TSharedPtr<SListView<TSharedPtr<FFileItemInfo>>> LogListView;

 	/** Holds the filtered list of log messages. */
 	TArray<TSharedPtr<FFileItemInfo>> LogMessages;

	/** Holds the session manager. */
	TSharedPtr<IPFileManager> SessionManager;

	/** Holds a flag indicating whether the log list should auto-scroll to the last item. */
	bool ShouldScrollToLast;

	/** The command list for controlling the device */
	TSharedPtr<FUICommandList> UICommandList;
};
