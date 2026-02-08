// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "UDBSerializer.h"
#include "UObject/UnrealType.h"
#include "UObject/TextProperty.h"
#include "UObject/EnumProperty.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/SoftObjectPath.h"
#include "Dom/JsonValue.h"

DEFINE_LOG_CATEGORY_STATIC(LogUDBSerializer, Log, All);

TSharedPtr<FJsonObject> FUDBSerializer::StructToJson(const UStruct* StructType, const void* StructData)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();

	if (StructType == nullptr || StructData == nullptr)
	{
		return JsonObject;
	}

	// Special case: if the top-level struct IS an FInstancedStruct, unwrap it
	if (StructType == FInstancedStruct::StaticStruct())
	{
		const FInstancedStruct* Instance = static_cast<const FInstancedStruct*>(StructData);
		if (Instance->IsValid())
		{
			JsonObject = StructToJson(Instance->GetScriptStruct(), Instance->GetMemory());
			JsonObject->SetStringField(TEXT("_struct_type"), Instance->GetScriptStruct()->GetName());
		}
		return JsonObject;
	}

	for (TFieldIterator<FProperty> It(StructType); It; ++It)
	{
		const FProperty* Property = *It;
		const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(StructData);

		TSharedPtr<FJsonValue> JsonValue = PropertyToJson(Property, ValuePtr);
		if (JsonValue.IsValid())
		{
			JsonObject->SetField(Property->GetName(), JsonValue);
		}
	}

	return JsonObject;
}

