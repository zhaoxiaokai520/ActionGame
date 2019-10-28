// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "SFileTree.h"
#include "DesktopPlatformModule.h"
#include "Misc/MessageDialog.h"
#include "EditorDirectories.h"
#include "AssetRegistryModule.h"
#include "Browser/SContentBrowser.h"
#include "Models/ContentItemInfo.h"
#include "Interfaces/IMainFrameModule.h"
#include "HAL/FileManager.h"
#include "PakMgrModule.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Widgets/SOverlay.h"
#include "SlateOptMacros.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Commands/UICommandList.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "FileTree/FileTreeCommands.h"
#include "Widgets/Views/SListView.h"
#include "FileTree/SFileTreeItemTableRow.h"
#include "FileTree/SFileTreeCommandBar.h"
#include "FileTree/SFileTreeFilterBar.h"
#include "FileTree/SFileTreeShortcutWindow.h"
#include "FileTree/SFileTreeToolbar.h"
#include "Widgets/Layout/SExpandableArea.h"


#define LOCTEXT_NAMESPACE "SFileTreePanel"


/* SFileTreePanel structors
 *****************************************************************************/

SFileTree::~SFileTree()
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
void SFileTree::Construct(const FArguments& InArgs, TSharedRef<IPFileManager> InSessionManager)
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
					.IsEnabled(this, &SFileTree::HandleMainContentIsEnabled)

				+ SVerticalBox::Slot()
					.AutoHeight()
					[
						// toolbar
						SNew(SFileTreeToolbar, UICommandList.ToSharedRef())
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						// filter bar
						SNew(SExpandableArea)
							.AreaTitle(LOCTEXT("FilterBarAreaTitle", "Log Filter"))
							.InitiallyCollapsed(true)
							.Padding(FMargin(8.0f, 6.0f))
							.BodyContent()
							[
								SAssignNew(FilterBar, SFileTreeFilterBar)
									.OnFilterChanged(this, &SFileTree::HandleFilterChanged)
							]
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
											.OnGenerateRow(this, &SFileTree::HandleLogListGenerateRow)
											.OnItemScrolledIntoView(this, &SFileTree::HandleLogListItemScrolledIntoView)
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

						//Shortcut buttons
						+ SHorizontalBox::Slot()
							.FillWidth(0.2f)
							[
								SAssignNew(ShortcutWindow, SFileTreeShortcutWindow)
									.OnCommandSubmitted(this, &SFileTree::HandleCommandSubmitted)
							]
					]

				+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 4.0f, 0.0f, 0.0f)
					[
						SNew(SBorder)
							.Padding(FMargin(8.0f, 6.0f))
							//.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
							[
								// command bar
								SAssignNew(CommandBar, SFileTreeCommandBar)
									.OnCommandSubmitted(this, &SFileTree::HandleCommandSubmitted)
									.OnPromoteToShortcutClicked(this, &SFileTree::HandleCommandBarPromoteToShortcutClicked)
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
					.Visibility(this, &SFileTree::HandleSelectSessionOverlayVisibility)
					[
						SNew(STextBlock)
							.Text(LOCTEXT("SelectSessionOverlayText", "Please select at least one instance from the Content Browser"))
					]
			]
	];

	SessionManager->OnInstanceSelectionChanged().AddSP(this, &SFileTree::HandleSessionManagerInstanceSelectionChanged);
	SessionManager->OnLogReceived().AddSP(this, &SFileTree::HandleSessionManagerLogReceived);
	SessionManager->OnSelectedSessionChanged().AddSP(this, &SFileTree::HandleSessionManagerSelectedSessionChanged);

	ReloadLog(true);
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


/* SFileTreePanel implementation
 *****************************************************************************/

