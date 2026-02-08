// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "UDBCommandHandler.h"
#include "Operations/UDBDataTableOps.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"

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
		return FUDBDataTableOps::ListDatatables(Params);
	}
	else if (Command == TEXT("get_datatable_schema"))
	{
		return FUDBDataTableOps::GetDatatableSchema(Params);
	}
	else if (Command == TEXT("query_datatable"))
	{
		return FUDBDataTableOps::QueryDatatable(Params);
	}
	else if (Command == TEXT("get_datatable_row"))
	{
		return FUDBDataTableOps::GetDatatableRow(Params);
	}
	else if (Command == TEXT("get_struct_schema"))
	{
		return FUDBDataTableOps::GetStructSchema(Params);
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
