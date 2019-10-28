// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SPakManager.h"
//#include "DesktopPlatformModule.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/SOverlay.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandList.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "PakManager/PakManagerCommands.h"
#include "Widgets/Views/SListView.h"
#include "PakManager/SPakManagerToolbar.h"
#include "Widgets/Layout/SExpandableArea.h"


#define LOCTEXT_NAMESPACE "SFileTreePanel"


/* SFileTreePanel structors
 *****************************************************************************/

SPakManager::~SPakManager()
{
	if (SessionManager.IsValid())
	{
		SessionManager->OnInstanceSelectionChanged().RemoveAll(this);
		SessionManager->OnLogReceived().RemoveAll(this);
		SessionManager->OnSelectedSessionChanged().RemoveAll(this);
	}
}


/* SFileTreePanel interface
 *****************************************************************************/

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPakManager::Construct(const FArguments& InArgs, TSharedRef<IPFileManager> InSessionManager)
{
	SessionManager = InSessionManager;
	ShouldScrollToLast = true;

	// create and bind the commands
	UICommandList = MakeShareable(new FUICommandList);
	BindCommands();

	ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
			[
				SNew(SVerticalBox)
					.IsEnabled(this, &SPakManager::HandleMainContentIsEnabled)

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						// toolbar
						SNew(SPakManagerToolbar, UICommandList.ToSharedRef())
					]

				//content area for the log
				+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
							.FillWidth(1.0f)
							[
								// log list
								SNew(SBorder)
									//.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
									.Padding(0.0f)
									[
										SAssignNew(LogListView, SListView<TSharedPtr<FFileItemInfo>>)
											.ItemHeight(24.0f)
											.ListItemsSource(&LogMessages)
											.SelectionMode(ESelectionMode::Multi)
											//.OnGenerateRow(this, &SPakManager::HandleLogListGenerateRow)
											.OnItemScrolledIntoView(this, &SPakManager::HandleLogListItemScrolledIntoView)
											.HeaderRow
											(
												SNew(SHeaderRow)

												+ SHeaderRow::Column("Verbosity")
													.DefaultLabel(LOCTEXT("LogListVerbosityColumnHeader", " "))
													.FixedWidth(24.0f)

												+ SHeaderRow::Column("Instance")
													.DefaultLabel(LOCTEXT("LogListHostNameColumnHeader", "Instance"))
													.FillWidth(0.20f)

												+ SHeaderRow::Column("TimeSeconds")
													.DefaultLabel(LOCTEXT("LogListTimestampColumnHeader", "Seconds"))
													.FillWidth(0.10f)

												+ SHeaderRow::Column("Message")
													.DefaultLabel(LOCTEXT("LogListTextColumnHeader", "Message"))
													.FillWidth(0.70f)
											)
									]
						]
					]

			]

		+ SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBorder)
					//.BorderImage(FEditorStyle::GetBrush("NotificationList.ItemBackground"))
					.Padding(8.0f)
					.Visibility(this, &SPakManager::HandleSelectSessionOverlayVisibility)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("SelectSessionOverlayText", "Please select at least one instance from the Content Browser"))
					]
			]
	];

	SessionManager->OnInstanceSelectionChanged().AddSP(this, &SPakManager::HandleSessionManagerInstanceSelectionChanged);
	SessionManager->OnLogReceived().AddSP(this, &SPakManager::HandleSessionManagerLogReceived);
	SessionManager->OnSelectedSessionChanged().AddSP(this, &SPakManager::HandleSessionManagerSelectedSessionChanged);

	ReloadLog(true);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SFileTreePanel implementation
 *****************************************************************************/

