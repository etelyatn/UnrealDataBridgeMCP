// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#pragma once

#include "CoreMinimal.h"
#include "UDBCommandHandler.h"

class FUDBAssetSearchOps
{
public:
	static FUDBCommandResult SearchAssets(const TSharedPtr<FJsonObject>& Params);
};
