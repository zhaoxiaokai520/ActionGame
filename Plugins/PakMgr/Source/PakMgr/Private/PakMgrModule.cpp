// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PakMgrModule.h"
#include "PakMgrStyle.h"
#include "PakMgrCommands.h"
#include "Algo/Count.h"
#include "LevelEditor.h"
#include "PFileManager.h"
#include "IMessagingModule.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

static const FName PakMgrTabName("PakMgrModule");

const FString FPakMgrRegistrySource::EditorSourceName = TEXT("Editor");
const FString FPakMgrRegistrySource::CustomSourceName = TEXT("Custom");

DEFINE_LOG_CATEGORY(LogPakMgr);

#define LOCTEXT_NAMESPACE "PakMgrModule"

void IPakMgrModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	
	FPakMgrStyle::Initialize();
	FPakMgrStyle::ReloadTextures();

	FPakMgrCommands::Register();
	
	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FPakMgrCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &IPakMgrModule::PluginButtonClicked),
		FCanExecuteAction());
		
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	
	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuExtension("WindowLayout", EExtensionHook::After, PluginCommands, FMenuExtensionDelegate::CreateRaw(this, &IPakMgrModule::AddMenuExtension));

		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
	
	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension("Settings", EExtensionHook::After, PluginCommands, FToolBarExtensionDelegate::CreateRaw(this, &IPakMgrModule::AddToolbarExtension));
		
		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
	
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(PakMgrTabName, FOnSpawnTab::CreateRaw(this, &IPakMgrModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("FPakMgrTabTitle", "PakMgr"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	MessageBusPtr = IMessagingModule::Get().GetDefaultBus();

	GameContentPath = FString() / FApp::GetProjectName() / TEXT("Content");
}

void IPakMgrModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FPakMgrStyle::Shutdown();

	FPakMgrCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(PakMgrTabName);
}

TSharedRef<SDockTab> IPakMgrModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	//FText WidgetText = FText::Format(
	//	LOCTEXT("WindowWidgetText", "Add code to {0} in {1} to override this window's contents"),
	//	FText::FromString(TEXT("FPakMgrModule::OnSpawnPluginTab")),
	//	FText::FromString(TEXT("PakMgr.cpp"))
	//	);

	//return SNew(SDockTab)
	//	.TabRole(ETabRole::NomadTab)
	//	[
	//		// Put your tab content here!
	//		SNew(SBox)
	//		.HAlign(HAlign_Center)
	//		.VAlign(VAlign_Center)
	//		[
	//			SNew(STextBlock)
	//			.Text(WidgetText)
	//		]
	//	];

	const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
		.TabRole(ETabRole::MajorTab);

	TSharedRef<SPakMgrPanel> panel = SNew(SPakMgrPanel, DockTab, SpawnTabArgs.GetOwnerWindow());
	WeakPakMgrPanel = panel;

	DockTab->SetContent(panel);

	return DockTab;
}

bool IPakMgrModule::FilterAssetIdentifiersForCurrentRegistrySource(TArray<FAssetIdentifier>& AssetIdentifiers, EAssetRegistryDependencyType::Type DependencyType, bool bForwardDependency)
{
	bool bMadeChange = false;
	if (!CurrentRegistrySource || !CurrentRegistrySource->RegistryState || CurrentRegistrySource->bIsEditor)
	{
		return bMadeChange;
	}

	for (int32 Index = 0; Index < AssetIdentifiers.Num(); Index++)
	{
		FName PackageName = AssetIdentifiers[Index].PackageName;

		if (PackageName != NAME_None)
		{
			if (!IsPackageInCurrentRegistrySource(PackageName))
			{
				// Remove bad package
				AssetIdentifiers.RemoveAt(Index);

				if (DependencyType != EAssetRegistryDependencyType::None)
				{
					// If this is a redirector replace with references
					TArray<FAssetData> Assets;
					AssetRegistry->GetAssetsByPackageName(PackageName, Assets, true);

					for (const FAssetData& Asset : Assets)
					{
						if (Asset.IsRedirector())
						{
							TArray<FAssetIdentifier> FoundReferences;

							if (bForwardDependency)
							{
								CurrentRegistrySource->RegistryState->GetDependencies(PackageName, FoundReferences, DependencyType);
							}
							else
							{
								CurrentRegistrySource->RegistryState->GetReferencers(PackageName, FoundReferences, DependencyType);
							}

							AssetIdentifiers.Insert(FoundReferences, Index);
							break;
						}
					}
				}

				// Need to redo this index, it was either removed or replaced
				Index--;
			}
		}
	}
	return bMadeChange;
}

