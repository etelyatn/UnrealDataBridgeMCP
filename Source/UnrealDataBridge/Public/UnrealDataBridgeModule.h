// Copyright Eugene Telyatnik. All Rights Reserved. https://github.com/etelyatn/UnrealDataBridgeMCP

#pragma once

#include "Modules/ModuleManager.h"

class FUDBTcpServer;

class FUnrealDataBridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	TUniquePtr<FUDBTcpServer> TcpServer;
};
