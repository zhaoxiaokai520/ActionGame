// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "Models/FileInstanceInfo.h"

#include "EngineServiceMessages.h"
#include "HAL/PlatformProcess.h"
#include "MessageEndpointBuilder.h"
//#include "SessionServiceMessages.h"

#include "FileItemInfo.h"


/* FSessionInstanceInfo structors
 *****************************************************************************/

FFileInstanceInfo::FFileInstanceInfo(const FGuid& InInstanceId, const TSharedRef<IFileInfo>& InOwner, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InMessageBus)
	: Authorized(false)
	, EngineVersion(0)
	, InstanceId(InInstanceId)
	, Owner(InOwner)
{
	//MessageEndpoint = FMessageEndpoint::Builder("FSessionInstanceInfo", InMessageBus)
	//	.Handling<FSessionServiceLog>(this, &FSessionInstanceInfo::HandleSessionLogMessage);
}


/* FSessionInstanceInfo interface
 *****************************************************************************/
void FFileInstanceInfo::UpdateFromMessage(const FEngineServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (Message.InstanceId != InstanceId)
	{
		return;
	}

	CurrentLevel = Message.CurrentLevel;
	EngineAddress = Context->GetSender();
	EngineVersion = Message.EngineVersion;
	HasBegunPlay = Message.HasBegunPlay;
	WorldTimeSeconds = Message.WorldTimeSeconds;
	InstanceType = Message.InstanceType;
}


//void FFileInstanceInfo::UpdateFromMessage(const FSessionServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
//{
//	if (Message.InstanceId != InstanceId)
//	{
//		return;
//	}
//
//	if (MessageEndpoint.IsValid() && (ApplicationAddress != Context->GetSender()))
//	{
//		MessageEndpoint->Send(new FSessionServiceLogSubscribe(), Context->GetSender());
//	}
//
//	Authorized = Message.Authorized;
//	ApplicationAddress = Context->GetSender();
//	BuildDate = Message.BuildDate;
//	DeviceName = Message.DeviceName;
//	InstanceName = Message.InstanceName;
//	IsConsoleBuild = Message.IsConsoleBuild;
//	PlatformName = Message.PlatformName;
//
//	LastUpdateTime = FDateTime::UtcNow();
//}


/* FSessionInstanceInfo interface
 *****************************************************************************/

void FFileInstanceInfo::ExecuteCommand(const FString& CommandString)
{
	if (MessageEndpoint.IsValid() && EngineAddress.IsValid())
	{
		//MessageEndpoint->Send(new FEngineServiceExecuteCommand(CommandString, FPlatformProcess::UserName(false)), EngineAddress);
	}
}


void FFileInstanceInfo::Terminate()
{
	if (MessageEndpoint.IsValid() && EngineAddress.IsValid())
	{
		//MessageEndpoint->Send(new FEngineServiceTerminate(FPlatformProcess::UserName(false)), EngineAddress);
	}
}


/* FSessionInstanceInfo callbacks
 *****************************************************************************/

//void FFileInstanceInfo::HandleSessionLogMessage(const FSessionServiceLog& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
//{
//	TSharedRef<FSessionLogMessage> LogMessage = MakeShareable(
//		new FSessionLogMessage(
//			InstanceId,
//			InstanceName,
//			Message.TimeSeconds,
//			Message.Data,
//			(ELogVerbosity::Type)Message.Verbosity,
//			Message.Category
//		)
//	);
//
//	LogMessages.Add(LogMessage);
//	LogReceivedEvent.Broadcast(AsShared(), LogMessage);
//}
