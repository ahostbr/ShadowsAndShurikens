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
	double RequestMs = 0.0;
	double DispatchMs = 0.0;
};

class FSOTS_BPGenBridgeServer : public TSharedFromThis<FSOTS_BPGenBridgeServer>
{
public:
	FSOTS_BPGenBridgeServer();
	~FSOTS_BPGenBridgeServer();

	bool Start(const FString& InBindAddress = TEXT("127.0.0.1"), int32 InPort = 55557, int32 InMaxRequestBytes = 1024 * 1024);
	void Stop();
	bool IsRunning() const;

	/** Retrieve recent requests for UI display (lightweight). */
	void GetRecentRequestsForUI(TArray<TSharedPtr<FJsonObject>>& OutRequests) const;

	/** Retrieve server status snapshot for UI surfaces. */
	void GetServerInfoForUI(TSharedPtr<FJsonObject>& OutInfo) const;

private:
	void AcceptLoop();
	void HandleConnection(FSocket* ClientSocket);
	bool ReadLine(FSocket* ClientSocket, TArray<uint8>& OutBytes, bool& bOutTooLarge) const;
	bool CheckRateLimit(FSOTS_BPGenBridgeDispatchResult& OutResult);
	bool CheckAuthToken(const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const;
	bool CheckActionAllowed(const FString& Action, FSOTS_BPGenBridgeDispatchResult& OutResult) const;
	bool CheckDangerousGate(const FString& Action, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const;
	bool DispatchOnGameThread(const FString& Action, const FString& RequestId, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult);
	void RouteBpgenAction(const FString& Action, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult);
	FString BytesToString(const TArray<uint8>& Bytes) const;
	bool SendResponseAndClose(FSocket* ClientSocket, const FString& ResponseString) const;
	FString BuildResponseJson(const FString& Action, const FString& RequestId, const FSOTS_BPGenBridgeDispatchResult& DispatchResult) const;
	void AddRecentRequestSummary(const FString& RequestId, const FString& Action, const FSOTS_BPGenBridgeDispatchResult& DispatchResult);
	void PruneExpiredSessions();

private:
	TSharedPtr<FSocket> ListenSocket;
	TFuture<void> AcceptTask;
	TAtomic<bool> bRunning;
	TAtomic<bool> bStopping;
	FString BindAddress;
	int32 Port = 55557;
	int32 MaxRequestBytes = 1024 * 1024;
	float GameThreadTimeoutSeconds = 30.0f;
	FString AuthToken;
	bool bAllowNonLoopbackBind = false;
	bool bSafeMode = false;
	double ServerStartSeconds = 0.0;
	FDateTime ServerStartUtc;
	FString LastStartError;
	FString LastStartErrorCode;
	int32 MaxRequestsPerSecond = 60;
	double RateWindowStartSeconds = 0.0;
	int32 RequestsInWindow = 0;
	int32 MaxRequestsPerMinute = 0;
	double MinuteWindowStartSeconds = 0.0;
	int32 RequestsInMinute = 0;
	int32 TotalRequests = 0;
	TSet<FString> AllowedActions;
	TSet<FString> DeniedActions;
	int32 MaxDiscoveryResults = 200;
	int32 MaxPinHarvestNodes = 200;
	int32 MaxAutoFixSteps = 5;

	struct FSessionState
	{
		FString SessionId;
		FString LastBlueprintPath;
		double LastAccessSeconds = 0.0;
		bool bCachePrimed = false;
	};

	TMap<FString, FSessionState> Sessions;
	double SessionIdleSeconds = 600.0;

	struct FRecentRequestSummary
	{
		FString RequestId;
		FString Action;
		bool bOk = false;
		FString ErrorCode;
		double RequestMs = 0.0;
		FString Timestamp;
	};

	TArray<FRecentRequestSummary> RecentRequests;
	int32 MaxRecentRequests = 50;
	mutable FCriticalSection RecentRequestsMutex;
	mutable FCriticalSection BatchMutex;
};
