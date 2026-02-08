// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "UDBTcpServer.h"
#include "Common/TcpListener.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

DEFINE_LOG_CATEGORY_STATIC(LogUDBTcpServer, Log, All);

FUDBTcpServer::FUDBTcpServer()
{
}

FUDBTcpServer::~FUDBTcpServer()
{
	Stop();
}

bool FUDBTcpServer::Start(int32 Port)
{
	if (bRunning)
	{
		UE_LOG(LogUDBTcpServer, Warning, TEXT("TCP server is already running"));
		return false;
	}

	FIPv4Endpoint ListenEndpoint(FIPv4Address::InternalLoopback, Port);

	Listener = MakeUnique<FTcpListener>(ListenEndpoint);
	Listener->OnConnectionAccepted().BindRaw(this, &FUDBTcpServer::HandleConnectionAccepted);

	if (!Listener->IsActive())
	{
		UE_LOG(LogUDBTcpServer, Error, TEXT("Failed to start TCP listener on 127.0.0.1:%d"), Port);
		Listener.Reset();
		return false;
	}

	bRunning = true;

	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateLambda([this](float DeltaTime) -> bool
		{
			if (bRunning)
			{
				ProcessClientData();
			}
			return bRunning;
		}),
		0.0f
	);

	UE_LOG(LogUDBTcpServer, Log, TEXT("TCP server listening on 127.0.0.1:%d"), Port);
	return true;
}

void FUDBTcpServer::Stop()
{
	if (!bRunning)
	{
		return;
	}

	bRunning = false;

	if (TickDelegateHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
		TickDelegateHandle.Reset();
	}

	if (ClientSocket != nullptr)
	{
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
	}

	Listener.Reset();
	ReceiveBuffer.Empty();

	UE_LOG(LogUDBTcpServer, Log, TEXT("TCP server stopped"));
}

bool FUDBTcpServer::IsRunning() const
{
	return bRunning;
}

bool FUDBTcpServer::HandleConnectionAccepted(FSocket* InClientSocket, const FIPv4Endpoint& ClientEndpoint)
{
	if (ClientSocket != nullptr)
	{
		UE_LOG(LogUDBTcpServer, Warning, TEXT("Rejecting connection from %s - already have a client"), *ClientEndpoint.ToString());
		InClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(InClientSocket);
		return true;
	}

	UE_LOG(LogUDBTcpServer, Log, TEXT("Client connected from %s"), *ClientEndpoint.ToString());
	ClientSocket = InClientSocket;
	ReceiveBuffer.Empty();
	return true;
}

void FUDBTcpServer::ProcessClientData()
{
	if (ClientSocket == nullptr)
	{
		return;
	}

	// Check connection state
	ESocketConnectionState ConnectionState = ClientSocket->GetConnectionState();
	if (ConnectionState == SCS_ConnectionError)
	{
		UE_LOG(LogUDBTcpServer, Log, TEXT("Client disconnected"));
		ClientSocket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
		ClientSocket = nullptr;
		ReceiveBuffer.Empty();
		return;
	}

	// Read available data
	uint32 PendingDataSize = 0;
	if (!ClientSocket->HasPendingData(PendingDataSize) || PendingDataSize == 0)
	{
		return;
	}

	uint8 TempBuffer[4096];
	int32 BytesRead = 0;

	if (!ClientSocket->Recv(TempBuffer, sizeof(TempBuffer) - 1, BytesRead) || BytesRead <= 0)
	{
		return;
	}

	FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(TempBuffer), BytesRead);
	ReceiveBuffer.Append(Converter.Get(), Converter.Length());

	// Process complete lines (delimited by \n)
	int32 NewlineIndex = INDEX_NONE;
	while (ReceiveBuffer.FindChar(TEXT('\n'), NewlineIndex))
	{
		FString Line = ReceiveBuffer.Left(NewlineIndex);
		ReceiveBuffer.RemoveAt(0, NewlineIndex + 1);

		Line.TrimStartAndEndInline();
		if (Line.IsEmpty())
		{
			continue;
		}

		// Parse JSON
		TSharedPtr<FJsonObject> RequestJson;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Line);

		if (!FJsonSerializer::Deserialize(Reader, RequestJson) || !RequestJson.IsValid())
		{
			UE_LOG(LogUDBTcpServer, Warning, TEXT("Failed to parse JSON: %s"), *Line);
			continue;
		}

		// Route command
		FString Command;
		if (!RequestJson->TryGetStringField(TEXT("command"), Command))
		{
			UE_LOG(LogUDBTcpServer, Warning, TEXT("JSON missing 'command' field: %s"), *Line);
			continue;
		}

		FString ResponseString;

		if (Command == TEXT("ping"))
		{
			TSharedRef<FJsonObject> ResponseJson = MakeShared<FJsonObject>();
			ResponseJson->SetBoolField(TEXT("success"), true);

			TSharedRef<FJsonObject> DataJson = MakeShared<FJsonObject>();
			DataJson->SetStringField(TEXT("message"), TEXT("pong"));
			ResponseJson->SetObjectField(TEXT("data"), DataJson);

			TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
				TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&ResponseString);
			FJsonSerializer::Serialize(ResponseJson, Writer);
		}
		else
		{
			UE_LOG(LogUDBTcpServer, Warning, TEXT("Unknown command: %s"), *Command);
			continue;
		}

		// Send response as UTF-8 with newline delimiter
		ResponseString.Append(TEXT("\n"));
		FTCHARToUTF8 Utf8Response(*ResponseString);

		int32 BytesSent = 0;
		if (!ClientSocket->Send(
			reinterpret_cast<const uint8*>(Utf8Response.Get()),
			Utf8Response.Length(),
			BytesSent))
		{
			UE_LOG(LogUDBTcpServer, Warning, TEXT("Failed to send response"));
		}
	}
}
