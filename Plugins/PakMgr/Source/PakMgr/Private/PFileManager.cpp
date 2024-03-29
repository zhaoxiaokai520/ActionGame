// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Containers/Ticker.h"
#include "EngineServiceMessages.h"
#include "MessageEndpointBuilder.h"
//#include "SessionServiceMessages.h"
#include "Models/FileInfo.h"


/* FSessionManager structors
 *****************************************************************************/

FPFileManager::FPFileManager(const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InMessageBus)
	: MessageBusPtr(InMessageBus)
{
	// fill in the owner array
	FString Filter;

	if (FParse::Value(FCommandLine::Get(), TEXT("SessionFilter="), Filter))
	{
		// Allow support for -SessionFilter=Filter1+Filter2+Filter3
		int32 PlusIdx = Filter.Find(TEXT("+"));

		while (PlusIdx != INDEX_NONE)
		{
			FString Owner = Filter.Left(PlusIdx);
			FilteredOwners.Add(Owner);
			Filter = Filter.Right(Filter.Len() - (PlusIdx + 1));
			PlusIdx = Filter.Find(TEXT("+"));
		}

		FilteredOwners.Add(Filter);
	}

	// connect to message bus
	//MessageEndpoint = FMessageEndpoint::Builder("FSessionManager", InMessageBus)
	//	.Handling<FEngineServicePong>(this, &FPFileManager::HandleEnginePongMessage);
		//.Handling<FSessionServicePong>(this, &FPFileManager::HandleSessionPongMessage);

	// initialize ticker
	TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FPFileManager::HandleTicker), 1.f);

	SendPing();
}


FPFileManager::~FPFileManager()
{
	FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
}


/* ISessionManager interface
 *****************************************************************************/

void FPFileManager::AddOwner(const FString& InOwner)
{
	FilteredOwners.Add(InOwner);
}


const TArray<TSharedPtr<IFileInstanceInfo>>& FPFileManager::GetSelectedInstances() const
{
	return SelectedInstances;
}


const TSharedPtr<IFileInfo>& FPFileManager::GetSelectedSession() const
{
	return SelectedSession;
}

void OnAddModule(FString& moduleName)
{

}

void FPFileManager::GetSessions(TArray<TSharedPtr<IFileInfo>>& OutSessions) const
{
	OutSessions.Empty(Sessions.Num());

	//for (TMap<FGuid, TSharedPtr<FFileInfo> >::TConstIterator It(Sessions); It; ++It)
	//{
	//	OutSessions.Add(It.Value());
	//}
}


bool FPFileManager::IsInstanceSelected(const TSharedRef<IFileInstanceInfo>& Instance) const
{
	return ((Instance->GetOwnerSession() == SelectedSession) && SelectedInstances.Contains(Instance));
}


FSimpleMulticastDelegate& FPFileManager::OnSessionsUpdated()
{
	return SessionsUpdatedDelegate;
}


FSimpleMulticastDelegate& FPFileManager::OnSessionInstanceUpdated()
{
	return SessionInstanceUpdatedDelegate;
}


void FPFileManager::RemoveOwner(const FString& InOwner)
{
	FilteredOwners.Remove(InOwner);
	RefreshSessions();
}


bool FPFileManager::SelectSession(const TSharedPtr<IFileInfo>& Session)
{
	// already selected?
	if (Session == SelectedSession)
	{
		return true;
	}

	// do we own the session?
	if (Session.IsValid() && !Sessions.Contains(Session->GetSessionId()))
	{
		return false;
	}

	// allowed to de/select?
	bool CanSelect = true;
	CanSelectSessionDelegate.Broadcast(Session, CanSelect);

	if (!CanSelect)
	{
		return false;
	}

	// set selection
	SelectedInstances.Empty();
	SelectedSession = Session;
	SelectedSessionChangedEvent.Broadcast(Session);

	return true;
}


bool FPFileManager::SetInstanceSelected(const TSharedRef<IFileInstanceInfo>& Instance, bool Selected)
{
	if (Instance->GetOwnerSession() != SelectedSession)
	{
		return false;
	}

	if (Selected)
	{
		if (!SelectedInstances.Contains(Instance))
		{
			SelectedInstances.Add(Instance);
			InstanceSelectionChangedDelegate.Broadcast(Instance, true);
		}
	}
	else
	{
		if (SelectedInstances.Remove(Instance) > 0)
		{
			InstanceSelectionChangedDelegate.Broadcast(Instance, false);
		}
	}

	return true;
}

