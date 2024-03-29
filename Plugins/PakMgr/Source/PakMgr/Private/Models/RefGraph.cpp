// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.


#include "RefGraph.h"
#include "ICollectionManager.h"
#include "AssetRegistryModule.h"
#include "CollectionManagerModule.h"
#include "PakMgrModule.h"
#include "Engine/AssetManager.h"

URefGraph::URefGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MaxSearchDepth = 1;
	MaxSearchBreadth = 15;

	bLimitSearchDepth = true;
	bLimitSearchBreadth = true;
	bIsShowSoftReferences = true;
	bIsShowHardReferences = true;
	bIsShowManagementReferences = false;
	bIsShowSearchableNames = false;
	bIsShowNativePackages = false;
}

void URefGraph::SetGraphRoot(const TArray<FAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin)
{
	CurrentGraphRootIdentifiers = GraphRootIdentifiers;
	CurrentGraphRootOrigin = GraphRootOrigin;

	// If we're focused on a searchable name, enable that flag
	for (const FAssetIdentifier& AssetId : GraphRootIdentifiers)
	{
		if (AssetId.IsValue())
		{
			bIsShowSearchableNames = true;
		}
		else if (AssetId.GetPrimaryAssetId().IsValid())
		{
			if (UAssetManager::IsValid())
			{
				UAssetManager::Get().UpdateManagementDatabase();
			}

			bIsShowManagementReferences = true;
		}
	}
}

const TArray<FAssetIdentifier>& URefGraph::GetCurrentGraphRootIdentifiers() const
{
	return CurrentGraphRootIdentifiers;
}

URefNode* URefGraph::RebuildGraph()
{
	RemoveAllNodes();
	URefNode* NewRootNode = ConstructNodes(CurrentGraphRootIdentifiers, CurrentGraphRootOrigin);
	NotifyGraphChanged();

	return NewRootNode;
}

URefNode* URefGraph::ConstructNodes(const TArray<FAssetIdentifier>& GraphRootIdentifiers, const FIntPoint& GraphRootOrigin)
{
	URefNode* RootNode = NULL;

	if (GraphRootIdentifiers.Num() > 0)
	{
		TSet<FName> AllowedPackageNames;
		if (ShouldFilterByCollection())
		{
			FCollectionManagerModule& CollectionManagerModule = FCollectionManagerModule::GetModule();
			TArray<FName> AssetPaths;
			CollectionManagerModule.Get().GetAssetsInCollection(CurrentCollectionFilter, ECollectionShareType::CST_All, AssetPaths);
			AllowedPackageNames.Reserve(AssetPaths.Num());
			for (FName AssetPath : AssetPaths)
			{
				AllowedPackageNames.Add(FName(*FPackageName::ObjectPathToPackageName(AssetPath.ToString())));
			}
		}

		TMap<FAssetIdentifier, int32> ReferencerNodeSizes;
		TSet<FAssetIdentifier> VisitedReferencerSizeNames;
		int32 ReferencerDepth = 1;
		RecursivelyGatherSizes(/*bReferencers=*/true, GraphRootIdentifiers, AllowedPackageNames, ReferencerDepth, VisitedReferencerSizeNames, ReferencerNodeSizes);

		TMap<FAssetIdentifier, int32> DependencyNodeSizes;
		TSet<FAssetIdentifier> VisitedDependencySizeNames;
		int32 DependencyDepth = 1;
		RecursivelyGatherSizes(/*bReferencers=*/false, GraphRootIdentifiers, AllowedPackageNames, DependencyDepth, VisitedDependencySizeNames, DependencyNodeSizes);

		TSet<FName> AllPackageNames;

		auto AddPackage = [](const FAssetIdentifier& AssetId, TSet<FName>& PackageNames)
		{
			// Only look for asset data if this is a package
			if (!AssetId.IsValue())
			{
				PackageNames.Add(AssetId.PackageName);
			}
		};

		for (const FAssetIdentifier& AssetId : VisitedReferencerSizeNames)
		{
			AddPackage(AssetId, AllPackageNames);
		}

		for (const FAssetIdentifier& AssetId : VisitedDependencySizeNames)
		{
			AddPackage(AssetId, AllPackageNames);
		}

		TMap<FName, FAssetData> PackagesToAssetDataMap;
		GatherAssetData(AllPackageNames, PackagesToAssetDataMap);

		// Create the root node
		RootNode = CreateReferenceNode();
		RootNode->SetupReferenceNode(GraphRootOrigin, GraphRootIdentifiers, PackagesToAssetDataMap.FindRef(GraphRootIdentifiers[0].PackageName));

		TSet<FAssetIdentifier> VisitedReferencerNames;
		int32 VisitedReferencerDepth = 1;
		RecursivelyConstructNodes(/*bReferencers=*/true, RootNode, GraphRootIdentifiers, GraphRootOrigin, ReferencerNodeSizes, PackagesToAssetDataMap, AllowedPackageNames, VisitedReferencerDepth, VisitedReferencerNames);

		TSet<FAssetIdentifier> VisitedDependencyNames;
		int32 VisitedDependencyDepth = 1;
		RecursivelyConstructNodes(/*bReferencers=*/false, RootNode, GraphRootIdentifiers, GraphRootOrigin, DependencyNodeSizes, PackagesToAssetDataMap, AllowedPackageNames, VisitedDependencyDepth, VisitedDependencyNames);
	}

	return RootNode;
}

