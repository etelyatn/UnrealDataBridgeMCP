// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "Operations/UDBDataTableOps.h"
#include "UDBSerializer.h"
#include "Engine/DataTable.h"
#include "Engine/CompositeDataTable.h"
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

TArray<UDataTable*> FUDBDataTableOps::GetParentTables(const UCompositeDataTable* CompositeTable)
{
	TArray<UDataTable*> Result;
	if (CompositeTable == nullptr)
	{
		return Result;
	}

	const FArrayProperty* ParentTablesProp = CastField<FArrayProperty>(
		UCompositeDataTable::StaticClass()->FindPropertyByName(TEXT("ParentTables"))
	);
	if (ParentTablesProp == nullptr)
	{
		UE_LOG(LogUDBDataTableOps, Warning, TEXT("Could not find ParentTables property on UCompositeDataTable"));
		return Result;
	}

	FScriptArrayHelper ArrayHelper(ParentTablesProp, ParentTablesProp->ContainerPtrToValuePtr<void>(CompositeTable));
	const FObjectProperty* InnerProp = CastField<FObjectProperty>(ParentTablesProp->Inner);
	if (InnerProp == nullptr)
	{
		return Result;
	}

	for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
	{
		UObject* Obj = InnerProp->GetObjectPropertyValue(ArrayHelper.GetRawPtr(Index));
		UDataTable* Table = Cast<UDataTable>(Obj);
		if (Table != nullptr)
		{
			Result.Add(Table);
		}
	}

	return Result;
}

TArray<TSharedPtr<FJsonValue>> FUDBDataTableOps::GetParentTablesJsonArray(const UCompositeDataTable* CompositeTable)
{
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TArray<UDataTable*> Parents = GetParentTables(CompositeTable);

	for (const UDataTable* Parent : Parents)
	{
		TSharedRef<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), Parent->GetName());
		Entry->SetStringField(TEXT("path"), Parent->GetPathName());
		JsonArray.Add(MakeShared<FJsonValueObject>(Entry));
	}

	return JsonArray;
}

UDataTable* FUDBDataTableOps::FindSourceTableForRow(const UCompositeDataTable* CompositeTable, FName RowName)
{
	TArray<UDataTable*> Parents = GetParentTables(CompositeTable);

	// Search back-to-front: higher-index tables override lower-index ones
	for (int32 Index = Parents.Num() - 1; Index >= 0; --Index)
	{
		UDataTable* Parent = Parents[Index];
		if (Parent == nullptr)
		{
			continue;
		}

		// If parent is itself a composite, recurse
		const UCompositeDataTable* NestedComposite = Cast<UCompositeDataTable>(Parent);
		if (NestedComposite != nullptr)
		{
			UDataTable* NestedResult = FindSourceTableForRow(NestedComposite, RowName);
			if (NestedResult != nullptr)
			{
				return NestedResult;
			}
			continue;
		}

		if (Parent->FindRowUnchecked(RowName) != nullptr)
		{
			return Parent;
		}
	}

	return nullptr;
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

		const UCompositeDataTable* CompositeTable = Cast<UCompositeDataTable>(DataTable);
		EntryJson->SetBoolField(TEXT("is_composite"), CompositeTable != nullptr);
		if (CompositeTable != nullptr)
		{
			EntryJson->SetArrayField(TEXT("parent_tables"), GetParentTablesJsonArray(CompositeTable));
		}

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

FUDBCommandResult FUDBDataTableOps::AddDatatableRow(const TSharedPtr<FJsonObject>& Params)
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

	const TSharedPtr<FJsonObject>* RowData = nullptr;
	if (!Params->TryGetObjectField(TEXT("row_data"), RowData) || RowData == nullptr || !(*RowData).IsValid())
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required param: row_data")
		);
	}

	FUDBCommandResult LoadError;
	UDataTable* DataTable = LoadDataTable(TablePath, LoadError);
	if (DataTable == nullptr)
	{
		return LoadError;
	}

	// Block writes to composite DataTables
	const UCompositeDataTable* CompositeTable = Cast<UCompositeDataTable>(DataTable);
	if (CompositeTable != nullptr)
	{
		TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
		Details->SetArrayField(TEXT("parent_tables"), GetParentTablesJsonArray(CompositeTable));
		return FUDBCommandHandler::Error(
			UDBErrorCodes::CompositeWriteBlocked,
			FString::Printf(TEXT("Cannot add rows to CompositeDataTable '%s'. Add to one of its source tables instead."), *DataTable->GetName()),
			Details
		);
	}

	const FName RowFName(*RowName);
	if (DataTable->FindRowUnchecked(RowFName) != nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::RowAlreadyExists,
			FString::Printf(TEXT("Row already exists: %s"), *RowName)
		);
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (RowStruct == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidStructType,
			FString::Printf(TEXT("DataTable has no row struct: %s"), *TablePath)
		);
	}

	uint8* RowMemory = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
	RowStruct->InitializeStruct(RowMemory);

	TArray<FString> Warnings;
	bool bDeserializeSuccess = FUDBSerializer::JsonToStruct(*RowData, RowStruct, RowMemory, Warnings);

	if (!bDeserializeSuccess)
	{
		RowStruct->DestroyStruct(RowMemory);
		FMemory::Free(RowMemory);
		return FUDBCommandHandler::Error(
			UDBErrorCodes::SerializationError,
			TEXT("Failed to deserialize row_data into row struct")
		);
	}

	DataTable->AddRow(RowFName, RowMemory, RowStruct);

	RowStruct->DestroyStruct(RowMemory);
	FMemory::Free(RowMemory);

	DataTable->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("row_name"), RowName);

	if (Warnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarningsArray;
		for (const FString& Warning : Warnings)
		{
			WarningsArray.Add(MakeShared<FJsonValueString>(Warning));
		}
		Data->SetArrayField(TEXT("warnings"), WarningsArray);
	}

	FUDBCommandResult Result = FUDBCommandHandler::Success(Data);
	Result.Warnings = MoveTemp(Warnings);
	return Result;
}