bool IPakMgrModule::IsPackageInCurrentRegistrySource(FName PackageName)
{
	if (CurrentRegistrySource && CurrentRegistrySource->RegistryState && !CurrentRegistrySource->bIsEditor)
	{
		const FAssetPackageData* FoundData = CurrentRegistrySource->RegistryState->GetAssetPackageData(PackageName);

		if (!FoundData || FoundData->DiskSize < 0)
		{
			return false;
		}
	}

	// In editor, no packages are filtered
	return true;
}

FAssetData IPakMgrModule::FindAssetDataFromAnyPath(const FString& AnyAssetPath, FString& OutFailureReason)
{
	//FString ObjectPath = ConvertAnyPathToObjectPath(ConvertPhysicalPathToUFSPath(AnyAssetPath), OutFailureReason);
	FString ObjectPath = ConvertPhysicalPathToUFSPath(AnyAssetPath);
	UPackage *mapPackage = GetMapPackage(ObjectPath);
	UE_LOG(LogPakMgr, Warning, TEXT("map package name %s logicName:%s"), *mapPackage->GetFullName(), *mapPackage->GetFName().ToString());
	if (ObjectPath.IsEmpty())
	{
		return FAssetData();
	}

	//if (IsMapPackageAsset(ObjectPath))
	//{
	//	OutFailureReason = FString::Printf(TEXT("The AssetData '%s' is not accessible because it is of type Map/Level."), *ObjectPath);
	//	return FAssetData();
	//}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	FAssetData AssetData = AssetRegistryModule.Get().GetAssetByObjectPath(*ObjectPath);
	if (!AssetData.IsValid())
	{
		OutFailureReason = FString::Printf(TEXT("The AssetData '%s' could not be found in the Content Browser."), *ObjectPath);
		return FAssetData();
	}

	//// Prevent loading a umap...
	//if (!IsPackageFlagsSupportedForAssetLibrary(AssetData.PackageFlags))
	//{
	//	OutFailureReason = FString::Printf(TEXT("The AssetData '%s' is not accessible because it is of type Map/Level."), *ObjectPath);
	//	return FAssetData();
	//}
	return AssetData;
}

void IPakMgrModule::GetAssetDataInPath(const FString& Path, TArray<FAssetData>& OutAssetData)
{
	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	new (Filter.PackagePaths) FName(*Path);

	// Query for a list of assets in the selected paths
	AssetRegistry->GetAssets(Filter, OutAssetData);
}

bool IPakMgrModule::IsPackageFlagsSupportedForAssetLibrary(uint32 PackageFlags)
{
	return (PackageFlags & (PKG_ContainsMap | PKG_PlayInEditor | PKG_ContainsMapData)) == 0;
}

bool IPakMgrModule::IsMapPackageAsset(const FString& ObjectPath)
{
	FString MapFilePath;
	return IsMapPackageAsset(ObjectPath, MapFilePath);
}

bool IPakMgrModule::IsMapPackageAsset(const FString& ObjectPath, FString& MapFilePath)
{
	const FString PackageName = ExtractPackageName(ObjectPath);
	if (PackageName.Len() > 0)
	{
		FString PackagePath;
		if (FPackageName::DoesPackageExist(PackageName, NULL, &PackagePath))
		{
			const FString FileExtension = FPaths::GetExtension(PackagePath, true);
			if (FileExtension == FPackageName::GetMapPackageExtension())
			{
				MapFilePath = PackagePath;
				return true;
			}
		}
	}

	return false;
}

FString IPakMgrModule::ExtractPackageName(const FString& ObjectPath)
{
	// To find the package name in an object path we need to find the path left of the FIRST delimiter.
	// Assets like BSPs, lightmaps etc. can have multiple '.' delimiters.
	const int32 PackageDelimiterPos = ObjectPath.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromStart);
	if (PackageDelimiterPos != INDEX_NONE)
	{
		return ObjectPath.Left(PackageDelimiterPos);
	}

	return ObjectPath;
}