int32 URefGraph::RecursivelyGatherSizes(bool bReferencers, const TArray<FAssetIdentifier>& Identifiers, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FAssetIdentifier>& VisitedNames, TMap<FAssetIdentifier, int32>& OutNodeSizes) const
{
	check(Identifiers.Num() > 0);

	VisitedNames.Append(Identifiers);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetIdentifier> ReferenceNames;

	if (bReferencers)
	{
		for (const FAssetIdentifier& AssetId : Identifiers)
		{
			AssetRegistryModule.Get().GetReferencers(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}
	else
	{
		for (const FAssetIdentifier& AssetId : Identifiers)
		{
			AssetRegistryModule.Get().GetDependencies(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}

	if (!bIsShowNativePackages)
	{
		auto RemoveNativePackage = [](const FAssetIdentifier& InAsset) { return InAsset.PackageName.ToString().StartsWith(TEXT("/Script")) && !InAsset.IsValue(); };

		ReferenceNames.RemoveAll(RemoveNativePackage);
	}

	int32 NodeSize = 0;
	if (ReferenceNames.Num() > 0 && !ExceedsMaxSearchDepth(CurrentDepth))
	{
		int32 NumReferencesMade = 0;
		int32 NumReferencesExceedingMax = 0;

		// Filter for our registry source
		IPakMgrModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(ReferenceNames, GetReferenceSearchFlags(false), !bReferencers);

		// Since there are referencers, use the size of all your combined referencers.
		// Do not count your own size since there could just be a horizontal line of nodes
		for (FAssetIdentifier& AssetId : ReferenceNames)
		{
			if (!VisitedNames.Contains(AssetId) && (!AssetId.IsPackage() || !ShouldFilterByCollection() || AllowedPackageNames.Contains(AssetId.PackageName)))
			{
				if (!ExceedsMaxSearchBreadth(NumReferencesMade))
				{
					TArray<FAssetIdentifier> NewPackageNames;
					NewPackageNames.Add(AssetId);
					NodeSize += RecursivelyGatherSizes(bReferencers, NewPackageNames, AllowedPackageNames, CurrentDepth + 1, VisitedNames, OutNodeSizes);
					NumReferencesMade++;
				}
				else
				{
					NumReferencesExceedingMax++;
				}
			}
		}

		if (NumReferencesExceedingMax > 0)
		{
			// Add one size for the collapsed node
			NodeSize++;
		}
	}

	if (NodeSize == 0)
	{
		// If you have no valid children, the node size is just 1 (counting only self to make a straight line)
		NodeSize = 1;
	}

	OutNodeSizes.Add(Identifiers[0], NodeSize);
	return NodeSize;
}

EAssetRegistryDependencyType::Type URefGraph::GetReferenceSearchFlags(bool bHardOnly) const
{
	int32 ReferenceFlags = 0;

	if (bIsShowSoftReferences && !bHardOnly)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::Soft;
	}
	if (bIsShowHardReferences)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::Hard;
	}
	if (bIsShowSearchableNames && !bHardOnly)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::SearchableName;
	}
	if (bIsShowManagementReferences)
	{
		ReferenceFlags |= EAssetRegistryDependencyType::HardManage;
		if (!bHardOnly)
		{
			ReferenceFlags |= EAssetRegistryDependencyType::SoftManage;
		}
	}

	return (EAssetRegistryDependencyType::Type)ReferenceFlags;
}

void URefGraph::GatherAssetData(const TSet<FName>& AllPackageNames, TMap<FName, FAssetData>& OutPackageToAssetDataMap) const
{
	// Take a guess to find the asset instead of searching for it. Most packages have a single asset in them with the same name as the package.
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	FARFilter Filter;
	for (auto PackageIt = AllPackageNames.CreateConstIterator(); PackageIt; ++PackageIt)
	{
		const FString& PackageName = (*PackageIt).ToString();
		const FString& PackagePath = PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
		Filter.ObjectPaths.Add(FName(*PackagePath));
	}

	TArray<FAssetData> AssetDataList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetDataList);
	for (auto AssetIt = AssetDataList.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		OutPackageToAssetDataMap.Add((*AssetIt).PackageName, *AssetIt);
	}
}