FUDBCommandResult FUDBDataTableOps::UpdateDatatableRow(const TSharedPtr<FJsonObject>& Params)
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

	const TSharedPtr<FJsonObject>* RowData = nullptr;
	if (!Params->TryGetObjectField(TEXT("row_data"), RowData) || RowData == nullptr || !(*RowData).IsValid())
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required param: row_data")
		);
	}

	FUDBCommandResult LoadError;
	UDataTable* DataTable = LoadDataTable(TablePath, LoadError);
	if (DataTable == nullptr)
	{
		return LoadError;
	}

	const FName RowFName(*RowName);

	// Auto-resolve composite to source table
	FString CompositeTablePath;
	const UCompositeDataTable* CompositeTable = Cast<UCompositeDataTable>(DataTable);
	if (CompositeTable != nullptr)
	{
		UDataTable* SourceTable = FindSourceTableForRow(CompositeTable, RowFName);
		if (SourceTable == nullptr)
		{
			return FUDBCommandHandler::Error(
				UDBErrorCodes::RowNotFound,
				FString::Printf(TEXT("Row '%s' not found in any source table of composite '%s'"), *RowName, *DataTable->GetName())
			);
		}
		CompositeTablePath = TablePath;
		DataTable = SourceTable;
		UE_LOG(LogUDBDataTableOps, Log, TEXT("Auto-resolved composite '%s' -> source table '%s' for row '%s'"),
			*CompositeTablePath, *DataTable->GetPathName(), *RowName);
	}

	uint8* RowPtr = DataTable->FindRowUnchecked(RowFName);
	if (RowPtr == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::RowNotFound,
			FString::Printf(TEXT("Row not found: %s"), *RowName)
		);
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (RowStruct == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidStructType,
			FString::Printf(TEXT("DataTable has no row struct: %s"), *TablePath)
		);
	}

	// Track which fields were modified
	TArray<FString> ModifiedFields;
	for (const auto& Pair : (*RowData)->Values)
	{
		ModifiedFields.Add(Pair.Key);
	}

	TArray<FString> Warnings;
	bool bDeserializeSuccess = FUDBSerializer::JsonToStruct(*RowData, RowStruct, RowPtr, Warnings);

	if (!bDeserializeSuccess)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::SerializationError,
			TEXT("Failed to deserialize row_data into existing row")
		);
	}

	DataTable->HandleDataTableChanged(RowFName);
	DataTable->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("row_name"), RowName);

	if (!CompositeTablePath.IsEmpty())
	{
		Data->SetStringField(TEXT("source_table_path"), DataTable->GetPathName());
		Data->SetStringField(TEXT("composite_table_path"), CompositeTablePath);
	}

	TArray<TSharedPtr<FJsonValue>> ModifiedFieldsArray;
	for (const FString& Field : ModifiedFields)
	{
		ModifiedFieldsArray.Add(MakeShared<FJsonValueString>(Field));
	}
	Data->SetArrayField(TEXT("modified_fields"), ModifiedFieldsArray);

	if (Warnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarningsArray;
		for (const FString& Warning : Warnings)
		{
			WarningsArray.Add(MakeShared<FJsonValueString>(Warning));
		}
		Data->SetArrayField(TEXT("warnings"), WarningsArray);
	}

	FUDBCommandResult Result = FUDBCommandHandler::Success(Data);
	Result.Warnings = MoveTemp(Warnings);
	return Result;
}