bool FPFileManager::SetAddModule(FString& moduleName)
{
	AddModuleDelegate.Broadcast(moduleName);

	return true;
}


/* FSessionManager implementation
 *****************************************************************************/

void FPFileManager::FindExpiredSessions(const FDateTime& Now)
{
	bool Dirty = false;

	//for (TMap<FGuid, TSharedPtr<FFileInfo> >::TIterator It(Sessions); It; ++It)
	//{
	//	if (Now > It.Value()->GetLastUpdateTime() + FTimespan::FromSeconds(10.0))
	//	{
	//		It.RemoveCurrent();
	//		Dirty = true;
	//	}
	//}

	if (Dirty)
	{
		SessionsUpdatedDelegate.Broadcast();
	}
}


bool FPFileManager::IsValidOwner(const FString& Owner)
{
	if (Owner == FPlatformProcess::UserName(false))
	{
		return true;
	}

	for (const auto& FilteredOwner : FilteredOwners)
	{
		if (FilteredOwner == Owner)
		{
			return true;
		}
	}

	return false;
}


void FPFileManager::RefreshSessions()
{
	bool Dirty = false;

	//for (TMap<FGuid, TSharedPtr<FFileInfo> >::TIterator It(Sessions); It; ++It)
	//{
	//	if (!IsValidOwner(It.Value()->GetSessionOwner()))
	//	{
	//		It.RemoveCurrent();
	//		Dirty = true;
	//	}
	//}

	if (Dirty)
	{
		SessionsUpdatedDelegate.Broadcast();
	}
}


void FPFileManager::SendPing()
{
    QUICK_SCOPE_CYCLE_COUNTER(STAT_FSessionmanager_SendPing);

	if (MessageEndpoint.IsValid())
	{
		//MessageEndpoint->Publish(new FEngineServicePing(), EMessageScope::Network);
		//MessageEndpoint->Publish(new FSessionServicePing(FPlatformProcess::UserName(false)), EMessageScope::Network);
	}

	LastPingTime = FDateTime::UtcNow();
}


/* FSessionManager callbacks
 *****************************************************************************/

void FPFileManager::HandleEnginePongMessage(const FEngineServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (!Message.SessionId.IsValid())
	{
		return;
	}

	// update instance
	TSharedPtr<FFileInfo> Session = Sessions.FindRef(Message.SessionId);

	if (Session.IsValid())
	{
		Session->UpdateFromMessage(Message, Context);
		SessionInstanceUpdatedDelegate.Broadcast();
	}
}


void FPFileManager::HandleLogReceived(const TSharedRef<IFileInfo>& Session, const TSharedRef<IFileInstanceInfo>& Instance, const TSharedRef<FFileItemInfo>& Message)
{
	if (Session == SelectedSession)
	{
		LogReceivedEvent.Broadcast(Session, Instance, Message);
	}
}


//void FPFileManager::HandleSessionPongMessage(const FSessionServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
//{
//	if (!Message.SessionId.IsValid())
//	{
//		return;
//	}
//
//	if (!Message.Standalone && !IsValidOwner(Message.SessionOwner))
//	{
//		return;
//	}
//
//	auto MessageBus = MessageBusPtr.Pin();
//
//	if (!MessageBus.IsValid())
//	{
//		return;
//	}
//
//	// update session
//	TSharedPtr<FFileInfo>& Session = Sessions.FindOrAdd(Message.SessionId);
//
//	if (Session.IsValid())
//	{
//		if (Session->GetSessionOwner() != Message.SessionOwner)
//		{
//			Session->UpdateFromMessage(Message, Context);
//			SessionsUpdatedDelegate.Broadcast();
//		}
//		else
//		{
//			Session->UpdateFromMessage(Message, Context);
//		}
//	}
//	else
//	{
//		Session = MakeShareable(new FFileInfo(Message.SessionId, MessageBus.ToSharedRef()));
//		Session->OnLogReceived().AddSP(this, &FPFileManager::HandleLogReceived);
//		Session->UpdateFromMessage(Message, Context);
//
//		SessionsUpdatedDelegate.Broadcast();
//	}
//}
//

bool FPFileManager::HandleTicker(float DeltaTime)
{
    QUICK_SCOPE_CYCLE_COUNTER(STAT_FSessionmanager_HandleTicker);

	FDateTime Now = FDateTime::UtcNow();

	// @todo gmp: don't expire sessions for now
//	FindExpiredSessions(Now);

	if (Now >= LastPingTime + FTimespan::FromSeconds(2.5))
	{
		SendPing();
	}

	return true;
}
