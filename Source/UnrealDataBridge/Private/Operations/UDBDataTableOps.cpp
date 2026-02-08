// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "Operations/UDBDataTableOps.h"
#include "UDBSerializer.h"
#include "Engine/DataTable.h"
#include "UObject/UObjectIterator.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

DEFINE_LOG_CATEGORY_STATIC(LogUDBDataTableOps, Log, All);

UDataTable* FUDBDataTableOps::LoadDataTable(const FString& TablePath, FUDBCommandResult& OutError)
{
	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, *TablePath);
	if (DataTable == nullptr)
	{
		OutError = FUDBCommandHandler::Error(
			UDBErrorCodes::TableNotFound,
			FString::Printf(TEXT("DataTable not found: %s"), *TablePath)
		);
	}
	return DataTable;
}

FUDBCommandResult FUDBDataTableOps::ListDatatables(const TSharedPtr<FJsonObject>& Params)
{
	FString PathFilter;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("path_filter"), PathFilter);
	}

	TArray<TSharedPtr<FJsonValue>> DatatablesArray;

	for (TObjectIterator<UDataTable> It; It; ++It)
	{
		UDataTable* DataTable = *It;
		if (DataTable == nullptr)
		{
			continue;
		}

		FString AssetPath = DataTable->GetPathName();

		// Apply path filter (prefix match)
		if (!PathFilter.IsEmpty() && !AssetPath.StartsWith(PathFilter))
		{
			continue;
		}

		TSharedRef<FJsonObject> EntryJson = MakeShared<FJsonObject>();
		EntryJson->SetStringField(TEXT("name"), DataTable->GetName());
		EntryJson->SetStringField(TEXT("path"), AssetPath);

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

	return FUDBCommandHandler::Success(Data);
}

FUDBCommandResult FUDBDataTableOps::GetDatatableSchema(const TSharedPtr<FJsonObject>& Params)
{
	FString TablePath;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("table_path"), TablePath))
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required param: table_path")
		);
	}

	FUDBCommandResult LoadError;
	UDataTable* DataTable = LoadDataTable(TablePath, LoadError);
	if (DataTable == nullptr)
	{
		return LoadError;
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (RowStruct == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidStructType,
			FString::Printf(TEXT("DataTable has no row struct: %s"), *TablePath)
		);
	}

	bool bIncludeInherited = true;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("include_inherited"), bIncludeInherited);
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("table_path"), TablePath);
	Data->SetStringField(TEXT("row_struct_name"), RowStruct->GetName());
	Data->SetObjectField(TEXT("schema"), FUDBSerializer::GetStructSchema(RowStruct, bIncludeInherited));

	return FUDBCommandHandler::Success(Data);
}

FUDBCommandResult FUDBDataTableOps::QueryDatatable(const TSharedPtr<FJsonObject>& Params)
{
	FString TablePath;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("table_path"), TablePath))
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required param: table_path")
		);
	}

	FUDBCommandResult LoadError;
	UDataTable* DataTable = LoadDataTable(TablePath, LoadError);
	if (DataTable == nullptr)
	{
		return LoadError;
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (RowStruct == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidStructType,
			FString::Printf(TEXT("DataTable has no row struct: %s"), *TablePath)
		);
	}

	// Parse optional params
	FString RowNamePattern;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("row_name_pattern"), RowNamePattern);
	}

	int32 Offset = 0;
	int32 Limit = 100;
	if (Params.IsValid())
	{
		// TryGetNumberField returns double; cast to int32
		double OffsetVal = 0.0;
		if (Params->TryGetNumberField(TEXT("offset"), OffsetVal))
		{
			Offset = FMath::Max(0, static_cast<int32>(OffsetVal));
		}
		double LimitVal = 0.0;
		if (Params->TryGetNumberField(TEXT("limit"), LimitVal))
		{
			Limit = FMath::Max(1, static_cast<int32>(LimitVal));
		}
	}

	// Parse optional fields projection
	TArray<FString> FieldsProjection;
	if (Params.IsValid())
	{
		const TArray<TSharedPtr<FJsonValue>>* FieldsArray = nullptr;
		if (Params->TryGetArrayField(TEXT("fields"), FieldsArray) && FieldsArray != nullptr)
		{
			for (const TSharedPtr<FJsonValue>& FieldValue : *FieldsArray)
			{
				FString FieldName;
				if (FieldValue.IsValid() && FieldValue->TryGetString(FieldName))
				{
					FieldsProjection.Add(FieldName);
				}
			}
		}
	}

	// Get all row names and apply pattern filter
	TArray<FName> RowNames = DataTable->GetRowNames();

	TArray<FName> FilteredRowNames;
	for (const FName& Name : RowNames)
	{
		if (!RowNamePattern.IsEmpty())
		{
			if (!Name.ToString().MatchesWildcard(RowNamePattern))
			{
				continue;
			}
		}
		FilteredRowNames.Add(Name);
	}

	const int32 TotalCount = FilteredRowNames.Num();

	// Apply pagination
	const int32 StartIndex = FMath::Min(Offset, TotalCount);
	const int32 EndIndex = FMath::Min(StartIndex + Limit, TotalCount);

	TArray<TSharedPtr<FJsonValue>> RowsArray;
	for (int32 Index = StartIndex; Index < EndIndex; ++Index)
	{
		const FName& RowName = FilteredRowNames[Index];
		const void* RowData = DataTable->FindRowUnchecked(RowName);
		if (RowData == nullptr)
		{
			continue;
		}

		TSharedPtr<FJsonObject> RowJson = FUDBSerializer::StructToJson(RowStruct, RowData);

		// Apply fields projection
		if (FieldsProjection.Num() > 0 && RowJson.IsValid())
		{
			TSharedPtr<FJsonObject> ProjectedJson = MakeShared<FJsonObject>();
			for (const FString& FieldName : FieldsProjection)
			{
				if (RowJson->HasField(FieldName))
				{
					ProjectedJson->SetField(FieldName, RowJson->TryGetField(FieldName));
				}
			}
			RowJson = ProjectedJson;
		}

		TSharedRef<FJsonObject> EntryJson = MakeShared<FJsonObject>();
		EntryJson->SetStringField(TEXT("row_name"), RowName.ToString());
		EntryJson->SetObjectField(TEXT("row_data"), RowJson);

		RowsArray.Add(MakeShared<FJsonValueObject>(EntryJson));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("table_path"), TablePath);
	Data->SetArrayField(TEXT("rows"), RowsArray);
	Data->SetNumberField(TEXT("total_count"), TotalCount);
	Data->SetNumberField(TEXT("offset"), Offset);
	Data->SetNumberField(TEXT("limit"), Limit);

	return FUDBCommandHandler::Success(Data);
}