void SPakManager::BindCommands()
{
	FPakManagerCommands::Register();

	const FPakManagerCommands& Commands = FPakManagerCommands::Get();

	UICommandList->MapAction(
		Commands.GenPaks,
		FExecuteAction::CreateSP(this, &SPakManager::HandleGenPaksActionExecute),
		FCanExecuteAction::CreateSP(this, &SPakManager::HandleGenPaksActionCanExecute));

	UICommandList->MapAction(
		Commands.GenMani,
		FExecuteAction::CreateSP(this, &SPakManager::HandleGenManiActionExecute),
		FCanExecuteAction::CreateSP(this, &SPakManager::HandleGenManiActionCanExecute));

	UICommandList->MapAction(
		Commands.MaxSize,
		FExecuteAction::CreateSP(this, &SPakManager::HandleMaxSizeActionExecute),
		FCanExecuteAction::CreateSP(this, &SPakManager::HandleMaxSizeActionCanExecute));
}


void SPakManager::ClearLog()
{
	LogMessages.Reset();
	LogListView->RequestListRefresh();
}


void SPakManager::CopyLog()
{
	TArray<TSharedPtr<FFileItemInfo>> SelectedItems = LogListView->GetSelectedItems();

	if (SelectedItems.Num() == 0)
	{
		return;
	}

	FString SelectedText;

	for (const auto& Item : SelectedItems)
	{
		SelectedText += FString::Printf(TEXT("%s [%s] %09.3f: %s"), *Item->Time.ToString(), *Item->InstanceName, Item->TimeSeconds, *Item->Text);
		SelectedText += LINE_TERMINATOR;
	}

	FPlatformApplicationMisc::ClipboardCopy(*SelectedText);
}


void SPakManager::ReloadLog(bool FullyReload)
{
	// reload log list
	if (FullyReload)
	{
		AvailableLogs.Reset();

		const auto& SelectedInstances = SessionManager->GetSelectedInstances();

		for (const auto& Instance : SelectedInstances)
		{
			const TArray<TSharedPtr<FFileItemInfo>>& InstanceLog = Instance->GetLog();

			for (const auto& LogMessage : InstanceLog)
			{
				AvailableLogs.HeapPush(LogMessage, FFileItemInfo::TimeComparer());
			}
		}
	}

	LogMessages.Reset();

	// refresh list view
	LogListView->RequestListRefresh();

	if (LogMessages.Num() > 0)
	{
		LogListView->RequestScrollIntoView(LogMessages.Last());
	}
}


void SPakManager::SaveLog()
{
	//IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();

	//if (DesktopPlatform == nullptr)
	//{
	//	FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SaveLogDialogUnsupportedError", "Saving is not supported on this platform!"));

	//	return;
	//}

	TArray<FString> Filenames;

	// open file dialog
	TSharedPtr<SWindow> ParentWindow = FSlateApplication::Get().FindWidgetWindow(AsShared());
	void* ParentWindowHandle = (ParentWindow.IsValid() && ParentWindow->GetNativeWindow().IsValid()) ? ParentWindow->GetNativeWindow()->GetOSWindowHandle() : nullptr;

	//if (!DesktopPlatform->SaveFileDialog(
	//	ParentWindowHandle,
	//	LOCTEXT("SaveLogDialogTitle", "Save Log As...").ToString(),
	//	LastLogFileSaveDirectory,
	//	TEXT("Session.log"),
	//	TEXT("Log Files (*.log)|*.log"),
	//	EFileDialogFlags::None,
	//	Filenames))
	//{
	//	return;
	//}

	// no log file selected?
	if (Filenames.Num() == 0)
	{
		return;
	}

	FString Filename = Filenames[0];

	// keep path as default for next time
	LastLogFileSaveDirectory = FPaths::GetPath(Filename);

	// add a file extension if none was provided
	if (FPaths::GetExtension(Filename).IsEmpty())
	{
		Filename += Filename + TEXT(".log");
	}

	// save file
	FArchive* LogFile = IFileManager::Get().CreateFileWriter(*Filename);

	if (LogFile != nullptr)
	{
		for (const auto& LogMessage : LogMessages)
		{
			FString LogEntry = FString::Printf(TEXT("%s [%s] %09.3f: %s"),
				*LogMessage->Time.ToString(),
				*LogMessage->InstanceName,
				LogMessage->TimeSeconds,
				*LogMessage->Text) + LINE_TERMINATOR;

			LogFile->Serialize(TCHAR_TO_ANSI(*LogEntry), LogEntry.Len());
		}

		LogFile->Close();
		delete LogFile;
	}
	else
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("SaveLogDialogFileError", "Failed to open the specified file for saving!"));
	}
}


