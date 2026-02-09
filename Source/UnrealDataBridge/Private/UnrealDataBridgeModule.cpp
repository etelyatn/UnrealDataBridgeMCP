
#include "UnrealDataBridgeModule.h"
#include "UDBSettings.h"
#include "UDBTcpServer.h"

#define LOCTEXT_NAMESPACE "FUnrealDataBridgeModule"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealDataBridge, Log, All);

void FUnrealDataBridgeModule::StartupModule()
{
	UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge module starting up"));

	const UUDBSettings* Settings = UUDBSettings::Get();
	if (!Settings->bAutoStart)
	{
		UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge auto-start disabled in settings"));
		return;
	}

	const int32 Port = Settings->Port;
	TcpServer = MakeUnique<FUDBTcpServer>();
	if (TcpServer->Start(Port))
	{
		UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge TCP server listening on 127.0.0.1:%d"), Port);
	}
	else
	{
		UE_LOG(LogUnrealDataBridge, Error, TEXT("Failed to start UnrealDataBridge TCP server on port %d"), Port);
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