FUDBCommandResult FUDBDataTableOps::GetDatatableRow(const TSharedPtr<FJsonObject>& Params)
{
	FString TablePath;
	FString RowName;

	bool bHasParams = Params.IsValid()
		&& Params->TryGetStringField(TEXT("table_path"), TablePath)
		&& Params->TryGetStringField(TEXT("row_name"), RowName);

	if (!bHasParams)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required params: table_path and row_name")
		);
	}

	FUDBCommandResult LoadError;
	UDataTable* DataTable = LoadDataTable(TablePath, LoadError);
	if (DataTable == nullptr)
	{
		return LoadError;
	}

	const void* RowData = DataTable->FindRowUnchecked(FName(*RowName));
	if (RowData == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::RowNotFound,
			FString::Printf(TEXT("Row not found: %s"), *RowName)
		);
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("table_path"), TablePath);
	Data->SetStringField(TEXT("row_name"), RowName);
	Data->SetStringField(TEXT("row_struct"), DataTable->GetRowStruct()->GetName());
	Data->SetObjectField(TEXT("row_data"), FUDBSerializer::StructToJson(DataTable->GetRowStruct(), RowData));

	return FUDBCommandHandler::Success(Data);
}

FUDBCommandResult FUDBDataTableOps::GetStructSchema(const TSharedPtr<FJsonObject>& Params)
{
	FString StructName;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("struct_name"), StructName))
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required param: struct_name")
		);
	}

	// Build list of names to search for (original + F-prefix toggled)
	TArray<FString> NamesToSearch;
	NamesToSearch.Add(StructName);
	if (StructName.StartsWith(TEXT("F")))
	{
		NamesToSearch.Add(StructName.RightChop(1));
	}
	else
	{
		NamesToSearch.Add(TEXT("F") + StructName);
	}

	// Search all loaded UScriptStructs by name
	UScriptStruct* FoundStruct = nullptr;
	for (TObjectIterator<UScriptStruct> It; It; ++It)
	{
		const FString& ItName = It->GetName();
		for (const FString& SearchName : NamesToSearch)
		{
			if (ItName == SearchName)
			{
				FoundStruct = *It;
				break;
			}
		}
		if (FoundStruct != nullptr)
		{
			break;
		}
	}

	if (FoundStruct == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidStructType,
			FString::Printf(TEXT("Struct not found: %s"), *StructName)
		);
	}

	bool bIncludeSubtypes = false;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("include_subtypes"), bIncludeSubtypes);
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetObjectField(TEXT("schema"), FUDBSerializer::GetStructSchema(FoundStruct));

	if (bIncludeSubtypes)
	{
		TArray<UScriptStruct*> Subtypes = FUDBSerializer::FindInstancedStructSubtypes(FoundStruct);
		TArray<TSharedPtr<FJsonValue>> SubtypeSchemas;
		for (const UScriptStruct* Subtype : Subtypes)
		{
			SubtypeSchemas.Add(MakeShared<FJsonValueObject>(FUDBSerializer::GetStructSchema(Subtype)));
		}
		Data->SetArrayField(TEXT("subtypes"), SubtypeSchemas);
	}

	return FUDBCommandHandler::Success(Data);
}
