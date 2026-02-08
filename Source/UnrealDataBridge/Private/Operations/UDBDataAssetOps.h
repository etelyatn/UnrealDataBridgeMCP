// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#pragma once

#include "CoreMinimal.h"
#include "UDBCommandHandler.h"

class UDataAsset;

class FUDBDataAssetOps
{
public:
	static FUDBCommandResult ListDataAssets(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult GetDataAsset(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult UpdateDataAsset(const TSharedPtr<FJsonObject>& Params);

private:
	/** Load a DataAsset by asset path, returns nullptr and sets OutError if not found */
	static UDataAsset* LoadDataAsset(const FString& AssetPath, FUDBCommandResult& OutError);
};
