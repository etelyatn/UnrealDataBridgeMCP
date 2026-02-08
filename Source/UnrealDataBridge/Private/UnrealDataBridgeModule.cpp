// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "UnrealDataBridgeModule.h"

#define LOCTEXT_NAMESPACE "FUnrealDataBridgeModule"

DEFINE_LOG_CATEGORY_STATIC(LogUnrealDataBridge, Log, All);

void FUnrealDataBridgeModule::StartupModule()
{
	UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge module starting up"));
}

void FUnrealDataBridgeModule::ShutdownModule()
{
	UE_LOG(LogUnrealDataBridge, Log, TEXT("UnrealDataBridge module shutting down"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FUnrealDataBridgeModule, UnrealDataBridge)
