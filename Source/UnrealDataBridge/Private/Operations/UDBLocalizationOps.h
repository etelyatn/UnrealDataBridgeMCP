// Copyright Eugene Telyatnik. All Rights Reserved. https://github.com/etelyatn/UnrealDataBridgeMCP

#pragma once

#include "CoreMinimal.h"
#include "UDBCommandHandler.h"

class UStringTable;

class FUDBLocalizationOps
{
public:
	static FUDBCommandResult ListStringTables(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult GetTranslations(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult SetTranslation(const TSharedPtr<FJsonObject>& Params);

private:
	/** Load a StringTable by asset path, returns nullptr and sets OutError if not found */
	static UStringTable* LoadStringTable(const FString& TablePath, FUDBCommandResult& OutError);
};