FUDBCommandResult FUDBDataTableOps::DeleteDatatableRow(const TSharedPtr<FJsonObject>& Params)
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

	const FName RowFName(*RowName);

	// Auto-resolve composite to source table
	FString CompositeTablePath;
	const UCompositeDataTable* CompositeTable = Cast<UCompositeDataTable>(DataTable);
	if (CompositeTable != nullptr)
	{
		UDataTable* SourceTable = FindSourceTableForRow(CompositeTable, RowFName);
		if (SourceTable == nullptr)
		{
			return FUDBCommandHandler::Error(
				UDBErrorCodes::RowNotFound,
				FString::Printf(TEXT("Row '%s' not found in any source table of composite '%s'"), *RowName, *DataTable->GetName())
			);
		}
		CompositeTablePath = TablePath;
		DataTable = SourceTable;
		UE_LOG(LogUDBDataTableOps, Log, TEXT("Auto-resolved composite '%s' -> source table '%s' for row '%s'"),
			*CompositeTablePath, *DataTable->GetPathName(), *RowName);
	}

	if (DataTable->FindRowUnchecked(RowFName) == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::RowNotFound,
			FString::Printf(TEXT("Row not found: %s"), *RowName)
		);
	}

	DataTable->RemoveRow(RowFName);
	DataTable->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("row_name"), RowName);

	if (!CompositeTablePath.IsEmpty())
	{
		Data->SetStringField(TEXT("source_table_path"), DataTable->GetPathName());
		Data->SetStringField(TEXT("composite_table_path"), CompositeTablePath);
	}

	return FUDBCommandHandler::Success(Data);
}

