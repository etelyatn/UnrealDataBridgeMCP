
#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "UDBCommandHandler.h"

class FSocket;
class FTcpListener;

class UNREALDATABRIDGE_API FUDBTcpServer
{
public:
	FUDBTcpServer();
	~FUDBTcpServer();

	bool Start(int32 Port = 8742);
	void Stop();
	bool IsRunning() const;

private:
	bool HandleConnectionAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint);
	void ProcessClientData();

	/** Process data for a single client socket. Returns false if client should be removed. */
	bool ProcessSingleClient(FSocket* InClientSocket);

	/** Send a JSON response string followed by newline delimiter to a specific client */
	void SendResponse(FSocket* InClientSocket, const FString& ResponseString);

	/** Close and destroy a client socket */
	void DestroyClientSocket(FSocket* InClientSocket);

	static constexpr double CommandTimeoutWarningSeconds = 30.0;
	static constexpr int32 ReceiveBufferSize = 65536;

	TUniquePtr<FTcpListener> Listener;
	TArray<FSocket*> ClientSockets;
	TMap<FSocket*, FString> ReceiveBuffers;
	FThreadSafeBool bRunning = false;
	FTSTicker::FDelegateHandle TickDelegateHandle;
	FUDBCommandHandler CommandHandler;
};