FString IPakMgrModule::ConvertAnyPathToObjectPath(const FString& AnyAssetPath, FString& OutFailureReason)
{
	if (AnyAssetPath.Len() < 2) // minimal length to have /G
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because the Root path need to be specified. ie /Game/"), *AnyAssetPath);
		return FString();
	}

	// Remove class name from Reference Path
	FString TextPath = FPackageName::ExportTextPathToObjectPath(AnyAssetPath);

	// Remove class name Fullname
	TextPath = RemoveFullName(TextPath, OutFailureReason);
	if (TextPath.IsEmpty())
	{
		return FString();
	}

	// Extract the subobject path if any
	FString SubObjectPath;
	int32 SubObjectDelimiterIdx;
	if (TextPath.FindChar(SUBOBJECT_DELIMITER_CHAR, SubObjectDelimiterIdx))
	{
		SubObjectPath = TextPath.Mid(SubObjectDelimiterIdx + 1);
		TextPath = TextPath.Left(SubObjectDelimiterIdx);
	}

	// Convert \ to /
	TextPath.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);
	FPaths::RemoveDuplicateSlashes(TextPath);

	// Get asset full name, i.e."PackageName.ObjectName:InnerAssetName.2ndInnerAssetName" from "/Game/Folder/PackageName.ObjectName:InnerAssetName.2ndInnerAssetName"
	FString AssetFullName;
	{
		// Get everything after the last slash
		int32 IndexOfLastSlash = INDEX_NONE;
		TextPath.FindLastChar('/', IndexOfLastSlash);

		FString Folders = TextPath.Left(IndexOfLastSlash);
		// Test for invalid characters
		if (!IsAValidPath(Folders, INVALID_LONGPACKAGE_CHARACTERS, OutFailureReason))
		{
			return FString();
		}

		AssetFullName = TextPath.Mid(IndexOfLastSlash + 1);
	}

	// Get the object name
	FString ObjectName = FPackageName::ObjectPathToObjectName(AssetFullName);
	if (ObjectName.IsEmpty())
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it doesn't contain an asset name."), *AnyAssetPath);
		return FString();
	}

	// Test for invalid characters
	if (!IsAValidPath(ObjectName, INVALID_OBJECTNAME_CHARACTERS, OutFailureReason))
	{
		return FString();
	}

	// Confirm that we have a valid Root Package and get the valid PackagePath /Game/MyFolder/MyAsset
	FString PackagePath;
	if (!FPackageName::TryConvertFilenameToLongPackageName(TextPath, PackagePath, &OutFailureReason))
	{
		return FString();
	}

	if (PackagePath.Len() == 0)
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert path '%s' because the PackagePath is empty."), *AnyAssetPath);
		return FString();
	}

	if (PackagePath[0] != TEXT('/'))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert path '%s' because the PackagePath '%s' doesn't start with a '/'."), *AnyAssetPath, *PackagePath);
		return FString();
	}

	FString ObjectPath = FString::Printf(TEXT("%s.%s"), *PackagePath, *ObjectName);

	if (FPackageName::IsScriptPackage(ObjectPath))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it start with /Script/"), *AnyAssetPath);
		return FString();
	}
	if (FPackageName::IsMemoryPackage(ObjectPath))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it start with /Memory/"), *AnyAssetPath);
		return FString();
	}

	// Confirm that the PackagePath starts with a valid root
	if (!HasValidRoot(PackagePath))
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path '%s' because it does not map to a root."), *AnyAssetPath);
		return FString();
	}

	return ObjectPath;
}

UPackage *IPakMgrModule::GetMapPackage(const FString &ufsPath)
{
	//remove map file extension
	FString MapPackageName(ufsPath.Left(ufsPath.Len() - 5));
	// If this map's UPackage exists, it is currently loaded
	UPackage* MapPackage = FindObject<UPackage>(ANY_PACKAGE, *MapPackageName, true);
	return MapPackage;
}

FString IPakMgrModule::ConvertPhysicalPathToUFSPath(const FString& absFilePath)
{
	FString retStr = absFilePath;
	retStr.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);
	int endC = retStr.Find("/Content/") + 9;
	return "/Game/" / retStr.RightChop(endC);
}