URefNode* URefGraph::RecursivelyConstructNodes(bool bReferencers, URefNode* RootNode, const TArray<FAssetIdentifier>& Identifiers, const FIntPoint& NodeLoc, const TMap<FAssetIdentifier, int32>& NodeSizes, const TMap<FName, FAssetData>& PackagesToAssetDataMap, const TSet<FName>& AllowedPackageNames, int32 CurrentDepth, TSet<FAssetIdentifier>& VisitedNames)
{
	check(Identifiers.Num() > 0);

	VisitedNames.Append(Identifiers);

	URefNode* NewNode = NULL;
	if (RootNode->GetIdentifier() == Identifiers[0])
	{
		// Don't create the root node. It is already created!
		NewNode = RootNode;
	}
	else
	{
		NewNode = CreateReferenceNode();
		NewNode->SetupReferenceNode(NodeLoc, Identifiers, PackagesToAssetDataMap.FindRef(Identifiers[0].PackageName));
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetIdentifier> ReferenceNames;
	TArray<FAssetIdentifier> HardReferenceNames;
	if (bReferencers)
	{
		for (const FAssetIdentifier& AssetId : Identifiers)
		{
			AssetRegistryModule.Get().GetReferencers(AssetId, HardReferenceNames, GetReferenceSearchFlags(true));
			AssetRegistryModule.Get().GetReferencers(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}
	else
	{
		for (const FAssetIdentifier& AssetId : Identifiers)
		{
			AssetRegistryModule.Get().GetDependencies(AssetId, HardReferenceNames, GetReferenceSearchFlags(true));
			AssetRegistryModule.Get().GetDependencies(AssetId, ReferenceNames, GetReferenceSearchFlags(false));
		}
	}

	if (!bIsShowNativePackages)
	{
		auto RemoveNativePackage = [](const FAssetIdentifier& InAsset) { return InAsset.PackageName.ToString().StartsWith(TEXT("/Script")) && !InAsset.IsValue(); };

		HardReferenceNames.RemoveAll(RemoveNativePackage);
		ReferenceNames.RemoveAll(RemoveNativePackage);
	}

	if (ReferenceNames.Num() > 0 && !ExceedsMaxSearchDepth(CurrentDepth))
	{
		FIntPoint ReferenceNodeLoc = NodeLoc;

		if (bReferencers)
		{
			// Referencers go left
			ReferenceNodeLoc.X -= 800;
		}
		else
		{
			// Dependencies go right
			ReferenceNodeLoc.X += 800;
		}

		const int32 NodeSizeY = 200;
		const int32 TotalReferenceSizeY = NodeSizes.FindChecked(Identifiers[0]) * NodeSizeY;

		ReferenceNodeLoc.Y -= TotalReferenceSizeY * 0.5f;
		ReferenceNodeLoc.Y += NodeSizeY * 0.5f;

		int32 NumReferencesMade = 0;
		int32 NumReferencesExceedingMax = 0;

		// Filter for our registry source
		IPakMgrModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(ReferenceNames, GetReferenceSearchFlags(false), !bReferencers);
		IPakMgrModule::Get().FilterAssetIdentifiersForCurrentRegistrySource(HardReferenceNames, GetReferenceSearchFlags(false), !bReferencers);

		for (int32 RefIdx = 0; RefIdx < ReferenceNames.Num(); ++RefIdx)
		{
			FAssetIdentifier ReferenceName = ReferenceNames[RefIdx];

			if (!VisitedNames.Contains(ReferenceName) && (!ReferenceName.IsPackage() || !ShouldFilterByCollection() || AllowedPackageNames.Contains(ReferenceName.PackageName)))
			{
				bool bIsHardReference = HardReferenceNames.Contains(ReferenceName);

				if (!ExceedsMaxSearchBreadth(NumReferencesMade))
				{
					int32 ThisNodeSizeY = ReferenceName.IsValue() ? 100 : NodeSizeY;

					const int32 RefSizeY = NodeSizes.FindChecked(ReferenceName);
					FIntPoint RefNodeLoc;
					RefNodeLoc.X = ReferenceNodeLoc.X;
					RefNodeLoc.Y = ReferenceNodeLoc.Y + RefSizeY * ThisNodeSizeY * 0.5 - ThisNodeSizeY * 0.5;

					TArray<FAssetIdentifier> NewIdentifiers;
					NewIdentifiers.Add(ReferenceName);

					URefNode* ReferenceNode = RecursivelyConstructNodes(bReferencers, RootNode, NewIdentifiers, RefNodeLoc, NodeSizes, PackagesToAssetDataMap, AllowedPackageNames, CurrentDepth + 1, VisitedNames);
					if (bIsHardReference)
					{
						if (bReferencers)
						{
							ReferenceNode->GetDependencyPin()->PinType.PinCategory = TEXT("hard");
						}
						else
						{
							ReferenceNode->GetReferencerPin()->PinType.PinCategory = TEXT("hard"); //-V595
						}
					}

					if (ensure(ReferenceNode))
					{
						if (bReferencers)
						{
							NewNode->AddReferencer(ReferenceNode);
						}
						else
						{
							ReferenceNode->AddReferencer(NewNode);
						}

						ReferenceNodeLoc.Y += RefSizeY * ThisNodeSizeY;
					}

					NumReferencesMade++;
				}
				else
				{
					NumReferencesExceedingMax++;
				}
			}
		}

		if (NumReferencesExceedingMax > 0)
		{
			// There are more references than allowed to be displayed. Make a collapsed node.
			URefNode* ReferenceNode = CreateReferenceNode();
			FIntPoint RefNodeLoc;
			RefNodeLoc.X = ReferenceNodeLoc.X;
			RefNodeLoc.Y = ReferenceNodeLoc.Y;

			if (ensure(ReferenceNode))
			{
				ReferenceNode->SetReferenceNodeCollapsed(RefNodeLoc, NumReferencesExceedingMax);

				if (bReferencers)
				{
					NewNode->AddReferencer(ReferenceNode);
				}
				else
				{
					ReferenceNode->AddReferencer(NewNode);
				}
			}
		}
	}

	return NewNode;
}

bool URefGraph::ExceedsMaxSearchDepth(int32 Depth) const
{
	return bLimitSearchDepth && Depth > MaxSearchDepth;
}

bool URefGraph::ExceedsMaxSearchBreadth(int32 Breadth) const
{
	return bLimitSearchBreadth && Breadth > MaxSearchBreadth;
}

URefNode* URefGraph::CreateReferenceNode()
{
	const bool bSelectNewNode = false;
	return Cast<URefNode>(CreateNode(URefNode::StaticClass(), bSelectNewNode));
}

void URefGraph::RemoveAllNodes()
{
	TArray<UEdGraphNode*> NodesToRemove = Nodes;
	for (int32 NodeIndex = 0; NodeIndex < NodesToRemove.Num(); ++NodeIndex)
	{
		RemoveNode(NodesToRemove[NodeIndex]);
	}
}

bool URefGraph::ShouldFilterByCollection() const
{
	return bEnableCollectionFilter && CurrentCollectionFilter != NAME_None;
}


