// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AssetData.h"
#include "EdGraph/EdGraph.h"
#include "Misc/AssetRegistryInterface.h"
#include "Models/RefNode.h"
#include "RefGraph.generated.h"

UCLASS()
class URefGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

public:	
	/** Set reference viewer to focus on these assets */
	void SetGraphRoot(const TArray<FAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin = FIntPoint(ForceInitToZero));

	/** Returns list of currently focused assets */
	const TArray<FAssetIdentifier>& GetCurrentGraphRootIdentifiers() const;

	/** Force the graph to rebuild */
	class URefNode* RebuildGraph();

private:
	URefNode* ConstructNodes(const TArray<FAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin);
	int32 RecursivelyGatherSizes(bool bReferencers, const TArray<FAssetIdentifier>& Identifiers, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FAssetIdentifier>& VisitedNames, TMap<FAssetIdentifier, int32>& OutNodeSizes) const;
	void GatherAssetData(const TSet<FName>& AllPackageNames, TMap<FName, FAssetData>& OutPackageToAssetDataMap) const;
	class URefNode* RecursivelyConstructNodes(bool bReferencers, URefNode* RootNode, const TArray<FAssetIdentifier>& Identifiers, const FIntPoint& NodeLoc, const TMap<FAssetIdentifier, int32>& NodeSizes, const TMap<FName, FAssetData>& PackagesToAssetDataMap, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FAssetIdentifier>& VisitedNames);

	EAssetRegistryDependencyType::Type GetReferenceSearchFlags(bool bHardOnly) const;
	bool ExceedsMaxSearchDepth(int32 Depth) const;
	bool ExceedsMaxSearchBreadth(int32 Breadth) const;
	URefNode* CreateReferenceNode();
	/** Removes all nodes from the graph */
	void RemoveAllNodes();
	/** Returns true if filtering is enabled and we have a valid collection */
	bool ShouldFilterByCollection() const;

private:

	TArray<FAssetIdentifier> CurrentGraphRootIdentifiers;
	FIntPoint CurrentGraphRootOrigin;

	int32 MaxSearchDepth;
	int32 MaxSearchBreadth;

	/** Current collection filter. NAME_None for no filter */
	FName CurrentCollectionFilter;
	bool bEnableCollectionFilter;

	bool bLimitSearchDepth;
	bool bLimitSearchBreadth;
	bool bIsShowSoftReferences;
	bool bIsShowHardReferences;
	bool bIsShowManagementReferences;
	bool bIsShowSearchableNames;
	bool bIsShowNativePackages;
};
