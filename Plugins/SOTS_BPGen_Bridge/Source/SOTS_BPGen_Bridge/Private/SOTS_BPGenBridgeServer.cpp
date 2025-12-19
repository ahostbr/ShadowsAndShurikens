#include "SOTS_BPGenBridgeServer.h"
#include "SOTS_BPGen_BridgeModule.h"
#include "SOTS_BPGenBuilder.h"
#include "SOTS_BPGenDiscovery.h"
#include "SOTS_BPGenEnsure.h"
#include "SOTS_BPGenInspector.h"
#include "JsonObjectConverter.h"
#include "Engine/Blueprint.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Async/Async.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
static const FString GProtocolVersion = TEXT("1.0");

static TSharedPtr<FJsonObject> BuildFeatureFlags(bool bSupportsDryRun, bool bHasAuthToken, bool bHasRateLimit)
{
	TSharedPtr<FJsonObject> Features = MakeShared<FJsonObject>();
	Features->SetBoolField(TEXT("targets"), true);
	Features->SetBoolField(TEXT("ensure_function"), true);
	Features->SetBoolField(TEXT("ensure_variable"), true);
	Features->SetBoolField(TEXT("umg"), true);
	Features->SetBoolField(TEXT("describe_node_links"), true);
	Features->SetBoolField(TEXT("error_codes"), true);
	Features->SetBoolField(TEXT("graph_edits"), true);
	Features->SetBoolField(TEXT("auto_fix"), true);
	Features->SetBoolField(TEXT("recipes"), true);
	Features->SetBoolField(TEXT("dry_run"), bSupportsDryRun);
	Features->SetBoolField(TEXT("auth_token"), bHasAuthToken);
	Features->SetBoolField(TEXT("rate_limit"), bHasRateLimit);
	return Features;
}

static FString GetBridgeAuthToken()
{
	FString Token;
	if (GConfig)
	{
		GConfig->GetString(TEXT("SOTS_BPGen_Bridge"), TEXT("AuthToken"), Token, GEngineIni);
	}

	if (Token.IsEmpty())
	{
		Token = FPlatformMisc::GetEnvironmentVariable(TEXT("SOTS_BPGEN_AUTH_TOKEN"));
	}

	Token.TrimStartAndEndInline();
	return Token;
}

static int32 GetBridgeMaxRequestsPerSecond()
{
	int32 Value = 60;
	if (GConfig)
	{
		GConfig->GetInt(TEXT("SOTS_BPGen_Bridge"), TEXT("MaxRequestsPerSecond"), Value, GEngineIni);
	}

	const FString EnvValue = FPlatformMisc::GetEnvironmentVariable(TEXT("SOTS_BPGEN_MAX_RPS"));
	if (!EnvValue.IsEmpty())
	{
		Value = FCString::Atoi(*EnvValue);
	}

	return Value;
}

static int32 ExtractMajorVersion(const FString& VersionString)
{
	TArray<FString> Parts;
	VersionString.ParseIntoArray(Parts, TEXT("."));
	if (Parts.Num() == 0)
	{
		return 0;
	}

	return FCString::Atoi(*Parts[0]);
}

static void CollectDryRunNodeIds(const FSOTS_BPGenGraphSpec& GraphSpec, TArray<FString>& OutNodeIds)
{
	OutNodeIds.Reset();
	OutNodeIds.Reserve(GraphSpec.Nodes.Num());

	for (const FSOTS_BPGenGraphNode& Node : GraphSpec.Nodes)
	{
		const FString NodeId = !Node.NodeId.IsEmpty() ? Node.NodeId : Node.Id;
		if (!NodeId.IsEmpty())
		{
			OutNodeIds.Add(NodeId);
		}
	}
}

static FSOTS_BPGenApplyResult BuildDryRunApplyResult(const FString& BlueprintPath, const FName& FunctionName, const FSOTS_BPGenGraphSpec& GraphSpec)
{
	FSOTS_BPGenApplyResult Result;
	Result.bSuccess = true;
	Result.TargetBlueprintPath = BlueprintPath;
	Result.FunctionName = FunctionName;
	Result.Warnings.Add(TEXT("Dry run: no changes applied."));
	CollectDryRunNodeIds(GraphSpec, Result.CreatedNodeIds);
	return Result;
}

static bool HasPinName(const FSOTS_BPGenNodeSummary& Summary, const FName& PinName)
{
	for (const FSOTS_BPGenDiscoveredPinDescriptor& Pin : Summary.Pins)
	{
		if (Pin.PinName == PinName)
		{
			return true;
		}
	}

	return false;
}

static bool HasLinkMatch(const TArray<FSOTS_BPGenNodeLink>& Links, const FString& FromNodeId, const FName& FromPinName, const FString& ToNodeId, const FName& ToPinName)
{
	for (const FSOTS_BPGenNodeLink& Link : Links)
	{
		if (Link.FromNodeId == FromNodeId
			&& Link.FromPinName == FromPinName
			&& Link.ToNodeId == ToNodeId
			&& Link.ToPinName == ToPinName)
		{
			return true;
		}
	}

	return false;
}

static bool TryParseGraphTarget(const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenGraphTarget& OutTarget, FString& OutError)
{
	OutTarget = FSOTS_BPGenGraphTarget();
	OutError.Reset();

	if (!Params.IsValid())
	{
		OutError = TEXT("Missing params.");
		return false;
	}

	if (Params->HasTypedField<EJson::Object>(TEXT("target")))
	{
		const TSharedPtr<FJsonObject> TargetObj = Params->GetObjectField(TEXT("target"));
		FJsonObjectConverter::JsonObjectToUStruct(TargetObj.ToSharedRef(), FSOTS_BPGenGraphTarget::StaticStruct(), &OutTarget, 0, 0);
	}

	if (OutTarget.BlueprintAssetPath.IsEmpty())
	{
		Params->TryGetStringField(TEXT("blueprint_asset_path"), OutTarget.BlueprintAssetPath);
	}

	if (OutTarget.TargetType.IsEmpty())
	{
		Params->TryGetStringField(TEXT("target_type"), OutTarget.TargetType);
	}

	if (OutTarget.Name.IsEmpty())
	{
		FString FunctionName;
		if (Params->TryGetStringField(TEXT("function_name"), FunctionName))
		{
			OutTarget.Name = FunctionName;
		}
	}

	if (Params->HasField(TEXT("create_if_missing")))
	{
		Params->TryGetBoolField(TEXT("create_if_missing"), OutTarget.bCreateIfMissing);
	}

	if (OutTarget.TargetType.IsEmpty())
	{
		OutTarget.TargetType = TEXT("Function");
	}

	if (OutTarget.BlueprintAssetPath.IsEmpty())
	{
		OutError = TEXT("Blueprint asset path is required.");
		return false;
	}

	return true;
}

static void BuildRecipeResult(const FSOTS_BPGenGraphSpec& GraphSpec, const FSOTS_BPGenApplyResult& ApplyResult, FSOTS_BPGenBridgeDispatchResult& OutResult)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> GraphSpecJson = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> ApplyJson = MakeShared<FJsonObject>();

	FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphSpec::StaticStruct(), &GraphSpec, GraphSpecJson.ToSharedRef(), 0, 0);
	FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &ApplyResult, ApplyJson.ToSharedRef(), 0, 0);

	ResultJson->SetObjectField(TEXT("expanded_graph_spec"), GraphSpecJson);
	ResultJson->SetObjectField(TEXT("apply_result"), ApplyJson);

	OutResult.bOk = ApplyResult.bSuccess;
	OutResult.Result = ResultJson;
	OutResult.Warnings.Append(ApplyResult.Warnings);
	if (!ApplyResult.bSuccess && !ApplyResult.ErrorMessage.IsEmpty())
	{
		OutResult.Errors.Add(ApplyResult.ErrorMessage);
	}
}
}

FSOTS_BPGenBridgeServer::FSOTS_BPGenBridgeServer()
	: bRunning(false)
{
}

FSOTS_BPGenBridgeServer::~FSOTS_BPGenBridgeServer()
{
	Stop();
}

