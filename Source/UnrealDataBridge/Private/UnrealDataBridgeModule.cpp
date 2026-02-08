// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "UnrealDataBridgeModule.h"
#include "UDBTcpServer.h"

#define LOCTEXT_NAMESPACE "FUnrealDataBridgeModule"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealDataBridge, Log, All);

void FUnrealDataBridgeModule::StartupModule()
{
	UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge module starting up"));

	TcpServer = MakeUnique<FUDBTcpServer>();
	if (TcpServer->Start(8742))
	{
		UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge TCP server listening on 127.0.0.1:8742"));
	}
	else
	{
		UE_LOG(LogUnrealDataBridge, Error, TEXT("Failed to start UnrealDataBridge TCP server"));
	}
}

void FUnrealDataBridgeModule::ShutdownModule()
{
	UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge module shutting down"));

	if (TcpServer.IsValid())
	{
		TcpServer->Stop();
		TcpServer.Reset();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealDataBridgeModule, UnrealDataBridge)