/** Remove Class from "Class /Game/MyFolder/MyAsset" */
FString IPakMgrModule::RemoveFullName(const FString& AnyAssetPath, FString& OutFailureReason)
{
	FString Result = AnyAssetPath.TrimStartAndEnd();
	int32 NumberOfSpace = Algo::Count(AnyAssetPath, TEXT(' '));

	if (NumberOfSpace == 0)
	{
		return MoveTemp(Result);
	}
	else if (NumberOfSpace > 1)
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert path '%s' because there are too many spaces."), *AnyAssetPath);
		return FString();
	}
	else// if (NumberOfSpace == 1)
	{
		int32 FoundIndex = 0;
		AnyAssetPath.FindChar(TEXT(' '), FoundIndex);
		check(FoundIndex > INDEX_NONE && FoundIndex < AnyAssetPath.Len()); // because of TrimStartAndEnd

		// Confirm that it's a valid Class
		FString ClassName = AnyAssetPath.Left(FoundIndex);

		// Convert \ to /
		ClassName.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);

		// Test ClassName for invalid Char
		const int32 StrLen = FCString::Strlen(INVALID_OBJECTNAME_CHARACTERS);
		for (int32 Index = 0; Index < StrLen; ++Index)
		{
			int32 InvalidFoundIndex = 0;
			if (ClassName.FindChar(INVALID_OBJECTNAME_CHARACTERS[Index], InvalidFoundIndex))
			{
				OutFailureReason = FString::Printf(TEXT("Can't convert the path %s because it contains invalid characters (probably spaces)."), *AnyAssetPath);
				return FString();
			}
		}

		// Return the path without the Class name
		return AnyAssetPath.Mid(FoundIndex + 1);
	}
}

// Test for invalid characters
bool IPakMgrModule::IsAValidPath(const FString& Path, const TCHAR* InvalidChar, FString& OutFailureReason)
{
	// Like !FName::IsValidGroupName(Path)), but with another list and no conversion to from FName
	// InvalidChar may be INVALID_OBJECTPATH_CHARACTERS or INVALID_LONGPACKAGE_CHARACTERS or ...
	const int32 StrLen = FCString::Strlen(InvalidChar);
	for (int32 Index = 0; Index < StrLen; ++Index)
	{
		int32 FoundIndex = 0;
		if (Path.FindChar(InvalidChar[Index], FoundIndex))
		{
			OutFailureReason = FString::Printf(TEXT("Can't convert the path %s because it contains invalid characters."), *Path);
			return false;
		}
	}

	if (Path.Len() > FPlatformMisc::GetMaxPathLength())
	{
		OutFailureReason = FString::Printf(TEXT("Can't convert the path %s because it is too long; this may interfere with cooking for consoles. Unreal filenames should be no longer than %d characters."), *Path, FPlatformMisc::GetMaxPathLength());
		return false;
	}
	return true;
}

bool IPakMgrModule::HasValidRoot(const FString& ObjectPath)
{
	FString Filename;
	bool bValidRoot = true;
	if (!ObjectPath.IsEmpty() && ObjectPath[ObjectPath.Len() - 1] == TEXT('/'))
	{
		bValidRoot = FPackageName::TryConvertLongPackageNameToFilename(ObjectPath, Filename);
	}
	else
	{
		FString ObjectPathWithSlash = ObjectPath;
		ObjectPathWithSlash.AppendChar(TEXT('/'));
		bValidRoot = FPackageName::TryConvertLongPackageNameToFilename(ObjectPathWithSlash, Filename);
	}

	return bValidRoot;
}

void IPakMgrModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->InvokeTab(PakMgrTabName);
}

void IPakMgrModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FPakMgrCommands::Get().OpenPluginWindow);
}

void IPakMgrModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FPakMgrCommands::Get().OpenPluginWindow);
}

TSharedPtr<IPFileManager> IPakMgrModule::GetPFileManager()
{
	if (!PFileManager.IsValid() && MessageBusPtr.IsValid())
	{
		PFileManager = MakeShareable(new FPFileManager(MessageBusPtr.Pin().ToSharedRef()));
	}

	return PFileManager;
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(IPakMgrModule, PakMgr)