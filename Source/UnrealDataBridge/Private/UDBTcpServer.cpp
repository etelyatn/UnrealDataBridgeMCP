// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#include "UDBTcpServer.h"
#include "UDBCommandHandler.h"
#include "Common/TcpListener.h"
#include "SocketSubsystem.h"
#include "Sockets.h"
#include "Containers/Ticker.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
			FUDBCommandResult ParseError = FUDBCommandHandler::Error(
				TEXT("PARSE_ERROR"),
				TEXT("Failed to parse JSON request")
			);
			SendResponse(FUDBCommandHandler::ResultToJson(ParseError, 0.0));
			continue;
		}

		// Extract command
		FString Command;
		if (!RequestJson->TryGetStringField(TEXT("command"), Command))
		{
			UE_LOG(LogUDBTcpServer, Warning, TEXT("JSON missing 'command' field: %s"), *Line);
			FUDBCommandResult MissingCmd = FUDBCommandHandler::Error(
				TEXT("MISSING_COMMAND"),
				TEXT("JSON request missing 'command' field")
			);
			SendResponse(FUDBCommandHandler::ResultToJson(MissingCmd, 0.0));
			continue;
		}

		// Extract params (optional)
		const TSharedPtr<FJsonObject>* ParamsPtr = nullptr;
		TSharedPtr<FJsonObject> Params;
		if (RequestJson->TryGetObjectField(TEXT("params"), ParamsPtr) && ParamsPtr != nullptr)
		{
			Params = *ParamsPtr;
		}

		// Execute command with timing
		const double StartTime = FPlatformTime::Seconds();
		FUDBCommandResult Result = CommandHandler.Execute(Command, Params);
		const double EndTime = FPlatformTime::Seconds();
		const double TimingMs = (EndTime - StartTime) * 1000.0;

		SendResponse(FUDBCommandHandler::ResultToJson(Result, TimingMs));
	}
}

void FUDBTcpServer::SendResponse(const FString& ResponseString)
{
	if (ClientSocket == nullptr)
	{
		return;
	}

	FString ResponseWithNewline = ResponseString + TEXT("\n");
	FTCHARToUTF8 Utf8Response(*ResponseWithNewline);

	int32 BytesSent = 0;
	if (!ClientSocket->Send(
		reinterpret_cast<const uint8*>(Utf8Response.Get()),
		Utf8Response.Length(),
		BytesSent))
	{
		UE_LOG(LogUDBTcpServer, Warning, TEXT("Failed to send response"));
	}
}