bool FSOTS_BPGenBridgeServer::Start(const FString& InBindAddress, int32 InPort, int32 InMaxRequestBytes)
{
	Stop();

	BindAddress = InBindAddress;
	Port = InPort;
	MaxRequestBytes = InMaxRequestBytes;
	AuthToken = GetBridgeAuthToken();
	MaxRequestsPerSecond = GetBridgeMaxRequestsPerSecond();
	if (MaxRequestsPerSecond < 0)
	{
		MaxRequestsPerSecond = 0;
	}
	RateWindowStartSeconds = 0.0;
	RequestsInWindow = 0;

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return false;
	}

	FSocket* RawSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("BPGenBridgeListen"), false);
	ListenSocket = MakeShareable(RawSocket);
	if (!ListenSocket.IsValid())
	{
		return false;
	}

	FIPv4Address ParsedAddress;
	if (!FIPv4Address::Parse(BindAddress, ParsedAddress))
	{
		Stop();
		return false;
	}

	TSharedRef<FInternetAddr> ListenAddr = SocketSubsystem->CreateInternetAddr();
	ListenAddr->SetIp(ParsedAddress.Value);
	ListenAddr->SetPort(Port);

	if (!ListenSocket->Bind(*ListenAddr) || !ListenSocket->Listen(16))
	{
		Stop();
		return false;
	}

	bRunning = true;
	TSharedPtr<FSOTS_BPGenBridgeServer> Self = AsShared();
	AcceptTask = Async(EAsyncExecution::Thread, [Self]()
	{
		Self->AcceptLoop();
	});

	return true;
}

void FSOTS_BPGenBridgeServer::Stop()
{
	bRunning = false;

	if (ListenSocket.IsValid())
	{
		ListenSocket->Close();
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (SocketSubsystem)
		{
			SocketSubsystem->DestroySocket(ListenSocket.Get());
		}
		ListenSocket.Reset();
	}

	if (AcceptTask.IsValid())
	{
		AcceptTask.Wait();
	}
}

bool FSOTS_BPGenBridgeServer::IsRunning() const
{
	return bRunning;
}

void FSOTS_BPGenBridgeServer::AcceptLoop()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		return;
	}

	while (bRunning)
	{
		if (!ListenSocket.IsValid())
		{
			break;
		}

		TSharedRef<FInternetAddr> ClientAddr = SocketSubsystem->CreateInternetAddr();
		FSocket* Accepted = ListenSocket->Accept(*ClientAddr, TEXT("BPGenBridgeClient"));
		if (!Accepted)
		{
			FPlatformProcess::Sleep(0.01f);
			continue;
		}

		HandleConnection(Accepted);
	}
}

bool FSOTS_BPGenBridgeServer::ReadLine(FSocket* ClientSocket, TArray<uint8>& OutBytes, bool& bOutTooLarge) const
{
	OutBytes.Reset();
	bOutTooLarge = false;
	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(1024);

	while (bRunning && OutBytes.Num() < MaxRequestBytes)
	{
		uint32 PendingData = 0;
		if (!ClientSocket->HasPendingData(PendingData))
		{
			FPlatformProcess::Sleep(0.005f);
			continue;
		}

		int32 BytesRead = 0;
		if (!ClientSocket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead))
		{
			return false;
		}

		if (BytesRead <= 0)
		{
			continue;
		}

		OutBytes.Append(Buffer.GetData(), BytesRead);

		if (OutBytes.Contains(static_cast<uint8>('\n')))
		{
			return true;
		}
	}

	if (OutBytes.Num() >= MaxRequestBytes && !OutBytes.Contains(static_cast<uint8>('\n')))
	{
		bOutTooLarge = true;
	}

	return OutBytes.Num() > 0;
}

FString FSOTS_BPGenBridgeServer::BytesToString(const TArray<uint8>& Bytes) const
{
	const FString Parsed = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Bytes.GetData())));
	return Parsed.Replace(TEXT("\r"), TEXT(""));
}

void FSOTS_BPGenBridgeServer::HandleConnection(FSocket* ClientSocket)
{
	TSharedPtr<FSocket> Client = MakeShareable(ClientSocket);
	if (!Client.IsValid())
	{
		return;
	}

	TArray<uint8> RequestBytes;
	bool bTooLarge = false;
	if (!ReadLine(Client.Get(), RequestBytes, bTooLarge))
	{
		Client->Close();
		return;
	}

	if (bTooLarge)
	{
		FSOTS_BPGenBridgeDispatchResult ErrorResult;
		ErrorResult.bOk = false;
		ErrorResult.ErrorCode = TEXT("ERR_REQUEST_TOO_LARGE");
		ErrorResult.Errors.Add(FString::Printf(TEXT("Request exceeded max_bytes limit (%d)."), MaxRequestBytes));
		const FString Response = BuildResponseJson(TEXT("unknown"), TEXT(""), ErrorResult);
		SendResponseAndClose(Client.Get(), Response);
		return;
	}

	const FString RequestString = BytesToString(RequestBytes);

	TSharedPtr<FJsonObject> RequestObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RequestString);
	if (!FJsonSerializer::Deserialize(Reader, RequestObject) || !RequestObject.IsValid())
	{
		FSOTS_BPGenBridgeDispatchResult ErrorResult;
		ErrorResult.bOk = false;
		ErrorResult.Errors.Add(TEXT("Failed to parse JSON request."));
		const FString Response = BuildResponseJson(TEXT("unknown"), TEXT(""), ErrorResult);
		SendResponseAndClose(Client.Get(), Response);
		return;
	}

	const FString Action = RequestObject->GetStringField(TEXT("action"));
	const FString RequestId = RequestObject->GetStringField(TEXT("request_id"));

	TSharedPtr<FJsonObject> Params;
	if (RequestObject->HasTypedField<EJson::Object>(TEXT("params")))
	{
		Params = RequestObject->GetObjectField(TEXT("params"));
	}
	else
	{
		Params = MakeShared<FJsonObject>();
	}

	FSOTS_BPGenBridgeDispatchResult DispatchResult;
	if (!CheckRateLimit(DispatchResult))
	{
		const FString Response = BuildResponseJson(Action, RequestId, DispatchResult);
		SendResponseAndClose(Client.Get(), Response);
		return;
	}

	if (!CheckAuthToken(Params, DispatchResult))
	{
		const FString Response = BuildResponseJson(Action, RequestId, DispatchResult);
		SendResponseAndClose(Client.Get(), Response);
		return;
	}

	if (!DispatchOnGameThread(Action, RequestId, Params, DispatchResult))
	{
		DispatchResult.bOk = false;
		DispatchResult.Errors.Add(TEXT("Request timed out while executing on game thread."));
	}

	const FString Response = BuildResponseJson(Action, RequestId, DispatchResult);
	SendResponseAndClose(Client.Get(), Response);
}

bool FSOTS_BPGenBridgeServer::CheckRateLimit(FSOTS_BPGenBridgeDispatchResult& OutResult)
{
	if (MaxRequestsPerSecond <= 0)
	{
		return true;
	}

	const double NowSeconds = FPlatformTime::Seconds();
	if (RateWindowStartSeconds <= 0.0 || (NowSeconds - RateWindowStartSeconds) >= 1.0)
	{
		RateWindowStartSeconds = NowSeconds;
		RequestsInWindow = 0;
	}

	++RequestsInWindow;
	if (RequestsInWindow > MaxRequestsPerSecond)
	{
		OutResult.bOk = false;
		OutResult.ErrorCode = TEXT("ERR_RATE_LIMIT");
		OutResult.Errors.Add(FString::Printf(TEXT("Rate limit exceeded (%d requests/second)."), MaxRequestsPerSecond));
		return false;
	}

	return true;
}

bool FSOTS_BPGenBridgeServer::CheckAuthToken(const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const
{
	if (AuthToken.IsEmpty())
	{
		return true;
	}

	FString ProvidedToken;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("auth_token"), ProvidedToken) || ProvidedToken != AuthToken)
	{
		OutResult.bOk = false;
		OutResult.ErrorCode = TEXT("ERR_UNAUTHORIZED");
		OutResult.Errors.Add(TEXT("Missing or invalid auth_token."));
		return false;
	}

	return true;
}

