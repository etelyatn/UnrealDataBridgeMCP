// Copyright Eugene Telyatnik. All Rights Reserved. https://github.com/etelyatn/UnrealDataBridgeMCP

#pragma once

#include "CoreMinimal.h"
#include "UDBCommandHandler.h"

class FUDBAssetSearchOps
{
public:
	static FUDBCommandResult SearchAssets(const TSharedPtr<FJsonObject>& Params);
};
