// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UNREALDATABRIDGE_API FUDBSerializer
{
public:
	/** Serialize a UStruct instance to a JSON object using UProperty reflection */
	static TSharedPtr<FJsonObject> StructToJson(const UStruct* StructType, const void* StructData);

	/** Serialize a single FProperty value to a JSON value */
	static TSharedPtr<FJsonValue> PropertyToJson(const FProperty* Property, const void* ValuePtr);
};
