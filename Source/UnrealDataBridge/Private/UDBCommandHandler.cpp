// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "UDBCommandHandler.h"
#include "UDBSerializer.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/DataTable.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogUDBCommandHandler, Log, All);

FUDBCommandResult FUDBCommandHandler::Execute(const FString& Command, const TSharedPtr<FJsonObject>& Params)
{
	if (Command == TEXT("ping"))
	{
		return HandlePing(Params);
	}
	else if (Command == TEXT("get_status"))
	{
		return HandleGetStatus(Params);
	}
	else if (Command == TEXT("list_datatables"))
	{
		return HandleListDatatables(Params);
	}
	else if (Command == TEXT("get_datatable_row"))
	{
		return HandleGetDatatableRow(Params);
	}

	UE_LOG(LogUDBCommandHandler, Warning, TEXT("Unknown command: %s"), *Command);
	return Error(UDBErrorCodes::UnknownCommand, FString::Printf(TEXT("Unknown command: %s"), *Command));
}

FString FUDBCommandHandler::ResultToJson(const FUDBCommandResult& Result, double TimingMs)
{
	TSharedRef<FJsonObject> ResponseJson = MakeShared<FJsonObject>();
	ResponseJson->SetBoolField(TEXT("success"), Result.bSuccess);

	if (Result.bSuccess)
	{
		if (Result.Data.IsValid())
		{
			ResponseJson->SetObjectField(TEXT("data"), Result.Data);
		}

		if (Result.Warnings.Num() > 0)
		{
			TArray<TSharedPtr<FJsonValue>> WarningsArray;
			for (const FString& Warning : Result.Warnings)
			{
				WarningsArray.Add(MakeShared<FJsonValueString>(Warning));
			}
			ResponseJson->SetArrayField(TEXT("warnings"), WarningsArray);
		}
	}
	else
	{
		TSharedRef<FJsonObject> ErrorObj = MakeShared<FJsonObject>();
		ErrorObj->SetStringField(TEXT("code"), Result.ErrorCode);
		ErrorObj->SetStringField(TEXT("message"), Result.ErrorMessage);

		if (Result.ErrorDetails.IsValid())
		{
			ErrorObj->SetObjectField(TEXT("details"), Result.ErrorDetails);
		}

		ResponseJson->SetObjectField(TEXT("error"), ErrorObj);
	}

	ResponseJson->SetNumberField(TEXT("timing_ms"), TimingMs);

	FString OutputString;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
	FJsonSerializer::Serialize(ResponseJson, Writer);

	return OutputString;
}

FUDBCommandResult FUDBCommandHandler::Success(TSharedPtr<FJsonObject> Data)
{
	FUDBCommandResult Result;
	Result.bSuccess = true;
	Result.Data = MoveTemp(Data);
	return Result;
}

FUDBCommandResult FUDBCommandHandler::Error(const FString& Code, const FString& Message, TSharedPtr<FJsonObject> Details)
{
	FUDBCommandResult Result;
	Result.bSuccess = false;
	Result.ErrorCode = Code;
	Result.ErrorMessage = Message;
	Result.ErrorDetails = MoveTemp(Details);
	return Result;
}

FUDBCommandResult FUDBCommandHandler::HandlePing(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("message"), TEXT("pong"));
	return Success(Data);
}

FUDBCommandResult FUDBCommandHandler::HandleGetStatus(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("status"), TEXT("ready"));
	Data->SetStringField(TEXT("editor"), TEXT("UnrealEditor"));
	return Success(Data);
}

FUDBCommandResult FUDBCommandHandler::HandleListDatatables(const TSharedPtr<FJsonObject>& Params)
{
	TArray<TSharedPtr<FJsonValue>> DatatablesArray;

	for (TObjectIterator<UDataTable> It; It; ++It)
	{
		UDataTable* DataTable = *It;
		if (DataTable == nullptr)
		{
			continue;
		}

		TSharedRef<FJsonObject> EntryJson = MakeShared<FJsonObject>();
		EntryJson->SetStringField(TEXT("name"), DataTable->GetName());
		EntryJson->SetStringField(TEXT("path"), DataTable->GetPathName());

		if (const UScriptStruct* RowStruct = DataTable->GetRowStruct())
		{
			EntryJson->SetStringField(TEXT("row_struct"), RowStruct->GetName());
		}
		else
		{
			EntryJson->SetStringField(TEXT("row_struct"), TEXT("None"));
		}

		EntryJson->SetNumberField(TEXT("row_count"), DataTable->GetRowMap().Num());

		DatatablesArray.Add(MakeShared<FJsonValueObject>(EntryJson));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetArrayField(TEXT("datatables"), DatatablesArray);

	return Success(Data);
}

FUDBCommandResult FUDBCommandHandler::HandleGetDatatableRow(const TSharedPtr<FJsonObject>& Params)
{
	FString TablePath;
	FString RowName;

	bool bHasParams = Params.IsValid()
		&& Params->TryGetStringField(TEXT("table_path"), TablePath)
		&& Params->TryGetStringField(TEXT("row_name"), RowName);

	if (!bHasParams)
	{
		return Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required params: table_path and row_name")
		);
	}

	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *TablePath);
	if (DataTable == nullptr)
	{
		return Error(
			UDBErrorCodes::TableNotFound,
			FString::Printf(TEXT("DataTable not found: %s"), *TablePath)
		);
	}

	const void* RowData = DataTable->FindRowUnchecked(FName(*RowName));
	if (RowData == nullptr)
	{
		return Error(
			UDBErrorCodes::RowNotFound,
			FString::Printf(TEXT("Row not found: %s"), *RowName)
		);
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("table_path"), TablePath);
	Data->SetStringField(TEXT("row_name"), RowName);
	Data->SetStringField(TEXT("row_struct"), DataTable->GetRowStruct()->GetName());
	Data->SetObjectField(TEXT("row_data"), FUDBSerializer::StructToJson(DataTable->GetRowStruct(), RowData));

	return Success(Data);
}
