// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "AssetRegistryModule.h"
#include "Logging/LogMacros.h"
#include "Engine/AssetManagerTypes.h"
#include "IMessageBus.h"
#include "AssetRegistryState.h"
#include "SPakMgrPanel.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPakMgr, Log, All);

class FToolBarBuilder;
class FMenuBuilder;

/** Struct containing the information about currently investigated asset context */
struct FPakMgrRegistrySource
{
	/** Display name of the source, equal to target Platform, Editor, or Custom */
	FString SourceName;

	/** Filename registry was loaded from */
	FString SourceFilename;

	/** Target platform for this state, may be null */
	ITargetPlatform* TargetPlatform;

	/** Raw asset registry state, if bIsEditor is true this points to the real editor asset registry */
	const FAssetRegistryState* RegistryState;

	/** If true, this is the editor  */
	uint8 bIsEditor : 1;

	/** If true, management data has been initialized */
	uint8 bManagementDataInitialized : 1;

	/** Map of chunk information, from chunk id to primary assets/specific assets that are explicitly assigned to that chunk */
	TMap<int32, FAssetManagerChunkInfo> ChunkAssignments;

	/** If SourceName matches this, this is the editor */
	static const FString EditorSourceName;

	/** If SourceName matches this, this is the custom source loaded from an arbitrary path */
	static const FString CustomSourceName;

	FPakMgrRegistrySource()
		: TargetPlatform(nullptr)
		, RegistryState(nullptr)
		, bIsEditor(false)
		, bManagementDataInitialized(false)
	{
	}

	~FPakMgrRegistrySource()
	{
		if (RegistryState && !bIsEditor)
		{
			delete RegistryState;
		}
	}
};

class IPakMgrModule : public IModuleInterface
{
public:
	/**
 * Singleton-like access to this module's interface.  This is just for convenience!
 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
 *
 * @return Returns singleton instance, loading the module on demand if needed
 */
	static inline IPakMgrModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IPakMgrModule >("PakMgr");
	}
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();
	TSharedPtr<IPFileManager> GetPFileManager();

	/** Filters list of identifiers and removes ones that do not exist in this registry source. Handles replacing redirectors as well */
	bool FilterAssetIdentifiersForCurrentRegistrySource(TArray<FAssetIdentifier>& AssetIdentifiers, EAssetRegistryDependencyType::Type DependencyType = EAssetRegistryDependencyType::None, bool bForwardDependency = true);
	FAssetData FindAssetDataFromAnyPath(const FString& AnyAssetPath, FString& OutFailureReason);
	/** path get from OpenFileDialg() is a relative path, event if convert it to absolute path(ep. c:/xxx/GameProj/Content/xxx).
	* So we need a function to convert absolute path to /Game/xxx path, so that asset data can be got.
	* We just replace path start to last character of "Content" with /Game
	*/
	FString ConvertPhysicalPathToUFSPath(const FString& absFilePath);
	UPackage *GetMapPackage(const FString &ufsPath);
private:
	FString ConvertAnyPathToObjectPath(const FString& AnyAssetPath, FString& OutFailureReason);
	bool IsMapPackageAsset(const FString& ObjectPath);
	bool IsMapPackageAsset(const FString& ObjectPath, FString& MapFilePath);
	bool IsPackageFlagsSupportedForAssetLibrary(uint32 PackageFlags);
	FString RemoveFullName(const FString& AnyAssetPath, FString& OutFailureReason);
	bool IsAValidPath(const FString& Path, const TCHAR* InvalidChar, FString& OutFailureReason);
	FString ExtractPackageName(const FString& ObjectPath);
	void GetAssetDataInPath(const FString& Paths, TArray<FAssetData>& OutAssetData);
	bool HasValidRoot(const FString& ObjectPath);
	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);
	bool IsPackageInCurrentRegistrySource(FName PackageName);

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	TWeakPtr<SPakMgrPanel> WeakPakMgrPanel;
	/** Holds a weak pointer to the message bus. */
	TWeakPtr<IMessageBus, ESPMode::ThreadSafe> MessageBusPtr;
	/** Holds the session manager singleton. */
	TSharedPtr<IPFileManager> PFileManager;
	FPakMgrRegistrySource* CurrentRegistrySource;
	IAssetRegistry* AssetRegistry;
	FString GameContentPath;
};