TSharedPtr<FJsonValue> FUDBSerializer::PropertyToJson(const FProperty* Property, const void* ValuePtr)
{
	if (Property == nullptr || ValuePtr == nullptr)
	{
		return nullptr;
	}

	// Bool
	if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue(ValuePtr));
	}

	// Int
	if (const FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(IntProp->GetPropertyValue(ValuePtr)));
	}

	// Int64
	if (const FInt64Property* Int64Prop = CastField<FInt64Property>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(Int64Prop->GetPropertyValue(ValuePtr)));
	}

	// Float
	if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(static_cast<double>(FloatProp->GetPropertyValue(ValuePtr)));
	}

	// Double
	if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		return MakeShared<FJsonValueNumber>(DoubleProp->GetPropertyValue(ValuePtr));
	}

	// FString
	if (const FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		return MakeShared<FJsonValueString>(StrProp->GetPropertyValue(ValuePtr));
	}

	// FName
	if (const FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		return MakeShared<FJsonValueString>(NameProp->GetPropertyValue(ValuePtr).ToString());
	}

	// FText
	if (const FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		const FText& TextValue = TextProp->GetPropertyValue(ValuePtr);
		return MakeShared<FJsonValueString>(TextValue.ToString());
	}

	// Enum property (enum class)
	if (const FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		const UEnum* Enum = EnumProp->GetEnum();
		const FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
		int64 Value = UnderlyingProp->GetSignedIntPropertyValue(ValuePtr);
		FString EnumName = Enum->GetNameStringByIndex(static_cast<int32>(Value));
		return MakeShared<FJsonValueString>(EnumName);
	}

	// Byte property with enum (old-style TEnumAsByte)
	if (const FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (const UEnum* Enum = ByteProp->GetIntPropertyEnum())
		{
			int64 Value = static_cast<int64>(ByteProp->GetPropertyValue(ValuePtr));
			FString EnumName = Enum->GetNameStringByIndex(static_cast<int32>(Value));
			return MakeShared<FJsonValueString>(EnumName);
		}
		return MakeShared<FJsonValueNumber>(static_cast<double>(ByteProp->GetPropertyValue(ValuePtr)));
	}

	// Struct property
	if (const FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		// FGameplayTag - serialize as tag string
		if (StructProp->Struct == FGameplayTag::StaticStruct())
		{
			const FGameplayTag* Tag = static_cast<const FGameplayTag*>(ValuePtr);
			return MakeShared<FJsonValueString>(Tag->ToString());
		}

		// FGameplayTagContainer - serialize as array of tag strings
		if (StructProp->Struct == FGameplayTagContainer::StaticStruct())
		{
			const FGameplayTagContainer* Container = static_cast<const FGameplayTagContainer*>(ValuePtr);
			TArray<TSharedPtr<FJsonValue>> TagArray;
			for (const FGameplayTag& Tag : *Container)
			{
				TagArray.Add(MakeShared<FJsonValueString>(Tag.ToString()));
			}
			return MakeShared<FJsonValueArray>(TagArray);
		}

		// FInstancedStruct - serialize with _struct_type discriminator
		if (StructProp->Struct == FInstancedStruct::StaticStruct())
		{
			const FInstancedStruct* Instance = static_cast<const FInstancedStruct*>(ValuePtr);
			if (Instance->IsValid())
			{
				TSharedPtr<FJsonObject> Obj = StructToJson(Instance->GetScriptStruct(), Instance->GetMemory());
				Obj->SetStringField(TEXT("_struct_type"), Instance->GetScriptStruct()->GetName());
				return MakeShared<FJsonValueObject>(Obj);
			}
			return MakeShared<FJsonValueNull>();
		}

		// FSoftObjectPath - serialize as string path
		if (StructProp->Struct == TBaseStructure<FSoftObjectPath>::Get())
		{
			const FSoftObjectPath* SoftPath = static_cast<const FSoftObjectPath*>(ValuePtr);
			return MakeShared<FJsonValueString>(SoftPath->ToString());
		}

		// Default: recursive struct serialization
		TSharedPtr<FJsonObject> NestedObj = StructToJson(StructProp->Struct, ValuePtr);
		return MakeShared<FJsonValueObject>(NestedObj);
	}

	// Array property
	if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		FScriptArrayHelper ArrayHelper(ArrayProp, ValuePtr);
		TArray<TSharedPtr<FJsonValue>> JsonArray;
		JsonArray.Reserve(ArrayHelper.Num());

		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			const void* ElementPtr = ArrayHelper.GetRawPtr(Index);
			TSharedPtr<FJsonValue> ElementValue = PropertyToJson(ArrayProp->Inner, ElementPtr);
			if (ElementValue.IsValid())
			{
				JsonArray.Add(ElementValue);
			}
		}

		return MakeShared<FJsonValueArray>(JsonArray);
	}

	// Map property
	if (const FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		FScriptMapHelper MapHelper(MapProp, ValuePtr);
		TSharedPtr<FJsonObject> MapObj = MakeShared<FJsonObject>();

		for (int32 Index = 0; Index < MapHelper.GetMaxIndex(); ++Index)
		{
			if (!MapHelper.IsValidIndex(Index))
			{
				continue;
			}

			// Get key as string
			const void* KeyPtr = MapHelper.GetKeyPtr(Index);
			FString KeyString;
			MapProp->KeyProp->ExportTextItem_Direct(KeyString, KeyPtr, nullptr, nullptr, PPF_None);

			// Get value
			const void* MapValuePtr = MapHelper.GetValuePtr(Index);
			TSharedPtr<FJsonValue> JsonValue = PropertyToJson(MapProp->ValueProp, MapValuePtr);
			if (JsonValue.IsValid())
			{
				MapObj->SetField(KeyString, JsonValue);
			}
		}

		return MakeShared<FJsonValueObject>(MapObj);
	}

	// Set property
	if (const FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		FScriptSetHelper SetHelper(SetProp, ValuePtr);
		TArray<TSharedPtr<FJsonValue>> JsonArray;

		for (int32 Index = 0; Index < SetHelper.GetMaxIndex(); ++Index)
		{
			if (!SetHelper.IsValidIndex(Index))
			{
				continue;
			}

			const void* ElementPtr = SetHelper.GetElementPtr(Index);
			TSharedPtr<FJsonValue> ElementValue = PropertyToJson(SetProp->ElementProp, ElementPtr);
			if (ElementValue.IsValid())
			{
				JsonArray.Add(ElementValue);
			}
		}

		return MakeShared<FJsonValueArray>(JsonArray);
	}

	// Object property (hard reference)
	if (const FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		const UObject* Object = ObjProp->GetObjectPropertyValue(ValuePtr);
		if (Object != nullptr)
		{
			return MakeShared<FJsonValueString>(Object->GetPathName());
		}
		return MakeShared<FJsonValueNull>();
	}

	// Soft object property
	if (const FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
	{
		const FSoftObjectPtr& SoftPtr = SoftObjProp->GetPropertyValue(ValuePtr);
		return MakeShared<FJsonValueString>(SoftPtr.ToSoftObjectPath().ToString());
	}

	UE_LOG(LogUDBSerializer, Warning, TEXT("Unhandled property type: %s (%s)"),
		*Property->GetName(), *Property->GetClass()->GetName());
	return nullptr;
}
