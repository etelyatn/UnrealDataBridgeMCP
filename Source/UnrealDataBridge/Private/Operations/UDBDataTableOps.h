// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#pragma once

#include "CoreMinimal.h"
#include "UDBCommandHandler.h"

class UDataTable;

class FUDBDataTableOps
{
public:
	static FUDBCommandResult ListDatatables(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult GetDatatableSchema(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult QueryDatatable(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult GetDatatableRow(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult GetStructSchema(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult AddDatatableRow(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult UpdateDatatableRow(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult DeleteDatatableRow(const TSharedPtr<FJsonObject>& Params);
	static FUDBCommandResult ImportDatatableJson(const TSharedPtr<FJsonObject>& Params);

private:
	/** Load a DataTable by asset path, returns nullptr and sets OutError if not found */
	static UDataTable* LoadDataTable(const FString& TablePath, FUDBCommandResult& OutError);
};