void SFileTree::BindCommands()
{
	FFileTreeCommands::Register();

	const FFileTreeCommands& Commands = FFileTreeCommands::Get();

	UICommandList->MapAction(
		Commands.SelCon,
		FExecuteAction::CreateSP(this, &SFileTree::HandleSelConActionExecute),
		FCanExecuteAction::CreateSP(this, &SFileTree::HandleSelConActionCanExecute));

	UICommandList->MapAction(
		Commands.AddM,
		FExecuteAction::CreateSP(this, &SFileTree::HandleAddMActionExecute),
		FCanExecuteAction::CreateSP(this, &SFileTree::HandleAddMActionCanExecute));

	UICommandList->MapAction(
		Commands.GenRef,
		FExecuteAction::CreateSP(this, &SFileTree::HandleGenRefActionExecute),
		FCanExecuteAction::CreateSP(this, &SFileTree::HandleGenRefActionCanExecute));

	UICommandList->MapAction(
		Commands.SortRef,
		FExecuteAction::CreateSP(this, &SFileTree::HandleSortActionExecute),
		FCanExecuteAction::CreateSP(this, &SFileTree::HandleSortActionCanExecute));

	UICommandList->MapAction(
		Commands.SearchFiles,
		FExecuteAction::CreateSP(this, &SFileTree::HandleSearchActionExecute),
		FCanExecuteAction::CreateSP(this, &SFileTree::HandleSearchActionCanExecute));
}


void SFileTree::ClearLog()
{
	LogMessages.Reset();
	LogListView->RequestListRefresh();
}


void SFileTree::CopyLog()
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


void SFileTree::ReloadLog(bool FullyReload)
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

		CommandBar->SetNumSelectedInstances(SelectedInstances.Num());
	}

	LogMessages.Reset();

	// filter log list
	FilterBar->ResetFilter();

	for (const auto& LogMessage : AvailableLogs)
	{
		if (FilterBar->FilterLogMessage(LogMessage.ToSharedRef()))
		{
			LogMessages.Add(LogMessage);
		}
	}

	// refresh list view
	LogListView->RequestListRefresh();

	if (LogMessages.Num() > 0)
	{
		LogListView->RequestScrollIntoView(LogMessages.Last());
	}
}


void SFileTree::SaveLog()
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


void SFileTree::SendCommand(const FString& CommandString)
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

FReply SFileTree::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
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

void SFileTree::HandleSelConActionExecute()
{
	void* ParentWindowWindowHandle = nullptr;
	IMainFrameModule& MainFrameModule = FModuleManager::LoadModuleChecked<IMainFrameModule>(TEXT("MainFrame"));
	const TSharedPtr<SWindow>& MainFrameParentWindow = MainFrameModule.GetParentWindow();
	if (MainFrameParentWindow.IsValid() && MainFrameParentWindow->GetNativeWindow().IsValid())
	{
		ParentWindowWindowHandle = MainFrameParentWindow->GetNativeWindow()->GetOSWindowHandle();
	}

	FString OutFolderName;
	if (!FDesktopPlatformModule::Get()->OpenDirectoryDialog(ParentWindowWindowHandle, LOCTEXT("PackageDirectoryDialogTitle", "Package project...").ToString(), ContentPath, OutFolderName))
	{
		return;
	}

	ContentPath = FPaths::ConvertRelativePathToFull(OutFolderName);
	FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_IMPORT, FPaths::GetPath(ContentPath)); // Save path as default for next time.
}


bool SFileTree::HandleSelConActionCanExecute()
{
	return true;
}


void SFileTree::HandleCommandBarPromoteToShortcutClicked(const FString& CommandString)
{
	ShortcutWindow->AddShortcut(CommandString, CommandString);
}


void SFileTree::HandleCommandSubmitted(const FString& CommandString)
{
	SendCommand(CommandString);
}


void SFileTree::HandleAddMActionExecute()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	const FText Title = LOCTEXT("Add Module", "Add Pak Module File");
	const FString FileTypes = TEXT("Module Resource (*.umap) | *.umap");

	TArray<FString> OutFilenames;
	DesktopPlatform->OpenFileDialog(
		ParentWindowWindowHandle,
		Title.ToString(),
		TEXT(""),
		TEXT(""),
		FileTypes,
		EFileDialogFlags::None,
		OutFilenames
	);

	if (OutFilenames.Num() > 0)
	{
		FString AbsPath = FPaths::ConvertRelativePathToFull(OutFilenames[0]);
		SessionManager->SetAddModule(AbsPath);
	}
}


bool SFileTree::HandleAddMActionCanExecute()
{
	return true;
}