bool FSOTS_BPGenBridgeServer::DispatchOnGameThread(const FString& Action, const FString& RequestId, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const
{
	FEvent* DoneEvent = FPlatformProcess::GetSynchEventFromPool(true);

	AsyncTask(ENamedThreads::GameThread, [this, Action, Params, &OutResult, DoneEvent]()
	{
		RouteBpgenAction(Action, Params, OutResult);
		DoneEvent->Trigger();
	});

	const bool bTriggered = DoneEvent->Wait(FTimespan::FromSeconds(GameThreadTimeoutSeconds));
	FPlatformProcess::ReturnSynchEventToPool(DoneEvent);
	return bTriggered;
}

void FSOTS_BPGenBridgeServer::RouteBpgenAction(const FString& Action, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const
{
	OutResult = FSOTS_BPGenBridgeDispatchResult();

	if (Action.Equals(TEXT("ping"), ESearchCase::IgnoreCase))
	{
		OutResult.bOk = true;
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("pong"), true);
		ResultObj->SetStringField(TEXT("time"), FDateTime::UtcNow().ToIso8601());
		ResultObj->SetStringField(TEXT("version"), TEXT("0.1"));
		ResultObj->SetStringField(TEXT("protocol_version"), GProtocolVersion);
		OutResult.Result = ResultObj;
		return;
	}

	FString ClientProtocol;
	if (Params.IsValid())
	{
		Params->TryGetStringField(TEXT("client_protocol_version"), ClientProtocol);
	}

	if (!ClientProtocol.IsEmpty())
	{
		const int32 ClientMajor = ExtractMajorVersion(ClientProtocol);
		const int32 ServerMajor = ExtractMajorVersion(GProtocolVersion);
		if (ClientMajor != ServerMajor)
		{
			OutResult.bOk = false;
			OutResult.ErrorCode = TEXT("ERR_PROTOCOL_MISMATCH");
			OutResult.Errors.Add(FString::Printf(TEXT("Client protocol %s incompatible with server %s"), *ClientProtocol, *GProtocolVersion));
			return;
		}
	}

	bool bDryRun = false;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("dry_run"), bDryRun);
	}

	if (Action.Equals(TEXT("apply_graph_spec"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenFunctionDef FunctionDef;
		FSOTS_BPGenGraphSpec GraphSpec;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), FunctionDef.TargetBlueprintPath);
			FString FunctionNameString;
			if (Params->TryGetStringField(TEXT("function_name"), FunctionNameString))
			{
				FunctionDef.FunctionName = FName(*FunctionNameString);
			}

			if (Params->HasTypedField<EJson::Object>(TEXT("function_def")))
			{
				const TSharedPtr<FJsonObject> FunctionObj = Params->GetObjectField(TEXT("function_def"));
				FJsonObjectConverter::JsonObjectToUStruct(FunctionObj.ToSharedRef(), FSOTS_BPGenFunctionDef::StaticStruct(), &FunctionDef, 0, 0);
			}

			if (Params->HasTypedField<EJson::Object>(TEXT("graph_spec")))
			{
				const TSharedPtr<FJsonObject> GraphObj = Params->GetObjectField(TEXT("graph_spec"));
				FJsonObjectConverter::JsonObjectToUStruct(GraphObj.ToSharedRef(), FSOTS_BPGenGraphSpec::StaticStruct(), &GraphSpec, 0, 0);
			}
		}

		if (bDryRun)
		{
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(FunctionDef.TargetBlueprintPath, FunctionDef.FunctionName, GraphSpec);
			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &DryRunResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			ResultJson->SetNumberField(TEXT("planned_links"), GraphSpec.Links.Num());
			OutResult.bOk = true;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryRunResult.Warnings);
			return;
		}

		const FSOTS_BPGenApplyResult ApplyResult = USOTS_BPGenBuilder::ApplyGraphSpecToFunction(nullptr, FunctionDef, GraphSpec);

		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &ApplyResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = ApplyResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(ApplyResult.Warnings);
		if (!ApplyResult.bSuccess && !ApplyResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(ApplyResult.ErrorMessage);
		}
		return;
	}

	if (Action.Equals(TEXT("apply_graph_spec_to_target"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenGraphSpec GraphSpec;

		if (Params.IsValid() && Params->HasTypedField<EJson::Object>(TEXT("graph_spec")))
		{
			const TSharedPtr<FJsonObject> GraphObj = Params->GetObjectField(TEXT("graph_spec"));
			FJsonObjectConverter::JsonObjectToUStruct(GraphObj.ToSharedRef(), FSOTS_BPGenGraphSpec::StaticStruct(), &GraphSpec, 0, 0);
		}

		if (bDryRun)
		{
			const FName TargetName = GraphSpec.Target.Name.IsEmpty() ? NAME_None : FName(*GraphSpec.Target.Name);
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(GraphSpec.Target.BlueprintAssetPath, TargetName, GraphSpec);
			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &DryRunResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			ResultJson->SetNumberField(TEXT("planned_links"), GraphSpec.Links.Num());
			OutResult.bOk = true;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryRunResult.Warnings);
			return;
		}

		const FSOTS_BPGenApplyResult ApplyResult = USOTS_BPGenBuilder::ApplyGraphSpecToTarget(nullptr, GraphSpec);

		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &ApplyResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = ApplyResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(ApplyResult.Warnings);
		if (!ApplyResult.bSuccess && !ApplyResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(ApplyResult.ErrorMessage);
		}
		return;
	}

	if (Action.Equals(TEXT("recipe_print_string"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenGraphTarget Target;
		FString TargetError;
		if (!TryParseGraphTarget(Params, Target, TargetError))
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TargetError);
			return;
		}

		FString Message = TEXT("Hello from BPGen");
		FString NodeIdPrefix = TEXT("recipe_print_");
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("message"), Message);
			Params->TryGetStringField(TEXT("node_id_prefix"), NodeIdPrefix);
		}

		FSOTS_BPGenGraphSpec GraphSpec;
		GraphSpec.Target = Target;

		FSOTS_BPGenGraphNode PrintNode;
		PrintNode.Id = NodeIdPrefix + TEXT("print");
		PrintNode.NodeId = PrintNode.Id;
		PrintNode.NodeType = FName(TEXT("K2Node_CallFunction"));
		PrintNode.SpawnerKey = TEXT("/Script/Engine.KismetSystemLibrary:PrintString");
		PrintNode.bPreferSpawnerKey = true;
		PrintNode.NodePosition = FVector2D(320.f, 0.f);
		PrintNode.ExtraData.Add(TEXT("InString.DefaultValue"), Message);
		GraphSpec.Nodes.Add(PrintNode);

		FSOTS_BPGenGraphLink EntryToPrint;
		EntryToPrint.FromNodeId = TEXT("Entry");
		EntryToPrint.FromPinName = FName(TEXT("then"));
		EntryToPrint.ToNodeId = PrintNode.Id;
		EntryToPrint.ToPinName = FName(TEXT("execute"));
		EntryToPrint.bUseSchema = true;
		GraphSpec.Links.Add(EntryToPrint);

		FSOTS_BPGenGraphLink PrintToResult;
		PrintToResult.FromNodeId = PrintNode.Id;
		PrintToResult.FromPinName = FName(TEXT("then"));
		PrintToResult.ToNodeId = TEXT("Result");
		PrintToResult.ToPinName = FName(TEXT("execute"));
		PrintToResult.bUseSchema = true;
		GraphSpec.Links.Add(PrintToResult);

		if (bDryRun)
		{
			const FName TargetName = GraphSpec.Target.Name.IsEmpty() ? NAME_None : FName(*GraphSpec.Target.Name);
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(GraphSpec.Target.BlueprintAssetPath, TargetName, GraphSpec);
			BuildRecipeResult(GraphSpec, DryRunResult, OutResult);
			if (OutResult.Result.IsValid())
			{
				OutResult.Result->SetBoolField(TEXT("dry_run"), true);
				OutResult.Result->SetNumberField(TEXT("planned_links"), GraphSpec.Links.Num());
			}
			return;
		}

		const FSOTS_BPGenApplyResult ApplyResult = USOTS_BPGenBuilder::ApplyGraphSpecToTarget(nullptr, GraphSpec);
		BuildRecipeResult(GraphSpec, ApplyResult, OutResult);
		return;
	}

	if (Action.Equals(TEXT("recipe_branch_on_bool"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenGraphTarget Target;
		FString TargetError;
		if (!TryParseGraphTarget(Params, Target, TargetError))
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TargetError);
			return;
		}

		bool bCondition = false;
		FString NodeIdPrefix = TEXT("recipe_branch_");
		if (Params.IsValid())
		{
			Params->TryGetBoolField(TEXT("condition"), bCondition);
			Params->TryGetStringField(TEXT("node_id_prefix"), NodeIdPrefix);
		}

		FSOTS_BPGenGraphSpec GraphSpec;
		GraphSpec.Target = Target;

		FSOTS_BPGenGraphNode BranchNode;
		BranchNode.Id = NodeIdPrefix + TEXT("branch");
		BranchNode.NodeId = BranchNode.Id;
		BranchNode.NodeType = FName(TEXT("K2Node_IfThenElse"));
		BranchNode.SpawnerKey = TEXT("/Script/BlueprintGraph.K2Node_IfThenElse");
		BranchNode.bPreferSpawnerKey = true;
		BranchNode.NodePosition = FVector2D(280.f, 0.f);
		BranchNode.ExtraData.Add(TEXT("Condition.DefaultValue"), bCondition ? TEXT("true") : TEXT("false"));
		GraphSpec.Nodes.Add(BranchNode);

		FSOTS_BPGenGraphLink EntryToBranch;
		EntryToBranch.FromNodeId = TEXT("Entry");
		EntryToBranch.FromPinName = FName(TEXT("then"));
		EntryToBranch.ToNodeId = BranchNode.Id;
		EntryToBranch.ToPinName = FName(TEXT("execute"));
		EntryToBranch.bUseSchema = true;
		GraphSpec.Links.Add(EntryToBranch);

		FSOTS_BPGenGraphLink BranchToResult;
		BranchToResult.FromNodeId = BranchNode.Id;
		BranchToResult.FromPinName = FName(TEXT("then"));
		BranchToResult.ToNodeId = TEXT("Result");
		BranchToResult.ToPinName = FName(TEXT("execute"));
		BranchToResult.bUseSchema = true;
		GraphSpec.Links.Add(BranchToResult);

		if (bDryRun)
		{
			const FName TargetName = GraphSpec.Target.Name.IsEmpty() ? NAME_None : FName(*GraphSpec.Target.Name);
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(GraphSpec.Target.BlueprintAssetPath, TargetName, GraphSpec);
			BuildRecipeResult(GraphSpec, DryRunResult, OutResult);
			if (OutResult.Result.IsValid())
			{
				OutResult.Result->SetBoolField(TEXT("dry_run"), true);
				OutResult.Result->SetNumberField(TEXT("planned_links"), GraphSpec.Links.Num());
			}
			return;
		}

		const FSOTS_BPGenApplyResult ApplyResult = USOTS_BPGenBuilder::ApplyGraphSpecToTarget(nullptr, GraphSpec);
		BuildRecipeResult(GraphSpec, ApplyResult, OutResult);
		return;
	}

	if (Action.Equals(TEXT("recipe_set_variable"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenGraphTarget Target;
		FString TargetError;
		if (!TryParseGraphTarget(Params, Target, TargetError))
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TargetError);
			return;
		}

		FString VariableName;
		FString DefaultValue;
		FString ValueLiteral;
		FString NodeIdPrefix = TEXT("recipe_set_");
		FSOTS_BPGenPin VarType;
		bool bHasPinType = false;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("variable_name"), VariableName);
			Params->TryGetStringField(TEXT("default_value"), DefaultValue);
			Params->TryGetStringField(TEXT("value"), ValueLiteral);
			Params->TryGetStringField(TEXT("node_id_prefix"), NodeIdPrefix);

			if (Params->HasTypedField<EJson::Object>(TEXT("pin_type")))
			{
				const TSharedPtr<FJsonObject> PinObj = Params->GetObjectField(TEXT("pin_type"));
				FJsonObjectConverter::JsonObjectToUStruct(PinObj.ToSharedRef(), FSOTS_BPGenPin::StaticStruct(), &VarType, 0, 0);
				bHasPinType = true;
			}
		}

		if (VariableName.IsEmpty() || !bHasPinType)
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("recipe_set_variable requires variable_name and pin_type."));
			return;
		}

		const FString EnsureDefault = !DefaultValue.IsEmpty() ? DefaultValue : ValueLiteral;

		UBlueprint* Blueprint = Cast<UBlueprint>(StaticLoadObject(UBlueprint::StaticClass(), nullptr, *Target.BlueprintAssetPath));
		UClass* OwnerClass = Blueprint ? Blueprint->GeneratedClass : nullptr;
		if (!OwnerClass && Blueprint)
		{
			OwnerClass = Blueprint->SkeletonGeneratedClass;
		}

		if (!OwnerClass)
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("recipe_set_variable failed to resolve blueprint owner class."));
			return;
		}

		const FString SpawnerKey = FString::Printf(TEXT("%s:%s"), *OwnerClass->GetPathName(), *VariableName);
		const FString ValueString = !ValueLiteral.IsEmpty() ? ValueLiteral : DefaultValue;

		FSOTS_BPGenGraphSpec GraphSpec;
		GraphSpec.Target = Target;

		FSOTS_BPGenGraphNode SetNode;
		SetNode.Id = NodeIdPrefix + VariableName;
		SetNode.NodeId = SetNode.Id;
		SetNode.NodeType = FName(TEXT("K2Node_VariableSet"));
		SetNode.SpawnerKey = SpawnerKey;
		SetNode.bPreferSpawnerKey = true;
		SetNode.VariableName = FName(*VariableName);
		SetNode.NodePosition = FVector2D(320.f, 0.f);
		if (!ValueString.IsEmpty())
		{
			SetNode.ExtraData.Add(*FString::Printf(TEXT("%s.DefaultValue"), *VariableName), ValueString);
		}
		GraphSpec.Nodes.Add(SetNode);

		FSOTS_BPGenGraphLink EntryToSet;
		EntryToSet.FromNodeId = TEXT("Entry");
		EntryToSet.FromPinName = FName(TEXT("then"));
		EntryToSet.ToNodeId = SetNode.Id;
		EntryToSet.ToPinName = FName(TEXT("execute"));
		EntryToSet.bUseSchema = true;
		GraphSpec.Links.Add(EntryToSet);

		FSOTS_BPGenGraphLink SetToResult;
		SetToResult.FromNodeId = SetNode.Id;
		SetToResult.FromPinName = FName(TEXT("then"));
		SetToResult.ToNodeId = TEXT("Result");
		SetToResult.ToPinName = FName(TEXT("execute"));
		SetToResult.bUseSchema = true;
		GraphSpec.Links.Add(SetToResult);

		if (bDryRun)
		{
			const FName TargetName = GraphSpec.Target.Name.IsEmpty() ? NAME_None : FName(*GraphSpec.Target.Name);
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(GraphSpec.Target.BlueprintAssetPath, TargetName, GraphSpec);
			BuildRecipeResult(GraphSpec, DryRunResult, OutResult);

			if (OutResult.Result.IsValid())
			{
				FSOTS_BPGenEnsureResult DryEnsure;
				DryEnsure.bSuccess = true;
				DryEnsure.BlueprintPath = Target.BlueprintAssetPath;
				DryEnsure.Name = VariableName;
				DryEnsure.Warnings.Add(TEXT("Dry run: ensure_variable skipped."));

				TSharedPtr<FJsonObject> EnsureJson = MakeShared<FJsonObject>();
				FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &DryEnsure, EnsureJson.ToSharedRef(), 0, 0);
				OutResult.Result->SetObjectField(TEXT("ensure_result"), EnsureJson);
				OutResult.Result->SetBoolField(TEXT("dry_run"), true);
				OutResult.Result->SetNumberField(TEXT("planned_links"), GraphSpec.Links.Num());
			}

			return;
		}

		const FSOTS_BPGenEnsureResult EnsureResult = USOTS_BPGenEnsure::EnsureVariable(nullptr, Target.BlueprintAssetPath, VariableName, VarType, EnsureDefault, true, false, true, true);
		if (!EnsureResult.bSuccess && !EnsureResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(EnsureResult.ErrorMessage);
		}
		OutResult.Warnings.Append(EnsureResult.Warnings);

		const FSOTS_BPGenApplyResult ApplyResult = USOTS_BPGenBuilder::ApplyGraphSpecToTarget(nullptr, GraphSpec);
		BuildRecipeResult(GraphSpec, ApplyResult, OutResult);

		if (OutResult.Result.IsValid())
		{
			TSharedPtr<FJsonObject> EnsureJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &EnsureResult, EnsureJson.ToSharedRef(), 0, 0);
			OutResult.Result->SetObjectField(TEXT("ensure_result"), EnsureJson);
		}

		return;
	}

	if (Action.Equals(TEXT("recipe_widget_bind_text"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FString WidgetName;
		FString PropertyName = TEXT("Text");
		FString FunctionName;
		FString Message = TEXT("BPGen Binding");
		FString NodeIdPrefix = TEXT("recipe_bind_");

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			Params->TryGetStringField(TEXT("widget_name"), WidgetName);
			Params->TryGetStringField(TEXT("property_name"), PropertyName);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetStringField(TEXT("message"), Message);
			Params->TryGetStringField(TEXT("node_id_prefix"), NodeIdPrefix);
		}

		if (BlueprintPath.IsEmpty() || WidgetName.IsEmpty() || FunctionName.IsEmpty())
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("recipe_widget_bind_text requires blueprint_asset_path, widget_name, and function_name."));
			return;
		}

		FSOTS_BPGenBindingRequest BindingRequest;
		BindingRequest.BlueprintAssetPath = BlueprintPath;
		BindingRequest.WidgetName = WidgetName;
		BindingRequest.PropertyName = PropertyName;
		BindingRequest.FunctionName = FunctionName;
		BindingRequest.bApplyGraph = false;

		FSOTS_BPGenPin ReturnPin;
		ReturnPin.Name = FName(TEXT("ReturnValue"));
		ReturnPin.Category = FName(TEXT("text"));
		BindingRequest.Signature.Outputs.Add(ReturnPin);

		FSOTS_BPGenGraphSpec GraphSpec;
		GraphSpec.Target.BlueprintAssetPath = BlueprintPath;
		GraphSpec.Target.TargetType = TEXT("WidgetBinding");
		GraphSpec.Target.Name = FunctionName;
		GraphSpec.Target.bCreateIfMissing = true;

		FSOTS_BPGenGraphNode ConvNode;
		ConvNode.Id = NodeIdPrefix + TEXT("text");
		ConvNode.NodeId = ConvNode.Id;
		ConvNode.NodeType = FName(TEXT("K2Node_CallFunction"));
		ConvNode.SpawnerKey = TEXT("/Script/Engine.KismetTextLibrary:Conv_StringToText");
		ConvNode.bPreferSpawnerKey = true;
		ConvNode.NodePosition = FVector2D(240.f, 0.f);
		ConvNode.ExtraData.Add(TEXT("InString.DefaultValue"), Message);
		GraphSpec.Nodes.Add(ConvNode);

		FSOTS_BPGenGraphLink LinkToReturn;
		LinkToReturn.FromNodeId = ConvNode.Id;
		LinkToReturn.FromPinName = FName(TEXT("ReturnValue"));
		LinkToReturn.ToNodeId = TEXT("Result");
		LinkToReturn.ToPinName = FName(TEXT("ReturnValue"));
		LinkToReturn.bUseSchema = true;
		GraphSpec.Links.Add(LinkToReturn);

		if (bDryRun)
		{
			const FName TargetName = GraphSpec.Target.Name.IsEmpty() ? NAME_None : FName(*GraphSpec.Target.Name);
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(GraphSpec.Target.BlueprintAssetPath, TargetName, GraphSpec);
			BuildRecipeResult(GraphSpec, DryRunResult, OutResult);

			if (OutResult.Result.IsValid())
			{
				FSOTS_BPGenBindingEnsureResult DryBinding;
				DryBinding.bSuccess = true;
				DryBinding.BlueprintPath = BlueprintPath;
				DryBinding.WidgetName = WidgetName;
				DryBinding.PropertyName = PropertyName;
				DryBinding.FunctionName = FunctionName;
				DryBinding.Warnings.Add(TEXT("Dry run: ensure_binding skipped."));

				TSharedPtr<FJsonObject> BindingJson = MakeShared<FJsonObject>();
				FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenBindingEnsureResult::StaticStruct(), &DryBinding, BindingJson.ToSharedRef(), 0, 0);
				OutResult.Result->SetObjectField(TEXT("binding_result"), BindingJson);
				OutResult.Result->SetBoolField(TEXT("dry_run"), true);
				OutResult.Result->SetNumberField(TEXT("planned_links"), GraphSpec.Links.Num());
			}

			return;
		}

		const FSOTS_BPGenBindingEnsureResult BindingResult = USOTS_BPGenEnsure::EnsureWidgetBinding(nullptr, BindingRequest);
		if (!BindingResult.bSuccess && !BindingResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(BindingResult.ErrorMessage);
		}
		OutResult.Warnings.Append(BindingResult.Warnings);

		const FSOTS_BPGenApplyResult ApplyResult = USOTS_BPGenBuilder::ApplyGraphSpecToTarget(nullptr, GraphSpec);
		BuildRecipeResult(GraphSpec, ApplyResult, OutResult);

		if (OutResult.Result.IsValid())
		{
			TSharedPtr<FJsonObject> BindingJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenBindingEnsureResult::StaticStruct(), &BindingResult, BindingJson.ToSharedRef(), 0, 0);
			OutResult.Result->SetObjectField(TEXT("binding_result"), BindingJson);
		}

		return;
	}

	if (Action.Equals(TEXT("discover_nodes"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FString SearchText;
		int32 MaxResults = 200;
		bool bIncludePins = false;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			Params->TryGetStringField(TEXT("search_text"), SearchText);
			Params->TryGetNumberField(TEXT("max_results"), MaxResults);
			Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);
		}

		const FSOTS_BPGenNodeDiscoveryResult Discovery = USOTS_BPGenDiscovery::DiscoverNodesWithDescriptors(nullptr, BlueprintPath, SearchText, MaxResults, bIncludePins);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenNodeDiscoveryResult::StaticStruct(), &Discovery, ResultJson.ToSharedRef(), 0, 0);

		OutResult.bOk = Discovery.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(Discovery.Errors);
		OutResult.Warnings.Append(Discovery.Warnings);
		return;
	}

	if (Action.Equals(TEXT("ensure_function"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FString FunctionName;
		FSOTS_BPGenFunctionSignature Signature;
		bool bCreateIfMissing = true;
		bool bUpdateIfExists = true;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			Params->TryGetStringField(TEXT("function_name"), FunctionName);
			Params->TryGetBoolField(TEXT("create_if_missing"), bCreateIfMissing);
			Params->TryGetBoolField(TEXT("update_if_exists"), bUpdateIfExists);

			if (Params->HasTypedField<EJson::Object>(TEXT("signature")))
			{
				const TSharedPtr<FJsonObject> SigObj = Params->GetObjectField(TEXT("signature"));
				FJsonObjectConverter::JsonObjectToUStruct(SigObj.ToSharedRef(), FSOTS_BPGenFunctionSignature::StaticStruct(), &Signature, 0, 0);
			}
		}

		if (BlueprintPath.IsEmpty() || FunctionName.IsEmpty())
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("Missing blueprint_asset_path or function_name for ensure_function."));
			return;
		}

		if (bDryRun)
		{
			FSOTS_BPGenEnsureResult DryEnsure;
			DryEnsure.bSuccess = true;
			DryEnsure.BlueprintPath = BlueprintPath;
			DryEnsure.Name = FunctionName;
			DryEnsure.Warnings.Add(TEXT("Dry run: ensure_function skipped."));

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &DryEnsure, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = true;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryEnsure.Warnings);
			return;
		}

		const FSOTS_BPGenEnsureResult EnsureResult = USOTS_BPGenEnsure::EnsureFunction(nullptr, BlueprintPath, FunctionName, Signature, bCreateIfMissing, bUpdateIfExists);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &EnsureResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = EnsureResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(EnsureResult.Warnings);
		if (!EnsureResult.bSuccess && !EnsureResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(EnsureResult.ErrorMessage);
		}
		return;
	}

	if (Action.Equals(TEXT("ensure_variable"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FString VarName;
		FSOTS_BPGenPin VarType;
		FString DefaultValue;
		bool bInstanceEditable = true;
		bool bExposeOnSpawn = false;
		bool bCreateIfMissing = true;
		bool bUpdateIfExists = true;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			Params->TryGetStringField(TEXT("variable_name"), VarName);
			Params->TryGetStringField(TEXT("default_value"), DefaultValue);
			Params->TryGetBoolField(TEXT("instance_editable"), bInstanceEditable);
			Params->TryGetBoolField(TEXT("expose_on_spawn"), bExposeOnSpawn);
			Params->TryGetBoolField(TEXT("create_if_missing"), bCreateIfMissing);
			Params->TryGetBoolField(TEXT("update_if_exists"), bUpdateIfExists);

			if (Params->HasTypedField<EJson::Object>(TEXT("pin_type")))
			{
				const TSharedPtr<FJsonObject> PinObj = Params->GetObjectField(TEXT("pin_type"));
				FJsonObjectConverter::JsonObjectToUStruct(PinObj.ToSharedRef(), FSOTS_BPGenPin::StaticStruct(), &VarType, 0, 0);
			}
		}

		if (BlueprintPath.IsEmpty() || VarName.IsEmpty())
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("Missing blueprint_asset_path or variable_name for ensure_variable."));
			return;
		}

		if (bDryRun)
		{
			FSOTS_BPGenEnsureResult DryEnsure;
			DryEnsure.bSuccess = true;
			DryEnsure.BlueprintPath = BlueprintPath;
			DryEnsure.Name = VarName;
			DryEnsure.Warnings.Add(TEXT("Dry run: ensure_variable skipped."));

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &DryEnsure, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = true;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryEnsure.Warnings);
			return;
		}

		const FSOTS_BPGenEnsureResult EnsureResult = USOTS_BPGenEnsure::EnsureVariable(nullptr, BlueprintPath, VarName, VarType, DefaultValue, bInstanceEditable, bExposeOnSpawn, bCreateIfMissing, bUpdateIfExists);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &EnsureResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = EnsureResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(EnsureResult.Warnings);
		if (!EnsureResult.bSuccess && !EnsureResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(EnsureResult.ErrorMessage);
		}
		return;
	}

	if (Action.Equals(TEXT("ensure_widget"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenWidgetSpec Spec;
		if (Params.IsValid())
		{
			FJsonObjectConverter::JsonObjectToUStruct(Params.ToSharedRef(), FSOTS_BPGenWidgetSpec::StaticStruct(), &Spec, 0, 0);
		}

		if (bDryRun)
		{
			FSOTS_BPGenWidgetEnsureResult DryEnsure;
			DryEnsure.BlueprintPath = Spec.BlueprintAssetPath;
			DryEnsure.WidgetName = Spec.WidgetName;
			DryEnsure.ParentName = Spec.ParentName;
			if (Spec.BlueprintAssetPath.IsEmpty() || Spec.WidgetClassPath.IsEmpty() || Spec.WidgetName.IsEmpty())
			{
				DryEnsure.bSuccess = false;
				DryEnsure.ErrorMessage = TEXT("ensure_widget requires blueprint_asset_path, widget_class_path, and widget_name.");
			}
			else
			{
				DryEnsure.bSuccess = true;
				DryEnsure.Warnings.Add(TEXT("Dry run: ensure_widget skipped."));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenWidgetEnsureResult::StaticStruct(), &DryEnsure, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryEnsure.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryEnsure.Warnings);
			if (!DryEnsure.bSuccess && !DryEnsure.ErrorMessage.IsEmpty())
			{
				OutResult.Errors.Add(DryEnsure.ErrorMessage);
			}
			return;
		}

		const FSOTS_BPGenWidgetEnsureResult EnsureResult = USOTS_BPGenEnsure::EnsureWidgetComponent(nullptr, Spec);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenWidgetEnsureResult::StaticStruct(), &EnsureResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = EnsureResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(EnsureResult.Warnings);
		if (!EnsureResult.bSuccess && !EnsureResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(EnsureResult.ErrorMessage);
		}
		return;
	}

	if (Action.Equals(TEXT("set_widget_properties"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenWidgetPropertyRequest Request;
		if (Params.IsValid())
		{
			FJsonObjectConverter::JsonObjectToUStruct(Params.ToSharedRef(), FSOTS_BPGenWidgetPropertyRequest::StaticStruct(), &Request, 0, 0);
		}

		if (bDryRun)
		{
			FSOTS_BPGenWidgetPropertyResult DryResult;
			DryResult.BlueprintPath = Request.BlueprintAssetPath;
			DryResult.WidgetName = Request.WidgetName;
			if (Request.BlueprintAssetPath.IsEmpty() || Request.WidgetName.IsEmpty())
			{
				DryResult.bSuccess = false;
				DryResult.ErrorMessage = TEXT("set_widget_properties requires blueprint_asset_path and widget_name.");
			}
			else if (Request.Properties.Num() == 0)
			{
				DryResult.bSuccess = false;
				DryResult.ErrorMessage = TEXT("set_widget_properties requires properties.");
			}
			else
			{
				DryResult.bSuccess = true;
				Request.Properties.GetKeys(DryResult.AppliedProperties);
				DryResult.Warnings.Add(TEXT("Dry run: set_widget_properties skipped."));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenWidgetPropertyResult::StaticStruct(), &DryResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryResult.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryResult.Warnings);
			if (!DryResult.bSuccess && !DryResult.ErrorMessage.IsEmpty())
			{
				OutResult.Errors.Add(DryResult.ErrorMessage);
			}
			return;
		}

		const FSOTS_BPGenWidgetPropertyResult PropResult = USOTS_BPGenEnsure::SetWidgetProperties(nullptr, Request);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenWidgetPropertyResult::StaticStruct(), &PropResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = PropResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(PropResult.Warnings);
		if (!PropResult.bSuccess && !PropResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(PropResult.ErrorMessage);
		}
		return;
	}

	if (Action.Equals(TEXT("ensure_binding"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenBindingRequest BindingRequest;
		if (Params.IsValid())
		{
			FJsonObjectConverter::JsonObjectToUStruct(Params.ToSharedRef(), FSOTS_BPGenBindingRequest::StaticStruct(), &BindingRequest, 0, 0);
		}

		if (bDryRun)
		{
			FSOTS_BPGenBindingEnsureResult DryBinding;
			DryBinding.BlueprintPath = BindingRequest.BlueprintAssetPath;
			DryBinding.WidgetName = BindingRequest.WidgetName;
			DryBinding.PropertyName = BindingRequest.PropertyName;
			DryBinding.FunctionName = BindingRequest.FunctionName;
			if (BindingRequest.BlueprintAssetPath.IsEmpty() || BindingRequest.WidgetName.IsEmpty()
				|| BindingRequest.PropertyName.IsEmpty() || BindingRequest.FunctionName.IsEmpty())
			{
				DryBinding.bSuccess = false;
				DryBinding.ErrorMessage = TEXT("ensure_binding requires blueprint_asset_path, widget_name, property_name, and function_name.");
			}
			else
			{
				DryBinding.bSuccess = true;
				DryBinding.Warnings.Add(TEXT("Dry run: ensure_binding skipped."));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenBindingEnsureResult::StaticStruct(), &DryBinding, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryBinding.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryBinding.Warnings);
			if (!DryBinding.bSuccess && !DryBinding.ErrorMessage.IsEmpty())
			{
				OutResult.Errors.Add(DryBinding.ErrorMessage);
			}
			return;
		}

		const FSOTS_BPGenBindingEnsureResult BindingResult = USOTS_BPGenEnsure::EnsureWidgetBinding(nullptr, BindingRequest);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenBindingEnsureResult::StaticStruct(), &BindingResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = BindingResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(BindingResult.Warnings);
		if (!BindingResult.bSuccess && !BindingResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(BindingResult.ErrorMessage);
		}
		return;
	}

	if (Action.Equals(TEXT("list_nodes"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FName FunctionName(NAME_None);
		bool bIncludePins = false;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			FString FunctionNameString;
			if (Params->TryGetStringField(TEXT("function_name"), FunctionNameString))
			{
				FunctionName = FName(*FunctionNameString);
			}
			Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);
		}

		const FSOTS_BPGenNodeListResult ListResult = USOTS_BPGenInspector::ListFunctionGraphNodes(nullptr, BlueprintPath, FunctionName, bIncludePins);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenNodeListResult::StaticStruct(), &ListResult, ResultJson.ToSharedRef(), 0, 0);

		OutResult.bOk = ListResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(ListResult.Errors);
		OutResult.Warnings.Append(ListResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("describe_node"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FName FunctionName(NAME_None);
		FString NodeId;
		bool bIncludePins = true;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			FString FunctionNameString;
			if (Params->TryGetStringField(TEXT("function_name"), FunctionNameString))
			{
				FunctionName = FName(*FunctionNameString);
			}
			Params->TryGetStringField(TEXT("node_id"), NodeId);
			Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);
		}

		const FSOTS_BPGenNodeDescribeResult DescribeResult = USOTS_BPGenInspector::DescribeNodeById(nullptr, BlueprintPath, FunctionName, NodeId, bIncludePins);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenNodeDescribeResult::StaticStruct(), &DescribeResult, ResultJson.ToSharedRef(), 0, 0);

		OutResult.bOk = DescribeResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(DescribeResult.Errors);
		OutResult.Warnings.Append(DescribeResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("compile_blueprint"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
		}

		if (bDryRun)
		{
			FSOTS_BPGenMaintenanceResult DryResult;
			DryResult.BlueprintPath = BlueprintPath;
			if (BlueprintPath.IsEmpty())
			{
				DryResult.bSuccess = false;
				DryResult.Errors.Add(TEXT("compile_blueprint requires blueprint_asset_path."));
			}
			else
			{
				DryResult.bSuccess = true;
				DryResult.Message = TEXT("Dry run: compile_blueprint skipped.");
				DryResult.Warnings.Add(TEXT("Dry run: no changes applied."));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenMaintenanceResult::StaticStruct(), &DryResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryResult.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryResult.Warnings);
			OutResult.Errors.Append(DryResult.Errors);
			return;
		}

		const FSOTS_BPGenMaintenanceResult CompileResult = USOTS_BPGenInspector::CompileBlueprintAsset(nullptr, BlueprintPath);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenMaintenanceResult::StaticStruct(), &CompileResult, ResultJson.ToSharedRef(), 0, 0);

		OutResult.bOk = CompileResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(CompileResult.Errors);
		OutResult.Warnings.Append(CompileResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("save_blueprint"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
		}

		if (bDryRun)
		{
			FSOTS_BPGenMaintenanceResult DryResult;
			DryResult.BlueprintPath = BlueprintPath;
			if (BlueprintPath.IsEmpty())
			{
				DryResult.bSuccess = false;
				DryResult.Errors.Add(TEXT("save_blueprint requires blueprint_asset_path."));
			}
			else
			{
				DryResult.bSuccess = true;
				DryResult.Message = TEXT("Dry run: save_blueprint skipped.");
				DryResult.Warnings.Add(TEXT("Dry run: no changes applied."));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenMaintenanceResult::StaticStruct(), &DryResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryResult.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryResult.Warnings);
			OutResult.Errors.Append(DryResult.Errors);
			return;
		}

		const FSOTS_BPGenMaintenanceResult SaveResult = USOTS_BPGenInspector::SaveBlueprintAsset(nullptr, BlueprintPath);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenMaintenanceResult::StaticStruct(), &SaveResult, ResultJson.ToSharedRef(), 0, 0);

		OutResult.bOk = SaveResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(SaveResult.Errors);
		OutResult.Warnings.Append(SaveResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("refresh_nodes"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FName FunctionName(NAME_None);
		bool bIncludePins = false;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			FString FunctionNameString;
			if (Params->TryGetStringField(TEXT("function_name"), FunctionNameString))
			{
				FunctionName = FName(*FunctionNameString);
			}
			Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);
		}

		if (bDryRun)
		{
			FSOTS_BPGenMaintenanceResult DryResult;
			DryResult.BlueprintPath = BlueprintPath;
			DryResult.FunctionName = FunctionName;
			if (BlueprintPath.IsEmpty() || FunctionName.IsNone())
			{
				DryResult.bSuccess = false;
				DryResult.Errors.Add(TEXT("refresh_nodes requires blueprint_asset_path and function_name."));
			}
			else
			{
				DryResult.bSuccess = true;
				DryResult.Message = TEXT("Dry run: refresh_nodes skipped.");
				DryResult.Warnings.Add(TEXT("Dry run: no changes applied."));
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenMaintenanceResult::StaticStruct(), &DryResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryResult.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryResult.Warnings);
			OutResult.Errors.Append(DryResult.Errors);
			return;
		}

		const FSOTS_BPGenMaintenanceResult RefreshResult = USOTS_BPGenInspector::RefreshAllNodesInFunction(nullptr, BlueprintPath, FunctionName, bIncludePins);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenMaintenanceResult::StaticStruct(), &RefreshResult, ResultJson.ToSharedRef(), 0, 0);

		OutResult.bOk = RefreshResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(RefreshResult.Errors);
		OutResult.Warnings.Append(RefreshResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("delete_node"), ESearchCase::IgnoreCase) || Action.Equals(TEXT("delete_node_by_id"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenDeleteNodeRequest Request;
		if (Params.IsValid())
		{
			FJsonObjectConverter::JsonObjectToUStruct(Params.ToSharedRef(), FSOTS_BPGenDeleteNodeRequest::StaticStruct(), &Request, 0, 0);
			Params->TryGetStringField(TEXT("blueprint_asset_path"), Request.BlueprintAssetPath);
			FString FunctionNameString;
			if (Params->TryGetStringField(TEXT("function_name"), FunctionNameString))
			{
				Request.FunctionName = FName(*FunctionNameString);
			}
			Params->TryGetStringField(TEXT("node_id"), Request.NodeId);
			bool bCompileValue = true;
			if (Params->TryGetBoolField(TEXT("compile"), bCompileValue))
			{
				Request.bCompile = bCompileValue;
			}
		}

		if (bDryRun)
		{
			FSOTS_BPGenGraphEditResult DryResult;
			DryResult.BlueprintPath = Request.BlueprintAssetPath;
			DryResult.FunctionName = Request.FunctionName;

			if (Request.BlueprintAssetPath.IsEmpty() || Request.FunctionName.IsNone() || Request.NodeId.IsEmpty())
			{
				DryResult.Errors.Add(TEXT("DeleteNodeById: blueprint_asset_path, function_name, and node_id are required."));
			}
			else
			{
				const FSOTS_BPGenNodeDescribeResult DescribeResult = USOTS_BPGenInspector::DescribeNodeById(nullptr, Request.BlueprintAssetPath, Request.FunctionName, Request.NodeId, false);
				if (!DescribeResult.bSuccess)
				{
					DryResult.Errors.Append(DescribeResult.Errors);
					DryResult.Warnings.Append(DescribeResult.Warnings);
				}
				else
				{
					DryResult.bSuccess = true;
					DryResult.AffectedNodeIds.Add(Request.NodeId);
					DryResult.Message = FString::Printf(TEXT("Dry run: would delete node '%s'."), *Request.NodeId);
					DryResult.Warnings.Add(TEXT("Dry run: no changes applied."));
				}
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphEditResult::StaticStruct(), &DryResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryResult.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Errors.Append(DryResult.Errors);
			OutResult.Warnings.Append(DryResult.Warnings);
			return;
		}

		const FSOTS_BPGenGraphEditResult EditResult = USOTS_BPGenBuilder::DeleteNodeById(nullptr, Request);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphEditResult::StaticStruct(), &EditResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = EditResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(EditResult.Errors);
		OutResult.Warnings.Append(EditResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("delete_link"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenDeleteLinkRequest Request;
		if (Params.IsValid())
		{
			FJsonObjectConverter::JsonObjectToUStruct(Params.ToSharedRef(), FSOTS_BPGenDeleteLinkRequest::StaticStruct(), &Request, 0, 0);
			Params->TryGetStringField(TEXT("blueprint_asset_path"), Request.BlueprintAssetPath);
			FString FunctionNameString;
			if (Params->TryGetStringField(TEXT("function_name"), FunctionNameString))
			{
				Request.FunctionName = FName(*FunctionNameString);
			}
			Params->TryGetStringField(TEXT("from_node_id"), Request.FromNodeId);
			Params->TryGetStringField(TEXT("to_node_id"), Request.ToNodeId);
			FString FromPinString;
			if (Params->TryGetStringField(TEXT("from_pin"), FromPinString))
			{
				Request.FromPinName = FName(*FromPinString);
			}
			FString ToPinString;
			if (Params->TryGetStringField(TEXT("to_pin"), ToPinString))
			{
				Request.ToPinName = FName(*ToPinString);
			}
			bool bCompileValue = true;
			if (Params->TryGetBoolField(TEXT("compile"), bCompileValue))
			{
				Request.bCompile = bCompileValue;
			}
		}

		if (bDryRun)
		{
			FSOTS_BPGenGraphEditResult DryResult;
			DryResult.BlueprintPath = Request.BlueprintAssetPath;
			DryResult.FunctionName = Request.FunctionName;

			if (Request.BlueprintAssetPath.IsEmpty() || Request.FunctionName.IsNone() || Request.FromNodeId.IsEmpty() || Request.ToNodeId.IsEmpty())
			{
				DryResult.Errors.Add(TEXT("DeleteLink: blueprint_asset_path, function_name, from_node_id, and to_node_id are required."));
			}
			else
			{
				const FSOTS_BPGenNodeDescribeResult FromDescribe = USOTS_BPGenInspector::DescribeNodeById(nullptr, Request.BlueprintAssetPath, Request.FunctionName, Request.FromNodeId, true);
				const FSOTS_BPGenNodeDescribeResult ToDescribe = USOTS_BPGenInspector::DescribeNodeById(nullptr, Request.BlueprintAssetPath, Request.FunctionName, Request.ToNodeId, true);

				if (!FromDescribe.bSuccess)
				{
					DryResult.Errors.Append(FromDescribe.Errors);
					DryResult.Warnings.Append(FromDescribe.Warnings);
				}
				if (!ToDescribe.bSuccess)
				{
					DryResult.Errors.Append(ToDescribe.Errors);
					DryResult.Warnings.Append(ToDescribe.Warnings);
				}

				if (DryResult.Errors.Num() == 0)
				{
					if (!HasPinName(FromDescribe.Description.Summary, Request.FromPinName))
					{
						DryResult.Errors.Add(FString::Printf(TEXT("DeleteLink: Pin '%s' not found on node '%s'."), *Request.FromPinName.ToString(), *Request.FromNodeId));
					}
					if (!HasPinName(ToDescribe.Description.Summary, Request.ToPinName))
					{
						DryResult.Errors.Add(FString::Printf(TEXT("DeleteLink: Pin '%s' not found on node '%s'."), *Request.ToPinName.ToString(), *Request.ToNodeId));
					}
				}

				if (DryResult.Errors.Num() == 0)
				{
					const bool bHasLink = HasLinkMatch(FromDescribe.Description.Links, Request.FromNodeId, Request.FromPinName, Request.ToNodeId, Request.ToPinName);
					if (!bHasLink)
					{
						DryResult.Warnings.Add(TEXT("DeleteLink: Link did not exist; nothing to remove."));
					}

					DryResult.bSuccess = true;
					DryResult.AffectedNodeIds.Add(Request.FromNodeId);
					DryResult.AffectedNodeIds.Add(Request.ToNodeId);
					DryResult.Message = FString::Printf(TEXT("Dry run: would remove link %s.%s -> %s.%s."), *Request.FromNodeId, *Request.FromPinName.ToString(), *Request.ToNodeId, *Request.ToPinName.ToString());
					DryResult.Warnings.Add(TEXT("Dry run: no changes applied."));
				}
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphEditResult::StaticStruct(), &DryResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryResult.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Errors.Append(DryResult.Errors);
			OutResult.Warnings.Append(DryResult.Warnings);
			return;
		}

		const FSOTS_BPGenGraphEditResult EditResult = USOTS_BPGenBuilder::DeleteLink(nullptr, Request);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphEditResult::StaticStruct(), &EditResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = EditResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(EditResult.Errors);
		OutResult.Warnings.Append(EditResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("replace_node"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenReplaceNodeRequest Request;
		if (Params.IsValid())
		{
			FJsonObjectConverter::JsonObjectToUStruct(Params.ToSharedRef(), FSOTS_BPGenReplaceNodeRequest::StaticStruct(), &Request, 0, 0);
			Params->TryGetStringField(TEXT("blueprint_asset_path"), Request.BlueprintAssetPath);
			FString FunctionNameString;
			if (Params->TryGetStringField(TEXT("function_name"), FunctionNameString))
			{
				Request.FunctionName = FName(*FunctionNameString);
			}
			Params->TryGetStringField(TEXT("existing_node_id"), Request.ExistingNodeId);
			if (Params->HasTypedField<EJson::Object>(TEXT("new_node")))
			{
				const TSharedPtr<FJsonObject> NewNodeObj = Params->GetObjectField(TEXT("new_node"));
				FJsonObjectConverter::JsonObjectToUStruct(NewNodeObj.ToSharedRef(), FSOTS_BPGenGraphNode::StaticStruct(), &Request.NewNode, 0, 0);
			}
			bool bCompileValue = true;
			if (Params->TryGetBoolField(TEXT("compile"), bCompileValue))
			{
				Request.bCompile = bCompileValue;
			}
		}

		if (bDryRun)
		{
			FSOTS_BPGenGraphEditResult DryResult;
			DryResult.BlueprintPath = Request.BlueprintAssetPath;
			DryResult.FunctionName = Request.FunctionName;

			if (Request.BlueprintAssetPath.IsEmpty() || Request.FunctionName.IsNone() || Request.ExistingNodeId.IsEmpty())
			{
				DryResult.Errors.Add(TEXT("ReplaceNodePreserveId: blueprint_asset_path, function_name, and existing_node_id are required."));
			}
			else
			{
				const FSOTS_BPGenNodeDescribeResult DescribeResult = USOTS_BPGenInspector::DescribeNodeById(nullptr, Request.BlueprintAssetPath, Request.FunctionName, Request.ExistingNodeId, false);
				if (!DescribeResult.bSuccess)
				{
					DryResult.Errors.Append(DescribeResult.Errors);
					DryResult.Warnings.Append(DescribeResult.Warnings);
				}
				else if (Request.NewNode.SpawnerKey.IsEmpty()
					&& Request.NewNode.NodeType != FName(TEXT("K2Node_Knot"))
					&& Request.NewNode.NodeType != FName(TEXT("K2Node_Select"))
					&& Request.NewNode.NodeType != FName(TEXT("K2Node_DynamicCast")))
				{
					DryResult.Errors.Add(FString::Printf(TEXT("ReplaceNodePreserveId: Unsupported node_type '%s'. Provide a spawner_key."), *Request.NewNode.NodeType.ToString()));
				}
				else
				{
					DryResult.bSuccess = true;
					DryResult.AffectedNodeIds.Add(Request.ExistingNodeId);
					DryResult.Message = FString::Printf(TEXT("Dry run: would replace node '%s' and preserve NodeId."), *Request.ExistingNodeId);
					DryResult.Warnings.Add(TEXT("Dry run: no changes applied."));
				}
			}

			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphEditResult::StaticStruct(), &DryResult, ResultJson.ToSharedRef(), 0, 0);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = DryResult.bSuccess;
			OutResult.Result = ResultJson;
			OutResult.Errors.Append(DryResult.Errors);
			OutResult.Warnings.Append(DryResult.Warnings);
			return;
		}

		const FSOTS_BPGenGraphEditResult EditResult = USOTS_BPGenBuilder::ReplaceNodePreserveId(nullptr, Request);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphEditResult::StaticStruct(), &EditResult, ResultJson.ToSharedRef(), 0, 0);
		OutResult.bOk = EditResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Errors.Append(EditResult.Errors);
		OutResult.Warnings.Append(EditResult.Warnings);
		return;
	}

	if (Action.Equals(TEXT("shutdown"), ESearchCase::IgnoreCase))
	{
		OutResult.bOk = true;
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("status"), TEXT("stopping"));
		OutResult.Result = ResultObj;

		Async(EAsyncExecution::Thread, [Self = AsShared()]
		{
			Self->Stop();
		});
		return;
	}

	OutResult.bOk = false;
	OutResult.Errors.Add(FString::Printf(TEXT("Unknown action '%s'."), *Action));
}

FString FSOTS_BPGenBridgeServer::BuildResponseJson(const FString& Action, const FString& RequestId, const FSOTS_BPGenBridgeDispatchResult& DispatchResult) const
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("ok"), DispatchResult.bOk);
	Response->SetStringField(TEXT("request_id"), RequestId);
	Response->SetStringField(TEXT("action"), Action);

	Response->SetStringField(TEXT("server.plugin"), TEXT("SOTS_BPGen_Bridge"));
	Response->SetStringField(TEXT("server.version"), TEXT("0.1"));
	Response->SetStringField(TEXT("server.protocol_version"), GProtocolVersion);
	Response->SetNumberField(TEXT("server.port"), Port);
	const bool bHasAuthToken = !AuthToken.IsEmpty();
	const bool bHasRateLimit = MaxRequestsPerSecond > 0;
	Response->SetObjectField(TEXT("server.features"), BuildFeatureFlags(true, bHasAuthToken, bHasRateLimit));

	if (!DispatchResult.ErrorCode.IsEmpty())
	{
		Response->SetStringField(TEXT("error_code"), DispatchResult.ErrorCode);
	}

	if (DispatchResult.Result.IsValid())
	{
		Response->SetObjectField(TEXT("result"), DispatchResult.Result);
	}
	else
	{
		Response->SetObjectField(TEXT("result"), MakeShared<FJsonObject>());
	}

	TArray<TSharedPtr<FJsonValue>> ErrorValues;
	for (const FString& Error : DispatchResult.Errors)
	{
		ErrorValues.Add(MakeShared<FJsonValueString>(Error));
	}
	Response->SetArrayField(TEXT("errors"), ErrorValues);

	TArray<TSharedPtr<FJsonValue>> WarningValues;
	for (const FString& Warning : DispatchResult.Warnings)
	{
		WarningValues.Add(MakeShared<FJsonValueString>(Warning));
	}
	Response->SetArrayField(TEXT("warnings"), WarningValues);

	FString ResponseString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseString);
	FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);

	ResponseString.AppendChar('\n');
	return ResponseString;
}

bool FSOTS_BPGenBridgeServer::SendResponseAndClose(FSocket* ClientSocket, const FString& ResponseString) const
{
	if (!ClientSocket)
	{
		return false;
	}

	FTCHARToUTF8 Converter(*ResponseString);
	int32 Sent = 0;
	ClientSocket->Send(reinterpret_cast<const uint8*>(Converter.Get()), Converter.Length(), Sent);
	ClientSocket->Close();

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (SocketSubsystem)
	{
		SocketSubsystem->DestroySocket(ClientSocket);
	}

	return Sent == Converter.Length();
}
