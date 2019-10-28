// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.


#include "SPakMgrPanel.h"
#include "SlateOptMacros.h"
#include "Textures/SlateIcon.h"
#include "EditorStyleSet.h"
#include "PakMgrModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Modules/ModuleManager.h"
#include "FileTree/SFileTree.h"
#include "Internationalization/Internationalization.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "MultiBox/MultiBoxExtender.h"
#include "MultiBox/MultiBoxBuilder.h"
#include "Browser/SContentBrowser.h"
#include "PakManager/SPakManager.h"

#define LOCTEXT_NAMESPACE "PakMgr"

static const FName FileTreeTabId("FileTree");
static const FName PakMgrTabId("PakMgr");
static const FName ContentBrowserTabId("ContentBrowser");

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SPakMgrPanel::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
	InitializeControllers();

	//const TSharedRef<SDockTab> DockTab = SNew(SDockTab).TabRole(ETabRole::MajorTab);
	// create & initialize tab manager
	TabManager = FGlobalTabmanager::Get()->NewTabManager(ConstructUnderMajorTab);
	
	TSharedRef<FWorkspaceItem> AppMenuGroup = TabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("PakMgrMenuGroupName", "Pak Mgr"));

	FName test;
	TabManager->RegisterTabSpawner(FileTreeTabId, FOnSpawnTab::CreateRaw(this, &SPakMgrPanel::HandleTabManagerSpawnTab, FileTreeTabId))
		.SetDisplayName(LOCTEXT("FileTreeTabTitle", "File Tree"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SessionFrontEnd.Tabs.Tools"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(PakMgrTabId, FOnSpawnTab::CreateRaw(this, &SPakMgrPanel::HandleTabManagerSpawnTab, PakMgrTabId))
		.SetDisplayName(LOCTEXT("PakMgrTitle", "Pak Manager"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SessionFrontEnd.Tabs.Tools"))
		.SetGroup(AppMenuGroup);

	TabManager->RegisterTabSpawner(ContentBrowserTabId, FOnSpawnTab::CreateRaw(this, &SPakMgrPanel::HandleTabManagerSpawnTab, ContentBrowserTabId))
		.SetDisplayName(LOCTEXT("ContentBrowserTitle", "Content Browser"))
		.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SessionFrontEnd.Tabs.Tools"))
		.SetGroup(AppMenuGroup);

	// create tab layout
	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("PakMgrLayout_v1.2")
		->AddArea
		(
			FTabManager::NewPrimaryArea()
			->SetOrientation(Orient_Horizontal)
			->Split
			(
				// session browser
				FTabManager::NewStack()
				->AddTab(ContentBrowserTabId, ETabState::OpenedTab)
				->SetHideTabWell(true)
				->SetSizeCoefficient(0.25f)
			)
			->Split
			(
				// applications
				FTabManager::NewStack()
				->AddTab(FileTreeTabId, ETabState::OpenedTab)
				->AddTab(PakMgrTabId, ETabState::OpenedTab)
				->SetSizeCoefficient(0.75f)
				->SetForegroundTab(FileTreeTabId)
			)
		);

	// create & initialize main menu
	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder(TSharedPtr<FUICommandList>());

	MenuBarBuilder.AddPullDownMenu(
		LOCTEXT("WindowMenuLabel", "Window"),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateStatic(&SPakMgrPanel::FillWindowMenu, TabManager),
		"Window"
	);

	ChildSlot
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MenuBarBuilder.MakeWidget()
		]

	+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			TabManager->RestoreFrom(Layout, ConstructUnderWindow).ToSharedRef()
		]
		];

	// Tell tab-manager about the multi-box for platforms with a global menu bar
	TabManager->SetMenuMultiBox(MenuBarBuilder.GetMultiBox());
}

void SPakMgrPanel::InitializeControllers()
{
	// load required modules and objects
	IPakMgrModule& pakMgrModule = FModuleManager::LoadModuleChecked<IPakMgrModule>("PakMgr");

	// create controllers
	PFileManager = pakMgrModule.GetPFileManager();
}

void SPakMgrPanel::FillWindowMenu(FMenuBuilder& MenuBuilder, const TSharedPtr<FTabManager> TabManager)
{
	if (!TabManager.IsValid())
	{
		return;
	}

#if !WITH_EDITOR
	FGlobalTabmanager::Get()->PopulateTabSpawnerMenu(MenuBuilder, WorkspaceMenu::GetMenuStructure().GetStructureRoot());
#endif //!WITH_EDITOR

	TabManager->PopulateLocalTabSpawnerMenu(MenuBuilder);
}

TSharedRef<SDockTab> SPakMgrPanel::HandleTabManagerSpawnTab(const FSpawnTabArgs& Args, FName TabIdentifier) const
{
	TSharedPtr<SWidget> TabWidget = SNullWidget::NullWidget;

	TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::PanelTab);

	if (TabIdentifier == FileTreeTabId)
	{
		TabWidget = SNew(SFileTree, PFileManager.ToSharedRef());
	}
	else if (TabIdentifier == PakMgrTabId)
	{
		TabWidget = SNew(SPakManager, PFileManager.ToSharedRef());
	}
	else if (TabIdentifier == ContentBrowserTabId)
	{
		auto tp = SNew(SContentBrowser, PFileManager.ToSharedRef());
		SContentBrowser::SetInstance(tp);
		TabWidget = tp;
	}

	DockTab->SetContent(TabWidget.ToSharedRef());

	return DockTab;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