FUDBCommandResult FUDBDataTableOps::ImportDatatableJson(const TSharedPtr<FJsonObject>& Params)
{
	FString TablePath;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("table_path"), TablePath))
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required param: table_path")
		);
	}

	const TArray<TSharedPtr<FJsonValue>>* RowsArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("rows"), RowsArray) || RowsArray == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidField,
			TEXT("Missing required param: rows")
		);
	}

	FString Mode = TEXT("create");
	Params->TryGetStringField(TEXT("mode"), Mode);

	bool bDryRun = false;
	Params->TryGetBoolField(TEXT("dry_run"), bDryRun);

	if (Mode != TEXT("create") && Mode != TEXT("upsert") && Mode != TEXT("replace"))
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidValue,
			FString::Printf(TEXT("Invalid mode: %s. Must be create, upsert, or replace"), *Mode)
		);
	}

	FUDBCommandResult LoadError;
	UDataTable* DataTable = LoadDataTable(TablePath, LoadError);
	if (DataTable == nullptr)
	{
		return LoadError;
	}

	// Block import to composite DataTables
	const UCompositeDataTable* CompositeTable = Cast<UCompositeDataTable>(DataTable);
	if (CompositeTable != nullptr)
	{
		TSharedPtr<FJsonObject> Details = MakeShared<FJsonObject>();
		Details->SetArrayField(TEXT("parent_tables"), GetParentTablesJsonArray(CompositeTable));
		return FUDBCommandHandler::Error(
			UDBErrorCodes::CompositeWriteBlocked,
			FString::Printf(TEXT("Cannot import rows into CompositeDataTable '%s'. Import into one of its source tables instead."), *DataTable->GetName()),
			Details
		);
	}

	const UScriptStruct* RowStruct = DataTable->GetRowStruct();
	if (RowStruct == nullptr)
	{
		return FUDBCommandHandler::Error(
			UDBErrorCodes::InvalidStructType,
			FString::Printf(TEXT("DataTable has no row struct: %s"), *TablePath)
		);
	}

	if (Mode == TEXT("replace") && !bDryRun)
	{
		DataTable->EmptyTable();
	}

	int32 CreatedCount = 0;
	int32 UpdatedCount = 0;
	int32 SkippedCount = 0;
	TArray<FString> Errors;
	TArray<FString> Warnings;

	for (int32 Index = 0; Index < RowsArray->Num(); ++Index)
	{
		const TSharedPtr<FJsonValue>& RowEntry = (*RowsArray)[Index];
		if (!RowEntry.IsValid() || RowEntry->Type != EJson::Object)
		{
			Errors.Add(FString::Printf(TEXT("Row %d: invalid entry (not an object)"), Index));
			continue;
		}

		const TSharedPtr<FJsonObject>& RowEntryObj = RowEntry->AsObject();
		FString EntryRowName;
		if (!RowEntryObj->TryGetStringField(TEXT("row_name"), EntryRowName))
		{
			Errors.Add(FString::Printf(TEXT("Row %d: missing row_name"), Index));
			continue;
		}

		const TSharedPtr<FJsonObject>* EntryRowData = nullptr;
		if (!RowEntryObj->TryGetObjectField(TEXT("row_data"), EntryRowData) || EntryRowData == nullptr || !(*EntryRowData).IsValid())
		{
			Errors.Add(FString::Printf(TEXT("Row %d (%s): missing row_data"), Index, *EntryRowName));
			continue;
		}

		const FName EntryRowFName(*EntryRowName);
		uint8* ExistingRow = DataTable->FindRowUnchecked(EntryRowFName);
		bool bRowExists = (ExistingRow != nullptr);

		if (bDryRun)
		{
			// Validate by attempting deserialization into temp memory
			uint8* TempMemory = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
			RowStruct->InitializeStruct(TempMemory);

			TArray<FString> RowWarnings;
			bool bSuccess = FUDBSerializer::JsonToStruct(*EntryRowData, RowStruct, TempMemory, RowWarnings);

			RowStruct->DestroyStruct(TempMemory);
			FMemory::Free(TempMemory);

			if (!bSuccess)
			{
				Errors.Add(FString::Printf(TEXT("Row %d (%s): deserialization failed"), Index, *EntryRowName));
			}
			else
			{
				for (const FString& W : RowWarnings)
				{
					Warnings.Add(FString::Printf(TEXT("Row %d (%s): %s"), Index, *EntryRowName, *W));
				}

				if (bRowExists && Mode == TEXT("create"))
				{
					++SkippedCount;
				}
				else if (bRowExists)
				{
					++UpdatedCount;
				}
				else
				{
					++CreatedCount;
				}
			}
			continue;
		}

		// Non-dry-run execution
		if (bRowExists && Mode == TEXT("create"))
		{
			++SkippedCount;
			continue;
		}

		if (bRowExists && Mode == TEXT("upsert"))
		{
			// Update existing row in place
			TArray<FString> RowWarnings;
			bool bSuccess = FUDBSerializer::JsonToStruct(*EntryRowData, RowStruct, ExistingRow, RowWarnings);
			if (!bSuccess)
			{
				Errors.Add(FString::Printf(TEXT("Row %d (%s): deserialization failed"), Index, *EntryRowName));
				continue;
			}
			for (const FString& W : RowWarnings)
			{
				Warnings.Add(FString::Printf(TEXT("Row %d (%s): %s"), Index, *EntryRowName, *W));
			}
			DataTable->HandleDataTableChanged(EntryRowFName);
			++UpdatedCount;
		}
		else
		{
			// Create new row
			uint8* RowMemory = static_cast<uint8*>(FMemory::Malloc(RowStruct->GetStructureSize()));
			RowStruct->InitializeStruct(RowMemory);

			TArray<FString> RowWarnings;
			bool bSuccess = FUDBSerializer::JsonToStruct(*EntryRowData, RowStruct, RowMemory, RowWarnings);

			if (!bSuccess)
			{
				RowStruct->DestroyStruct(RowMemory);
				FMemory::Free(RowMemory);
				Errors.Add(FString::Printf(TEXT("Row %d (%s): deserialization failed"), Index, *EntryRowName));
				continue;
			}

			for (const FString& W : RowWarnings)
			{
				Warnings.Add(FString::Printf(TEXT("Row %d (%s): %s"), Index, *EntryRowName, *W));
			}

			DataTable->AddRow(EntryRowFName, RowMemory, RowStruct);

			RowStruct->DestroyStruct(RowMemory);
			FMemory::Free(RowMemory);
			++CreatedCount;
		}
	}

	if (!bDryRun)
	{
		DataTable->MarkPackageDirty();
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetNumberField(TEXT("created"), CreatedCount);
	Data->SetNumberField(TEXT("updated"), UpdatedCount);
	Data->SetNumberField(TEXT("skipped"), SkippedCount);

	if (Errors.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> ErrorsArray;
		for (const FString& Err : Errors)
		{
			ErrorsArray.Add(MakeShared<FJsonValueString>(Err));
		}
		Data->SetArrayField(TEXT("errors"), ErrorsArray);
	}

	if (Warnings.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> WarningsArray;
		for (const FString& Warning : Warnings)
		{
			WarningsArray.Add(MakeShared<FJsonValueString>(Warning));
		}
		Data->SetArrayField(TEXT("warnings"), WarningsArray);
	}

	FUDBCommandResult Result = FUDBCommandHandler::Success(Data);
	Result.Warnings = MoveTemp(Warnings);
	return Result;
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
