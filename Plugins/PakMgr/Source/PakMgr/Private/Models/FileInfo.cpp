// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "FileInfo.h"
#include "EngineServiceMessages.h"
//#include "SessionServiceMessages.h"
#include "Models/FileInstanceInfo.h"


/* FSessionInfo structors
 *****************************************************************************/

FFileInfo::FFileInfo(const FGuid& InSessionId, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InMessageBus)
	: MessageBusPtr(InMessageBus)
	, SessionId(InSessionId)
{ }


/* FSessionInfo interface
 *****************************************************************************/

void FFileInfo::UpdateFromMessage(const FEngineServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (Message.SessionId != SessionId)
	{
		return;
	}

	// update instance
	// @todo gmp: reconsider merging EngineService and SessionService
	/*TSharedPtr<FSessionInstanceInfo> Instance = Instances.FindRef(Context->GetSender());

	if (Instance.IsValid())
	{
		Instance->UpdateFromMessage(Message, Context);
	}*/
	for (TMap<FMessageAddress, TSharedPtr<FFileInstanceInfo> >::TIterator It(Instances); It; ++It)
	{
		if (It.Value()->GetInstanceId() == Message.InstanceId)
		{
			It.Value()->UpdateFromMessage(Message, Context);
			break;
		}
	}
}


//void FSessionInfo::UpdateFromMessage(const FSessionServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
//{
//	if (Message.SessionId != SessionId)
//	{
//		return;
//	}
//
//	// update session info
//	Standalone = Message.Standalone;
//	SessionOwner = Message.SessionOwner;
//	SessionName = Message.SessionName;
//
//	// update instance
//	TSharedPtr<FFileInstanceInfo>& Instance = Instances.FindOrAdd(Context->GetSender());
//
//	if (Instance.IsValid())
//	{
//		Instance->UpdateFromMessage(Message, Context);
//	}
//	else
//	{
//		auto MessageBus = MessageBusPtr.Pin();
//
//		if (MessageBus.IsValid())
//		{
//			Instance = MakeShareable(new FFileInstanceInfo(Message.InstanceId, AsShared(), MessageBus.ToSharedRef()));
//			Instance->OnLogReceived().AddSP(this, &FSessionInfo::HandleLogReceived);
//			Instance->UpdateFromMessage(Message, Context);
//
//			InstanceDiscoveredEvent.Broadcast(AsShared(), Instance.ToSharedRef());
//		}
//	}
//
//	LastUpdateTime = FDateTime::UtcNow();
//}


/* ISessionInfo interface
 *****************************************************************************/

void FFileInfo::GetInstances(TArray<TSharedPtr<IFileInstanceInfo>>& OutInstances) const
{
	OutInstances.Empty();

	for (TMap<FMessageAddress, TSharedPtr<FFileInstanceInfo> >::TConstIterator It(Instances); It; ++It)
	{
		OutInstances.Add(It.Value());
	}
}


const FDateTime& FFileInfo::GetLastUpdateTime() const
{
	return LastUpdateTime;
}


const int32 FFileInfo::GetNumInstances() const
{
	return Instances.Num();
}


const FGuid& FFileInfo::GetSessionId() const
{
	return SessionId;
}


const FString& FFileInfo::GetSessionName() const
{
	return SessionName;
}


const FString& FFileInfo::GetSessionOwner() const
{
	return SessionOwner;
}


const bool FFileInfo::IsStandalone() const
{
	return Standalone;
}


void FFileInfo::Terminate()
{
	for (TMap<FMessageAddress, TSharedPtr<FFileInstanceInfo> >::TIterator It(Instances); It; ++It)
	{
		It.Value()->Terminate();
	}
}


/* FSessionInfo callbacks
 *****************************************************************************/

void FFileInfo::HandleLogReceived(const TSharedRef<IFileInstanceInfo>& Instance, const TSharedRef<FFileItemInfo>& itemInfo)
{
	LogReceivedEvent.Broadcast(AsShared(), Instance, itemInfo);
}