void SFileTree::HandleFilterChanged()
{
	HighlightText = FilterBar->GetFilterText().ToString();

	ReloadLog(false);
}


void SFileTree::HandleLogListItemScrolledIntoView(TSharedPtr<FFileItemInfo> Item, const TSharedPtr<ITableRow>& TableRow)
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


TSharedRef<ITableRow> SFileTree::HandleLogListGenerateRow(TSharedPtr<FFileItemInfo> Message, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SFileTreeitemTableRow, OwnerTable)
		.HighlightText(this, &SFileTree::HandleLogListGetHighlightText)
		.FileItemInfo(Message)
		.ToolTipText(FText::FromString(Message->Text));
}


FText SFileTree::HandleLogListGetHighlightText() const
{
	return FText::FromString(HighlightText); //FilterBar->GetFilterText();
}


bool SFileTree::HandleMainContentIsEnabled() const
{
	//return (SessionManager->GetSelectedInstances().Num() > 0);
	return true;
}


void SFileTree::HandleGenRefActionExecute()
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetIdentifier> ReferenceNames;

	
	FString OutFailureReason = "";
	FString AssetPath = "";
	IPakMgrModule pakModule = IPakMgrModule::Get();
	
	TArray<TSharedPtr<FContentItemInfo>> items = SContentBrowser::Get()->GetItems();
	for (auto& item : items)
	{
		AssetPath = item->Text;
		TArray<FName> HardDependencies;
		TArray<FName> SoftDependencies;
		//FAssetData AssetData = IPakMgrModule::Get().FindAssetDataFromAnyPath(AssetPath, OutFailureReason);
		//if (!AssetData.IsValid())
		//{
		//	return;
		//}
		const UPackage* OuterPackage = pakModule.GetMapPackage(pakModule.ConvertPhysicalPathToUFSPath(AssetPath));
		AssetRegistryModule.Get().GetReferencers(FAssetIdentifier(OuterPackage->GetFName()), ReferenceNames);
		AssetRegistryModule.Get().GetDependencies(OuterPackage->GetFName(), HardDependencies, EAssetRegistryDependencyType::Hard);
		AssetRegistryModule.Get().GetDependencies(OuterPackage->GetFName(), SoftDependencies, EAssetRegistryDependencyType::Soft);
		if (ReferenceNames.Num() > 0 && HardDependencies.Num() > 0)
			UE_LOG(LogPakMgr, Warning, TEXT("HandleGenRefActionExecute ReferenceNames %s dependencies %s "), *ReferenceNames[0].ToString(), *HardDependencies[0].ToString());
	}
}


bool SFileTree::HandleGenRefActionCanExecute()
{
	return true;
}

void SFileTree::HandleSortActionExecute()
{
	SaveLog();
}


bool SFileTree::HandleSortActionCanExecute()
{
	return true;
}

void SFileTree::HandleSearchActionExecute()
{
	SaveLog();
}


bool SFileTree::HandleSearchActionCanExecute()
{
	return true;
}


EVisibility SFileTree::HandleSelectSessionOverlayVisibility() const
{
	//if (SessionManager->GetSelectedInstances().Num() > 0)
	//{
	//	return EVisibility::Hidden;
	//}

	return EVisibility::Visible;
}


void SFileTree::HandleSessionManagerInstanceSelectionChanged(const TSharedPtr<IFileInstanceInfo>& /*Instance*/, bool /*Selected*/)
{
	ReloadLog(true);
}


void SFileTree::HandleSessionManagerLogReceived(const TSharedRef<IFileInfo>& Session, const TSharedRef<IFileInstanceInfo>& Instance, const TSharedRef<FFileItemInfo>& Message)
{
	if (!SessionManager->IsInstanceSelected(Instance) || !FilterBar->FilterLogMessage(Message))
	{
		return;
	}

	AvailableLogs.Add(Message);
	LogMessages.Add(Message);

	LogListView->RequestListRefresh();

	if (ShouldScrollToLast)
	{
		LogListView->RequestScrollIntoView(Message);
	}
}


void SFileTree::HandleSessionManagerSelectedSessionChanged(const TSharedPtr<IFileInfo>& SelectedSession)
{
	ReloadLog(true);
}


#undef LOCTEXT_NAMESPACE
