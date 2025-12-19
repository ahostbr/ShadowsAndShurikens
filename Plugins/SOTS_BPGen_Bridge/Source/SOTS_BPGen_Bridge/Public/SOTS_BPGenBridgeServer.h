#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Templates/SharedPointer.h"
#include "Containers/Queue.h"
#include "Async/Future.h"

class FSocket;

struct FSOTS_BPGenBridgeDispatchResult
{
	bool bOk = false;
	TSharedPtr<FJsonObject> Result;
	TArray<FString> Errors;
	TArray<FString> Warnings;
	FString ErrorCode;
};

class FSOTS_BPGenBridgeServer : public TSharedFromThis<FSOTS_BPGenBridgeServer>
{
public:
	FSOTS_BPGenBridgeServer();
	~FSOTS_BPGenBridgeServer();

	bool Start(const FString& InBindAddress = TEXT("127.0.0.1"), int32 InPort = 55557, int32 InMaxRequestBytes = 1024 * 1024);
	void Stop();
	bool IsRunning() const;

private:
	void AcceptLoop();
	void HandleConnection(FSocket* ClientSocket);
	bool ReadLine(FSocket* ClientSocket, TArray<uint8>& OutBytes, bool& bOutTooLarge) const;
	bool CheckRateLimit(FSOTS_BPGenBridgeDispatchResult& OutResult);
	bool CheckAuthToken(const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const;
	bool DispatchOnGameThread(const FString& Action, const FString& RequestId, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const;
	void RouteBpgenAction(const FString& Action, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const;
	FString BytesToString(const TArray<uint8>& Bytes) const;
	bool SendResponseAndClose(FSocket* ClientSocket, const FString& ResponseString) const;
	FString BuildResponseJson(const FString& Action, const FString& RequestId, const FSOTS_BPGenBridgeDispatchResult& DispatchResult) const;

private:
	TSharedPtr<FSocket> ListenSocket;
	TFuture<void> AcceptTask;
	TAtomic<bool> bRunning;
	FString BindAddress;
	int32 Port = 55557;
	int32 MaxRequestBytes = 1024 * 1024;
	float GameThreadTimeoutSeconds = 30.0f;
	FString AuthToken;
	int32 MaxRequestsPerSecond = 60;
	double RateWindowStartSeconds = 0.0;
	int32 RequestsInWindow = 0;
};
