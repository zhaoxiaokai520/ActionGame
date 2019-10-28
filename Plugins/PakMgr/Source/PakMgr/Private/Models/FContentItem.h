// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "CoreMinimal.h"
#include "Text.h"
#include "Models/IFileInstanceInfo.h"

enum class EFileTreeNodeType
{
	Group,
	Instance,
	Session
};


/**
 * Base class for items in the session tree view.
 */
class FContentItem
{
public:

	/** Virtual destructor. */
	virtual ~FContentItem() { }

public:

	/**
	 * Adds a child process item to this item.
	 *
	 * @param The child item to add.
	 * @see ClearChildren, GetChildren
	 */
	void AddChild(const TSharedRef<FContentItem>& Child)
	{
		Children.Add(Child);
	}

	/**
	 * Clears the collection of child items.
	 *
	 * @see AddChild, GetChildren
	 */
	void ClearChildren()
	{
		Children.Reset();
	}

	/**
	 * Gets the child items.
	 *
	 * @return Child items.
	 * @see AddChild, ClearChildren, GetParent
	 */
	const TArray<TSharedPtr<FContentItem>>& GetChildren()
	{
		return Children;
	}

	/**
	 * Gets the parent item.
	 *
	 * @return Parent item.
	 * @see GetChildren, SetParent
	 */
	const TSharedPtr<FContentItem>& GetParent()
	{
		return Parent;
	}

	/**
	 * Sets the parent item.
	 *
	 * @param Node The parent item to set.
	 * @see GetParent
	 */
	void SetParent(const TSharedPtr<FContentItem>& Node)
	{
		Parent = Node;
	}

public:

	virtual EFileTreeNodeType GetType() = 0;

private:

	/** Holds the child process items. */
	TArray<TSharedPtr<FContentItem>> Children;

	/** Holds a pointer to the parent item. */
	TSharedPtr<FContentItem> Parent;
};


/**
 * Implements a group item in the session tree view.
 */
class FFileGroupTreeItem
	: public FContentItem
{
public:

	/** Creates and initializes a new instance. */
	FFileGroupTreeItem(const FText& InGroupName, const FText& InToolTipText)
		: GroupName(InGroupName)
		, ToolTipText(InToolTipText)
	{ }

	/** Virtual destructor. */
	virtual ~FFileGroupTreeItem() { }

public:

	/**
	 * Gets the name of the group associated with this item.
	 *
	 * @return Group name.
	 */
	FText GetGroupName() const
	{
		return GroupName;
	}

	/**
	 * Gets the tool tip text of the group associated with this item.
	 *
	 * @return Tool tip text.
	 */
	FText GetToolTipText() const
	{
		return ToolTipText;
	}

public:

	// FSessionBrowserTreeNode interface

	virtual EFileTreeNodeType GetType() override
	{
		return EFileTreeNodeType::Group;
	}

private:

	/** The name of the group associated with this item. */
	FText GroupName;

	/** The tool tip text. */
	FText ToolTipText;
};


/**
 * Implements a instance item in the session tree view.
 */
class FFileInstanceTreeItem
	: public FContentItem
{
public:

	/** Creates and initializes a new instance. */
	FFileInstanceTreeItem(const TSharedRef<IFileInstanceInfo>& InInstanceInfo)
		: InstanceInfo(InInstanceInfo)
	{
	
	}

	/** Virtual destructor. */
	virtual ~FFileInstanceTreeItem() { }

public:

	/**
	 * Gets the instance info associated with this item.
	 *
	 * @return Instance info.
	 */
	TSharedPtr<IFileInstanceInfo> GetInstanceInfo() const
	{
		return InstanceInfo.Pin();
	}

public:

	// FSessionBrowserTreeNode interface

	virtual EFileTreeNodeType GetType() override
	{
		return EFileTreeNodeType::Instance;
	}

private:

	/** Weak pointer to the instance info associated with this item. */
	TWeakPtr<IFileInstanceInfo> InstanceInfo;
};


/**
 * Implements a session item in the session tree view.
 */
class FFileSessionTreeItem
	: public FContentItem
{
public:

	/** Creates and initializes a new instance. */
	FFileSessionTreeItem()
	{ }

	/** Virtual destructor. */
	virtual ~FFileSessionTreeItem() { }

public:

	/**
	 * Gets the session info associated with this item.
	 *
	 * @return Session info.
	 */

public:

	// FSessionBrowserTreeNode interface

	virtual EFileTreeNodeType GetType() override
	{
		return EFileTreeNodeType::Session;
	}

private:

	/** Weak pointer to the session info associated with this item. */
};
