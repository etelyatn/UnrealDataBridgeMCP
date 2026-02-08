// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#pragma once

#include "Modules/ModuleManager.h"

class FUnrealDataBridgeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
