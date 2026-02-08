// Copyright Mavka Games. All Rights Reserved. https://www.mavka.games/

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"

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

	TUniquePtr<FTcpListener> Listener;
	FSocket* ClientSocket = nullptr;
	FString ReceiveBuffer;
	FThreadSafeBool bRunning = false;
	FTSTicker::FDelegateHandle TickDelegateHandle;
};