void SPakManager::SendCommand(const FString& CommandString)
{
	if (CommandString.IsEmpty())
	{
		return;
	}

	for (auto& Instance : SessionManager->GetSelectedInstances())
	{
		Instance->ExecuteCommand(CommandString);
	}
}



/* SWidget implementation
 *****************************************************************************/

FReply SPakManager::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsControlDown())
	{
		if (InKeyEvent.GetKey() == EKeys::C)
		{
			CopyLog();

			return FReply::Handled();
		}
	
		if (InKeyEvent.GetKey() == EKeys::S)
		{
			SaveLog();

			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}


/* SSessionConsolePanel event handlers
 *****************************************************************************/

void SPakManager::HandleGenPaksActionExecute()
{
	ClearLog();
}


bool SPakManager::HandleGenPaksActionCanExecute()
{
	return true;
}


void SPakManager::HandleCommandBarPromoteToShortcutClicked(const FString& CommandString)
{

}


void SPakManager::HandleCommandSubmitted(const FString& CommandString)
{
	SendCommand(CommandString);
}


void SPakManager::HandleGenManiActionExecute()
{
	CopyLog();
}


bool SPakManager::HandleGenManiActionCanExecute()
{
	return true;
}


void SPakManager::HandleFilterChanged()
{
	ReloadLog(false);
}


void SPakManager::HandleLogListItemScrolledIntoView(TSharedPtr<FFileItemInfo> Item, const TSharedPtr<ITableRow>& TableRow)
{
	if (LogMessages.Num() > 0)
	{
		ShouldScrollToLast = LogListView->IsItemVisible(LogMessages.Last());
	}
	else
	{
		ShouldScrollToLast = true;
	}
}


//TSharedRef<ITableRow> SPakManager::HandleLogListGenerateRow(TSharedPtr<FFileLogMessage> Message, const TSharedRef<STableViewBase>& OwnerTable)
//{
//	return SNew(SFileTreeLogTableRow, OwnerTable)
//		.HighlightText(this, &SPakManager::HandleLogListGetHighlightText)
//		.LogMessage(Message)
//		.ToolTipText(FText::FromString(Message->Text));
//}


FText SPakManager::HandleLogListGetHighlightText() const
{
	return FText::FromString(HighlightText); //FilterBar->GetFilterText();
}


bool SPakManager::HandleMainContentIsEnabled() const
{
	//return (SessionManager->GetSelectedInstances().Num() > 0);
	return true;
}


void SPakManager::HandleMaxSizeActionExecute()
{
	SaveLog();
}


bool SPakManager::HandleMaxSizeActionCanExecute()
{
	return true;
}


EVisibility SPakManager::HandleSelectSessionOverlayVisibility() const
{
	//if (SessionManager->GetSelectedInstances().Num() > 0)
	//{
	//	return EVisibility::Hidden;
	//}

	return EVisibility::Visible;
}


void SPakManager::HandleSessionManagerInstanceSelectionChanged(const TSharedPtr<IFileInstanceInfo>& /*Instance*/, bool /*Selected*/)
{
	ReloadLog(true);
}


void SPakManager::HandleSessionManagerLogReceived(const TSharedRef<IFileInfo>& Session, const TSharedRef<IFileInstanceInfo>& Instance, const TSharedRef<FFileItemInfo>& Message)
{
	AvailableLogs.Add(Message);
	LogMessages.Add(Message);

	LogListView->RequestListRefresh();

	if (ShouldScrollToLast)
	{
		LogListView->RequestScrollIntoView(Message);
	}
}


void SPakManager::HandleSessionManagerSelectedSessionChanged(const TSharedPtr<IFileInfo>& SelectedSession)
{
	ReloadLog(true);
}


#undef LOCTEXT_NAMESPACE
