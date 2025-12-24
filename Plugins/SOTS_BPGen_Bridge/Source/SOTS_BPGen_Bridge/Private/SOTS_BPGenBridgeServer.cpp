#include "SOTS_BPGenBridgeServer.h"
#include "SOTS_BPGen_BridgeModule.h"
#include "SOTS_BPGenBuilder.h"
#include "SOTS_BPGenDiscovery.h"
#include "SOTS_BPGenEnsure.h"
#include "SOTS_BPGenInspector.h"
#include "SOTS_BPGenSpawnerRegistry.h"
#include "JsonObjectConverter.h"
#include "Engine/Blueprint.h"
#include "Containers/StringConv.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "ScopedTransaction.h"
#include "Math/UnrealMathUtility.h"
#include "Async/Async.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogSOTS_BPGenBridge, Log, All);

namespace
{
static const FString GProtocolVersion = TEXT("1.0");
static const int32 GSpecVersion = 1;
static const FString GSpecSchema = TEXT("SOTS_BPGen_GraphSpec");

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
	Features->SetBoolField(TEXT("batch"), true);
	Features->SetBoolField(TEXT("sessions"), true);
	Features->SetBoolField(TEXT("cache_controls"), true);
	Features->SetBoolField(TEXT("limits"), true);
	Features->SetBoolField(TEXT("recent_requests"), true);
	Features->SetBoolField(TEXT("server_info"), true);
	Features->SetBoolField(TEXT("spec_schema"), true);
	Features->SetBoolField(TEXT("canonicalize_spec"), true);
	Features->SetBoolField(TEXT("health"), true);
	Features->SetBoolField(TEXT("safety"), true);
	Features->SetBoolField(TEXT("audit"), true);
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

static bool GetBridgeBoolConfig(const TCHAR* Key, bool DefaultValue, const TCHAR* EnvKey = nullptr)
{
	bool bValue = DefaultValue;
	if (GConfig)
	{
		GConfig->GetBool(TEXT("SOTS_BPGen_Bridge"), Key, bValue, GEngineIni);
	}

	if (EnvKey)
	{
		const FString EnvValue = FPlatformMisc::GetEnvironmentVariable(EnvKey);
		if (!EnvValue.IsEmpty())
		{
			bValue = EnvValue.Equals(TEXT("1")) || EnvValue.Equals(TEXT("true"), ESearchCase::IgnoreCase);
		}
	}

	return bValue;
}

static int32 GetBridgeMaxRequestsPerMinute()
{
	int32 Value = 0;
	if (GConfig)
	{
		GConfig->GetInt(TEXT("SOTS_BPGen_Bridge"), TEXT("MaxRequestsPerMinute"), Value, GEngineIni);
	}

	const FString EnvValue = FPlatformMisc::GetEnvironmentVariable(TEXT("SOTS_BPGEN_MAX_RPM"));
	if (!EnvValue.IsEmpty())
	{
		Value = FCString::Atoi(*EnvValue);
	}

	return Value;
}

static double GetBridgeSessionIdleSeconds()
{
	int32 Minutes = 10;
	if (GConfig)
	{
		GConfig->GetInt(TEXT("SOTS_BPGen_Bridge"), TEXT("SessionIdleMinutes"), Minutes, GEngineIni);
	}

	const FString EnvValue = FPlatformMisc::GetEnvironmentVariable(TEXT("SOTS_BPGEN_SESSION_IDLE_MIN"));
	if (!EnvValue.IsEmpty())
	{
		Minutes = FCString::Atoi(*EnvValue);
	}

	if (Minutes < 1)
	{
		Minutes = 1;
	}

	return static_cast<double>(Minutes) * 60.0;
}

static FString NormalizeActionName(const FString& Action)
{
	return Action.ToLower();
}

static void NormalizeActionSet(TSet<FString>& OutSet, const TArray<FString>& Input)
{
	OutSet.Empty();
	for (const FString& Entry : Input)
	{
		const FString Normalized = NormalizeActionName(Entry);
		if (!Normalized.IsEmpty())
		{
			OutSet.Add(Normalized);
		}
	}
}

static bool IsLoopbackAddress(const FString& Address)
{
	FString Trimmed = Address;
	Trimmed.TrimStartAndEndInline();
	if (Trimmed.IsEmpty())
	{
		return false;
	}

	const FString Lower = Trimmed.ToLower();
	if (Lower == TEXT("localhost") || Lower == TEXT("::1") || Lower == TEXT("0:0:0:0:0:0:0:1"))
	{
		return true;
	}

	FIPv4Address Parsed;
	if (FIPv4Address::Parse(Trimmed, Parsed))
	{
		return Parsed.A == 127;
	}

	return false;
}

static int32 GetBridgeLimit(const TCHAR* Key, int32 DefaultValue, const TCHAR* EnvKey = nullptr)
{
	int32 Value = DefaultValue;
	if (GConfig)
	{
		GConfig->GetInt(TEXT("SOTS_BPGen_Bridge"), Key, Value, GEngineIni);
	}

	if (EnvKey)
	{
		const FString EnvValue = FPlatformMisc::GetEnvironmentVariable(EnvKey);
		if (!EnvValue.IsEmpty())
		{
			Value = FCString::Atoi(*EnvValue);
		}
	}

	if (Value < 0)
	{
		Value = 0;
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

static bool IsDangerousActionName(const FString& Action)
{
	return Action.Equals(TEXT("delete_node"), ESearchCase::IgnoreCase)
		|| Action.Equals(TEXT("delete_node_by_id"), ESearchCase::IgnoreCase)
		|| Action.Equals(TEXT("delete_link"), ESearchCase::IgnoreCase)
		|| Action.Equals(TEXT("replace_node"), ESearchCase::IgnoreCase)
		|| Action.Equals(TEXT("save_blueprint"), ESearchCase::IgnoreCase)
		|| Action.Equals(TEXT("batch"), ESearchCase::IgnoreCase)
		|| Action.Equals(TEXT("session_batch"), ESearchCase::IgnoreCase);
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

static TSharedPtr<FJsonObject> SanitizeRequestForAudit(const TSharedPtr<FJsonObject>& RequestObject)
{
	if (!RequestObject.IsValid())
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> Sanitized = MakeShared<FJsonObject>();
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : RequestObject->Values)
	{
		if (Pair.Key.Equals(TEXT("params"), ESearchCase::IgnoreCase))
		{
			TSharedPtr<FJsonObject> ParamsObj = Pair.Value.IsValid() ? Pair.Value->AsObject() : nullptr;
			TSharedPtr<FJsonObject> CleanParams = MakeShared<FJsonObject>();
			if (ParamsObj.IsValid())
			{
				for (const TPair<FString, TSharedPtr<FJsonValue>>& ParamPair : ParamsObj->Values)
				{
					if (ParamPair.Key.Equals(TEXT("auth_token"), ESearchCase::IgnoreCase))
					{
						continue;
					}
					CleanParams->SetField(ParamPair.Key, ParamPair.Value);
				}
			}
			Sanitized->SetObjectField(TEXT("params"), CleanParams);
		}
		else
		{
			Sanitized->SetField(Pair.Key, Pair.Value);
		}
	}

	return Sanitized;
}

static void AppendAssetsFromChangeSummary(const TSharedPtr<FJsonObject>& ChangeSummary, TArray<TSharedPtr<FJsonValue>>& OutAssets)
{
	if (!ChangeSummary.IsValid())
	{
		return;
	}

	FString BlueprintPath;
	if (ChangeSummary->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath) && !BlueprintPath.IsEmpty())
	{
		OutAssets.Add(MakeShared<FJsonValueString>(BlueprintPath));
		return;
	}

	if (ChangeSummary->TryGetStringField(TEXT("BlueprintAssetPath"), BlueprintPath) && !BlueprintPath.IsEmpty())
	{
		OutAssets.Add(MakeShared<FJsonValueString>(BlueprintPath));
	}
}

static void WriteAuditLog(
	const TSharedPtr<FJsonObject>& RequestObject,
	const TSharedPtr<FJsonObject>& ResponseObject,
	const FString& Action,
	const FString& RequestId,
	const FDateTime& StartedUtc,
	const FDateTime& CompletedUtc,
	double RequestMs)
{
	if (!ResponseObject.IsValid())
	{
		return;
	}

	TSharedPtr<IPlugin> BridgePlugin = IPluginManager::Get().FindPlugin(TEXT("SOTS_BPGen_Bridge"));
	if (!BridgePlugin.IsValid())
	{
		return;
	}

	const FString DateFolder = StartedUtc.ToString(TEXT("%Y%m%d"));
	const FString BaseDir = FPaths::Combine(BridgePlugin->GetBaseDir(), TEXT("Saved"), TEXT("BPGenAudit"), DateFolder);
	IFileManager::Get().MakeDirectory(*BaseDir, true);

	const FString SafeRequestId = FPaths::MakeValidFileName(RequestId.IsEmpty() ? TEXT("request") : RequestId);
	const FString SafeAction = FPaths::MakeValidFileName(Action.IsEmpty() ? TEXT("unknown") : Action);
	FString FileName = FString::Printf(TEXT("%s_%s.json"), *SafeRequestId, *SafeAction);
	FString FullPath = FPaths::Combine(BaseDir, FileName);
	if (IFileManager::Get().FileExists(*FullPath))
	{
		const FString Suffix = StartedUtc.ToString(TEXT("%H%M%S"));
		FileName = FString::Printf(TEXT("%s_%s_%s.json"), *SafeRequestId, *SafeAction, *Suffix);
		FullPath = FPaths::Combine(BaseDir, FileName);
	}

	TSharedPtr<FJsonObject> AuditObj = MakeShared<FJsonObject>();
	AuditObj->SetStringField(TEXT("request_id"), RequestId);
	AuditObj->SetStringField(TEXT("action"), Action);
	AuditObj->SetStringField(TEXT("received_utc"), StartedUtc.ToIso8601());
	AuditObj->SetStringField(TEXT("completed_utc"), CompletedUtc.ToIso8601());
	AuditObj->SetNumberField(TEXT("duration_ms"), RequestMs);

	if (RequestObject.IsValid())
	{
		AuditObj->SetObjectField(TEXT("request"), RequestObject);
	}
	AuditObj->SetObjectField(TEXT("response"), ResponseObject);

	TSharedPtr<FJsonObject> ResultObj;
	if (ResponseObject->HasTypedField<EJson::Object>(TEXT("result")))
	{
		ResultObj = ResponseObject->GetObjectField(TEXT("result"));
	}

	TSharedPtr<FJsonObject> ChangeSummaryObj;
	if (ResultObj.IsValid())
	{
		if (ResultObj->HasTypedField<EJson::Object>(TEXT("change_summary")))
		{
			ChangeSummaryObj = ResultObj->GetObjectField(TEXT("change_summary"));
		}
		else if (ResultObj->HasTypedField<EJson::Object>(TEXT("ChangeSummary")))
		{
			ChangeSummaryObj = ResultObj->GetObjectField(TEXT("ChangeSummary"));
		}
	}

	if (ChangeSummaryObj.IsValid())
	{
		AuditObj->SetObjectField(TEXT("change_summary"), ChangeSummaryObj);
	}

	TArray<TSharedPtr<FJsonValue>> AssetsTouched;
	if (ChangeSummaryObj.IsValid())
	{
		AppendAssetsFromChangeSummary(ChangeSummaryObj, AssetsTouched);
	}
	AuditObj->SetArrayField(TEXT("assets_touched"), AssetsTouched);

	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(AuditObj.ToSharedRef(), Writer);
	OutputString.AppendChar('\n');

	FFileHelper::SaveStringToFile(OutputString, *FullPath);
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

static void AddChangeSummaryAlias(const TSharedPtr<FJsonObject>& ResultJson);
static void AddSpecMigrationAliases(const TSharedPtr<FJsonObject>& ResultJson);

static void BuildRecipeResult(const FSOTS_BPGenGraphSpec& GraphSpec, const FSOTS_BPGenApplyResult& ApplyResult, FSOTS_BPGenBridgeDispatchResult& OutResult)
{
	TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> GraphSpecJson = MakeShared<FJsonObject>();
	TSharedPtr<FJsonObject> ApplyJson = MakeShared<FJsonObject>();

	FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphSpec::StaticStruct(), &GraphSpec, GraphSpecJson.ToSharedRef(), 0, 0);
	FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &ApplyResult, ApplyJson.ToSharedRef(), 0, 0);
	AddChangeSummaryAlias(ApplyJson);
	AddSpecMigrationAliases(ApplyJson);
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

static void AddChangeSummaryAlias(const TSharedPtr<FJsonObject>& ResultJson)
{
	if (!ResultJson.IsValid())
	{
		return;
	}

	if (!ResultJson->HasTypedField<EJson::Object>(TEXT("change_summary")) && ResultJson->HasTypedField<EJson::Object>(TEXT("ChangeSummary")))
	{
		ResultJson->SetObjectField(TEXT("change_summary"), ResultJson->GetObjectField(TEXT("ChangeSummary")));
	}
}

static void AddSpecMigrationAliases(const TSharedPtr<FJsonObject>& ResultJson)
{
	if (!ResultJson.IsValid())
	{
		return;
	}

	bool bSpecMigrated = false;
	if (!ResultJson->HasField(TEXT("spec_migrated")) && ResultJson->TryGetBoolField(TEXT("bSpecMigrated"), bSpecMigrated))
	{
		ResultJson->SetBoolField(TEXT("spec_migrated"), bSpecMigrated);
	}

	const TArray<TSharedPtr<FJsonValue>>* MigrationNotes = nullptr;
	if (!ResultJson->HasField(TEXT("migration_notes")) && ResultJson->TryGetArrayField(TEXT("MigrationNotes"), MigrationNotes) && MigrationNotes)
	{
		ResultJson->SetArrayField(TEXT("migration_notes"), *MigrationNotes);
	}

	const TArray<TSharedPtr<FJsonValue>>* RepairSteps = nullptr;
	if (!ResultJson->HasField(TEXT("repair_steps")) && ResultJson->TryGetArrayField(TEXT("RepairSteps"), RepairSteps) && RepairSteps)
	{
		ResultJson->SetArrayField(TEXT("repair_steps"), *RepairSteps);
	}
}
}

FSOTS_BPGenBridgeServer::FSOTS_BPGenBridgeServer()
	: bRunning(false)
	, bStopping(false)
{
}

FSOTS_BPGenBridgeServer::~FSOTS_BPGenBridgeServer()
{
	Stop();
}

bool FSOTS_BPGenBridgeServer::Start(const FString& InBindAddress, int32 InPort, int32 InMaxRequestBytes)
{
	Stop();
	bStopping.Store(false);

	BindAddress = InBindAddress;
	Port = InPort;
	MaxRequestBytes = InMaxRequestBytes;
	AuthToken = GetBridgeAuthToken();
	bAllowNonLoopbackBind = GetBridgeBoolConfig(TEXT("bAllowNonLoopbackBind"), false, TEXT("SOTS_BPGEN_ALLOW_NON_LOOPBACK"));
	bSafeMode = GetBridgeBoolConfig(TEXT("bSafeMode"), false, TEXT("SOTS_BPGEN_SAFE_MODE"));
	MaxRequestsPerSecond = GetBridgeMaxRequestsPerSecond();
	if (MaxRequestsPerSecond < 0)
	{
		MaxRequestsPerSecond = 0;
	}
	MaxRequestsPerMinute = GetBridgeMaxRequestsPerMinute();
	if (MaxRequestsPerMinute < 0)
	{
		MaxRequestsPerMinute = 0;
	}
	RateWindowStartSeconds = 0.0;
	RequestsInWindow = 0;
	MinuteWindowStartSeconds = 0.0;
	RequestsInMinute = 0;
	TotalRequests = 0;
	MaxDiscoveryResults = GetBridgeLimit(TEXT("MaxDiscoveryResults"), 200, TEXT("SOTS_BPGEN_MAX_DISCOVERY"));
	MaxPinHarvestNodes = GetBridgeLimit(TEXT("MaxPinHarvestNodes"), 200, TEXT("SOTS_BPGEN_MAX_PIN_HARVEST"));
	MaxAutoFixSteps = GetBridgeLimit(TEXT("MaxAutoFixSteps"), 5, TEXT("SOTS_BPGEN_MAX_AUTOFIX"));
	MaxRecentRequests = GetBridgeLimit(TEXT("MaxRecentRequests"), 50, TEXT("SOTS_BPGEN_MAX_RECENT"));
	SessionIdleSeconds = GetBridgeSessionIdleSeconds();
	LastStartError.Reset();
	LastStartErrorCode.Reset();
	RecentRequests.Reset();
	Sessions.Reset();
	AllowedActions.Reset();
	DeniedActions.Reset();

	if (GConfig)
	{
		TArray<FString> AllowedList;
		TArray<FString> DeniedList;
		GConfig->GetArray(TEXT("SOTS_BPGen_Bridge"), TEXT("AllowedActions"), AllowedList, GEngineIni);
		GConfig->GetArray(TEXT("SOTS_BPGen_Bridge"), TEXT("DeniedActions"), DeniedList, GEngineIni);
		NormalizeActionSet(AllowedActions, AllowedList);
		NormalizeActionSet(DeniedActions, DeniedList);
	}

	if (!bAllowNonLoopbackBind && !IsLoopbackAddress(BindAddress))
	{
		LastStartErrorCode = TEXT("ERR_NON_LOOPBACK_BIND_DISALLOWED");
		LastStartError = FString::Printf(TEXT("Non-loopback bind disallowed (address: %s)."), *BindAddress);
		UE_LOG(LogSOTS_BPGenBridge, Error, TEXT("%s"), *LastStartError);
		return false;
	}

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		LastStartErrorCode = TEXT("ERR_NO_SOCKET_SUBSYSTEM");
		LastStartError = TEXT("Socket subsystem unavailable.");
		return false;
	}

	FSocket* RawSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("BPGenBridgeListen"), false);
	ListenSocket = TSharedPtr<FSocket>(RawSocket, [SocketSubsystem](FSocket* Socket)
	{
		if (!Socket)
		{
			return;
		}

		Socket->Close();
		if (SocketSubsystem)
		{
			SocketSubsystem->DestroySocket(Socket);
		}
	});
	if (ListenSocket.IsValid())
	{
		ListenSocket->SetReuseAddr(true);
		ListenSocket->SetNonBlocking(true);
	}
	if (!ListenSocket.IsValid())
	{
		LastStartErrorCode = TEXT("ERR_CREATE_SOCKET_FAILED");
		LastStartError = TEXT("Failed to create listen socket.");
		return false;
	}

	FIPv4Address ParsedAddress;
	if (!FIPv4Address::Parse(BindAddress, ParsedAddress))
	{
		LastStartErrorCode = TEXT("ERR_PARSE_BIND_ADDRESS");
		LastStartError = FString::Printf(TEXT("Failed to parse bind address: %s"), *BindAddress);
		Stop();
		return false;
	}

	TSharedRef<FInternetAddr> ListenAddr = SocketSubsystem->CreateInternetAddr();
	ListenAddr->SetIp(ParsedAddress.Value);
	ListenAddr->SetPort(Port);

	const int32 ListenBacklog = 5; // match VibeUE listener backlog
	if (!ListenSocket->Bind(*ListenAddr) || !ListenSocket->Listen(ListenBacklog))
	{
		const ESocketErrors LastErr = SocketSubsystem->GetLastErrorCode();
		LastStartErrorCode = LexToString(static_cast<int32>(LastErr));
		LastStartError = FString::Printf(TEXT("Bind/Listen failed for %s:%d (err=%s)"), *BindAddress, Port, *LastStartErrorCode);
		UE_LOG(LogSOTS_BPGenBridge, Error, TEXT("%s"), *LastStartError);
		Stop();
		return false;
	}

	bRunning = true;
	ServerStartSeconds = FPlatformTime::Seconds();
	ServerStartUtc = FDateTime::UtcNow();
	TWeakPtr<FSOTS_BPGenBridgeServer> WeakSelf = AsWeak();
	AcceptTask = Async(EAsyncExecution::Thread, [WeakSelf]()
	{
		if (TSharedPtr<FSOTS_BPGenBridgeServer> Pinned = WeakSelf.Pin())
		{
			Pinned->AcceptLoop();
		}
	});

	return true;
}

void FSOTS_BPGenBridgeServer::Stop()
{
	if (bStopping.Exchange(true))
	{
		return;
	}

	bRunning = false;

	// Move the task so we don't touch shared state while waiting.
	TFuture<void> LocalAcceptTask = MoveTemp(AcceptTask);
	AcceptTask = TFuture<void>();

	TSharedPtr<FSocket> LocalSocket;
	if (ListenSocket.IsValid())
	{
		ListenSocket->Close();
		LocalSocket = MoveTemp(ListenSocket);
		ListenSocket.Reset();
	}

	if (LocalAcceptTask.IsValid())
	{
		LocalAcceptTask.Wait();
	}

	LocalSocket.Reset();
}

bool FSOTS_BPGenBridgeServer::IsRunning() const
{
	return bRunning;
}

void FSOTS_BPGenBridgeServer::GetServerInfoForUI(TSharedPtr<FJsonObject>& OutInfo) const
{
	TSharedPtr<FJsonObject> Info = MakeShared<FJsonObject>();
	Info->SetBoolField(TEXT("running"), bRunning);
	Info->SetStringField(TEXT("bind_address"), BindAddress);
	Info->SetNumberField(TEXT("port"), Port);
	Info->SetBoolField(TEXT("safe_mode"), bSafeMode);
	Info->SetBoolField(TEXT("allow_non_loopback_bind"), bAllowNonLoopbackBind);
	Info->SetNumberField(TEXT("max_request_bytes"), MaxRequestBytes);
	Info->SetNumberField(TEXT("max_requests_per_second"), MaxRequestsPerSecond);
	Info->SetNumberField(TEXT("max_requests_per_minute"), MaxRequestsPerMinute);
	Info->SetNumberField(TEXT("uptime_seconds"), bRunning ? (FPlatformTime::Seconds() - ServerStartSeconds) : 0.0);
	Info->SetStringField(TEXT("started_utc"), ServerStartUtc.GetTicks() > 0 ? ServerStartUtc.ToIso8601() : FString());
	Info->SetStringField(TEXT("last_start_error"), LastStartError);
	Info->SetStringField(TEXT("last_start_error_code"), LastStartErrorCode);
	Info->SetNumberField(TEXT("total_requests"), TotalRequests);

	TSharedPtr<FJsonObject> Features = BuildFeatureFlags(true, !AuthToken.IsEmpty(), (MaxRequestsPerSecond > 0 || MaxRequestsPerMinute > 0));
	Info->SetObjectField(TEXT("features"), Features);

	OutInfo = Info;
}

void FSOTS_BPGenBridgeServer::GetRecentRequestsForUI(TArray<TSharedPtr<FJsonObject>>& OutRequests) const
{
	FScopeLock Lock(&RecentRequestsMutex);
	for (const FRecentRequestSummary& Entry : RecentRequests)
	{
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		Obj->SetStringField(TEXT("request_id"), Entry.RequestId);
		Obj->SetStringField(TEXT("action"), Entry.Action);
		Obj->SetBoolField(TEXT("ok"), Entry.bOk);
		Obj->SetStringField(TEXT("error_code"), Entry.ErrorCode);
		Obj->SetNumberField(TEXT("request_ms"), Entry.RequestMs);
		Obj->SetStringField(TEXT("timestamp"), Entry.Timestamp);
		OutRequests.Add(Obj);
	}
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

		bool bPending = false;
		if (!ListenSocket->HasPendingConnection(bPending) || !bPending)
		{
			FPlatformProcess::Sleep(0.1f);
			continue;
		}

		TSharedRef<FInternetAddr> ClientAddr = SocketSubsystem->CreateInternetAddr();
		FSocket* Accepted = ListenSocket->Accept(*ClientAddr, TEXT("BPGenBridgeClient"));
		if (!Accepted)
		{
			FPlatformProcess::Sleep(0.1f);
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
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	TSharedPtr<FSocket> Client(ClientSocket, [SocketSubsystem](FSocket* Socket)
	{
		if (!Socket)
		{
			return;
		}

		Socket->Close();
		if (SocketSubsystem)
		{
			SocketSubsystem->DestroySocket(Socket);
		}
	});
	if (Client.IsValid())
	{
		Client->SetNoDelay(true);
		const int32 SocketBufferSize = 65536;
		int32 OutSize = 0;
		Client->SetSendBufferSize(SocketBufferSize, OutSize);
		Client->SetReceiveBufferSize(SocketBufferSize, OutSize);
		Client->SetNonBlocking(false);
	}
	if (!Client.IsValid())
	{
		return;
	}

	auto ProcessRequest = [&](const FString& RequestString)
	{
		const double RequestStartSeconds = FPlatformTime::Seconds();
		const FDateTime RequestStartUtc = FDateTime::UtcNow();

		auto WriteAudit = [&](const TSharedPtr<FJsonObject>& RequestObj, const FString& ResponseString, const FString& ActionName, const FString& RequestIdValue, double RequestMs)
		{
			TSharedPtr<FJsonObject> ResponseObject;
			TSharedRef<TJsonReader<>> ResponseReader = TJsonReaderFactory<>::Create(ResponseString);
			if (!FJsonSerializer::Deserialize(ResponseReader, ResponseObject) || !ResponseObject.IsValid())
			{
				return;
			}

			const TSharedPtr<FJsonObject> SanitizedRequest = SanitizeRequestForAudit(RequestObj);
			WriteAuditLog(SanitizedRequest, ResponseObject, ActionName, RequestIdValue, RequestStartUtc, FDateTime::UtcNow(), RequestMs);
		};

		if (RequestString.Len() > MaxRequestBytes)
		{
			FSOTS_BPGenBridgeDispatchResult ErrorResult;
			ErrorResult.bOk = false;
			ErrorResult.ErrorCode = TEXT("ERR_REQUEST_TOO_LARGE");
			ErrorResult.Errors.Add(FString::Printf(TEXT("Request exceeded max_bytes limit (%d)."), MaxRequestBytes));
			ErrorResult.RequestMs = (FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0;
			const FString Response = BuildResponseJson(TEXT("unknown"), TEXT(""), ErrorResult);
			SendResponseAndClose(Client.Get(), Response);
			WriteAudit(nullptr, Response, TEXT("unknown"), TEXT(""), ErrorResult.RequestMs);
			return;
		}

		TSharedPtr<FJsonObject> RequestObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RequestString);
		if (!FJsonSerializer::Deserialize(Reader, RequestObject) || !RequestObject.IsValid())
		{
			FSOTS_BPGenBridgeDispatchResult ErrorResult;
			ErrorResult.bOk = false;
			ErrorResult.Errors.Add(TEXT("Failed to parse JSON request."));
			ErrorResult.RequestMs = (FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0;
			const FString Response = BuildResponseJson(TEXT("unknown"), TEXT(""), ErrorResult);
			SendResponseAndClose(Client.Get(), Response);
			WriteAudit(nullptr, Response, TEXT("unknown"), TEXT(""), ErrorResult.RequestMs);
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

		PruneExpiredSessions();

		FSOTS_BPGenBridgeDispatchResult DispatchResult;
		if (!CheckRateLimit(DispatchResult))
		{
			DispatchResult.RequestMs = (FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0;
			const FString Response = BuildResponseJson(Action, RequestId, DispatchResult);
			SendResponseAndClose(Client.Get(), Response);
			WriteAudit(RequestObject, Response, Action, RequestId, DispatchResult.RequestMs);
			return;
		}

		if (!CheckAuthToken(Params, DispatchResult))
		{
			DispatchResult.RequestMs = (FPlatformTime::Seconds() - RequestStartSeconds) * 1000.0;
			const FString Response = BuildResponseJson(Action, RequestId, DispatchResult);
			SendResponseAndClose(Client.Get(), Response);
			WriteAudit(RequestObject, Response, Action, RequestId, DispatchResult.RequestMs);
			return;
		}

		const double DispatchStartSeconds = FPlatformTime::Seconds();
		if (!DispatchOnGameThread(Action, RequestId, Params, DispatchResult))
		{
			DispatchResult.bOk = false;
			DispatchResult.Errors.Add(TEXT("Request timed out while executing on game thread."));
		}
		const double DispatchEndSeconds = FPlatformTime::Seconds();
		DispatchResult.DispatchMs = (DispatchEndSeconds - DispatchStartSeconds) * 1000.0;
		DispatchResult.RequestMs = (DispatchEndSeconds - RequestStartSeconds) * 1000.0;

		const FString Response = BuildResponseJson(Action, RequestId, DispatchResult);
		SendResponseAndClose(Client.Get(), Response);
		WriteAudit(RequestObject, Response, Action, RequestId, DispatchResult.RequestMs);

		AddRecentRequestSummary(RequestId, Action, DispatchResult);
		const int32 ErrorCodeCount = DispatchResult.ErrorCode.IsEmpty() ? 0 : 1;
		UE_LOG(LogSOTS_BPGenBridge, Log, TEXT("BPGen request id=%s action=%s ok=%s error_code=%s error_codes=%d errors=%d warnings=%d ms=%.2f"),
			*RequestId,
			*Action,
			DispatchResult.bOk ? TEXT("true") : TEXT("false"),
			DispatchResult.ErrorCode.IsEmpty() ? TEXT("none") : *DispatchResult.ErrorCode,
			ErrorCodeCount,
			DispatchResult.Errors.Num(),
			DispatchResult.Warnings.Num(),
			DispatchResult.RequestMs);
	};

	TArray<uint8> RecvBuffer;
	RecvBuffer.SetNumUninitialized(4096);
	FString MessageBuffer;
	bool bLoggedHexSample = false;

	while (bRunning && Client->GetConnectionState() == SCS_Connected)
	{
		const bool bIsConnected = Client->GetConnectionState() == SCS_Connected;
		uint32 PendingDataSize = 0;
		const bool bHasPending = Client->HasPendingData(PendingDataSize);
		UE_LOG(LogSOTS_BPGenBridge, VeryVerbose, TEXT("BPGen recv state connected=%s pending=%s size=%u"), bIsConnected ? TEXT("true") : TEXT("false"), bHasPending ? TEXT("true") : TEXT("false"), PendingDataSize);

		int32 BytesRead = 0;
		const bool bReadSuccess = Client->Recv(RecvBuffer.GetData(), RecvBuffer.Num(), BytesRead);
		if (!bReadSuccess)
		{
			const ESocketErrors LastError = SocketSubsystem ? SocketSubsystem->GetLastErrorCode() : SE_NO_ERROR;
			if (LastError == SE_EWOULDBLOCK || LastError == SE_EINTR)
			{
				UE_LOG(LogSOTS_BPGenBridge, Verbose, TEXT("BPGen recv would block/interrupt; sleeping"));
				FPlatformProcess::Sleep(0.01f);
				continue;
			}

			UE_LOG(LogSOTS_BPGenBridge, Warning, TEXT("BPGen recv failed; error=%d"), static_cast<int32>(LastError));
			break;
		}

		if (BytesRead <= 0)
		{
			UE_LOG(LogSOTS_BPGenBridge, Verbose, TEXT("BPGen recv zero/empty; sleeping"));
			FPlatformProcess::Sleep(0.001f);
			continue;
		}

		if (!bLoggedHexSample)
		{
			const int32 SampleLen = FMath::Min(BytesRead, 50);
			FString HexSample;
			HexSample.Reserve(SampleLen * 2);
			for (int32 Index = 0; Index < SampleLen; ++Index)
			{
				HexSample += FString::Printf(TEXT("%02x"), RecvBuffer[Index]);
			}
			UE_LOG(LogSOTS_BPGenBridge, Verbose, TEXT("BPGen recv sample (%d bytes): %s%s"), SampleLen, *HexSample, BytesRead > SampleLen ? TEXT("...") : TEXT(""));
			bLoggedHexSample = true;
		}

		FUTF8ToTCHAR Converter(reinterpret_cast<const ANSICHAR*>(RecvBuffer.GetData()), BytesRead);
		MessageBuffer.Append(FString(Converter.Length(), Converter.Get()));

		if (MessageBuffer.Len() > MaxRequestBytes)
		{
			FSOTS_BPGenBridgeDispatchResult ErrorResult;
			ErrorResult.bOk = false;
			ErrorResult.ErrorCode = TEXT("ERR_REQUEST_TOO_LARGE");
			ErrorResult.Errors.Add(FString::Printf(TEXT("Request exceeded max_bytes limit (%d)."), MaxRequestBytes));
			ErrorResult.RequestMs = 0.0;
			const FString Response = BuildResponseJson(TEXT("unknown"), TEXT(""), ErrorResult);
			SendResponseAndClose(Client.Get(), Response);
			UE_LOG(LogSOTS_BPGenBridge, Warning, TEXT("BPGen request aborted: buffer exceeded max bytes"));
			return;
		}

		TArray<FString> Messages;
		MessageBuffer.ParseIntoArray(Messages, TEXT("\n"), false);
		if (Messages.Num() > 1)
		{
			UE_LOG(LogSOTS_BPGenBridge, Verbose, TEXT("BPGen buffered %d message(s) this pass"), Messages.Num() - 1);
		}

		const int32 CompleteCount = FMath::Max(Messages.Num() - 1, 0);
		for (int32 Index = 0; Index < CompleteCount; ++Index)
		{
			FString Line = Messages[Index];
			Line.ReplaceInline(TEXT("\r"), TEXT(""));
			if (Line.IsEmpty())
			{
				continue;
			}

			ProcessRequest(Line);
		}

		MessageBuffer = Messages.Num() > 0 ? Messages.Last() : MessageBuffer;
		FPlatformProcess::Sleep(0.005f);
	}

	UE_LOG(LogSOTS_BPGenBridge, Verbose, TEXT("BPGen connection handler exiting"));
}

bool FSOTS_BPGenBridgeServer::CheckRateLimit(FSOTS_BPGenBridgeDispatchResult& OutResult)
{
	++TotalRequests;

	const double NowSeconds = FPlatformTime::Seconds();
	if (MaxRequestsPerSecond > 0)
	{
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
	}

	if (MaxRequestsPerMinute > 0)
	{
		if (MinuteWindowStartSeconds <= 0.0 || (NowSeconds - MinuteWindowStartSeconds) >= 60.0)
		{
			MinuteWindowStartSeconds = NowSeconds;
			RequestsInMinute = 0;
		}

		++RequestsInMinute;
		if (RequestsInMinute > MaxRequestsPerMinute)
		{
			OutResult.bOk = false;
			OutResult.ErrorCode = TEXT("ERR_RATE_LIMIT");
			OutResult.Errors.Add(FString::Printf(TEXT("Rate limit exceeded (%d requests/minute)."), MaxRequestsPerMinute));
			return false;
		}
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

bool FSOTS_BPGenBridgeServer::CheckActionAllowed(const FString& Action, FSOTS_BPGenBridgeDispatchResult& OutResult) const
{
	const FString Normalized = NormalizeActionName(Action);
	if (DeniedActions.Contains(Normalized))
	{
		OutResult.bOk = false;
		OutResult.ErrorCode = TEXT("ERR_ACTION_DENIED");
		OutResult.Errors.Add(TEXT("Action is denied by DeniedActions configuration."));
		return false;
	}

	if (AllowedActions.Num() > 0 && !AllowedActions.Contains(Normalized))
	{
		OutResult.bOk = false;
		OutResult.ErrorCode = TEXT("ERR_ACTION_NOT_ALLOWED");
		OutResult.Errors.Add(TEXT("Action is not present in AllowedActions configuration."));
		return false;
	}

	return true;
}

bool FSOTS_BPGenBridgeServer::CheckDangerousGate(const FString& Action, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult) const
{
	if (!IsDangerousActionName(Action))
	{
		return true;
	}

	bool bIsDangerous = true;
	if (Action.Equals(TEXT("batch"), ESearchCase::IgnoreCase) || Action.Equals(TEXT("session_batch"), ESearchCase::IgnoreCase))
	{
		bool bAtomic = true;
		if (Params.IsValid())
		{
			Params->TryGetBoolField(TEXT("atomic"), bAtomic);
		}
		bIsDangerous = bAtomic;
	}

	if (!bIsDangerous)
	{
		return true;
	}

	if (bSafeMode)
	{
		OutResult.bOk = false;
		OutResult.ErrorCode = TEXT("ERR_SAFE_MODE_ACTIVE");
		OutResult.Errors.Add(TEXT("Safe mode is enabled; dangerous operations are blocked."));
		return false;
	}

	bool bDangerousOk = false;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("dangerous_ok"), bDangerousOk);
	}

	if (!bDangerousOk)
	{
		OutResult.bOk = false;
		OutResult.ErrorCode = TEXT("ERR_DANGEROUS_OP_REQUIRES_OPT_IN");
		OutResult.Errors.Add(TEXT("Set params.dangerous_ok=true to proceed with dangerous operations."));
		return false;
	}

	return true;
}

void FSOTS_BPGenBridgeServer::AddRecentRequestSummary(const FString& RequestId, const FString& Action, const FSOTS_BPGenBridgeDispatchResult& DispatchResult)
{
	FScopeLock Lock(&RecentRequestsMutex);

	FRecentRequestSummary Summary;
	Summary.RequestId = RequestId;
	Summary.Action = Action;
	Summary.bOk = DispatchResult.bOk;
	Summary.ErrorCode = DispatchResult.ErrorCode;
	Summary.RequestMs = DispatchResult.RequestMs;
	Summary.Timestamp = FDateTime::UtcNow().ToIso8601();

	RecentRequests.Add(Summary);
	if (RecentRequests.Num() > MaxRecentRequests && MaxRecentRequests > 0)
	{
		const int32 Overflow = RecentRequests.Num() - MaxRecentRequests;
		RecentRequests.RemoveAt(0, Overflow);
	}
}

void FSOTS_BPGenBridgeServer::PruneExpiredSessions()
{
	if (Sessions.Num() == 0)
	{
		return;
	}

	const double NowSeconds = FPlatformTime::Seconds();
	TArray<FString> ExpiredKeys;
	for (const TPair<FString, FSessionState>& Pair : Sessions)
	{
		if (NowSeconds - Pair.Value.LastAccessSeconds > SessionIdleSeconds)
		{
			ExpiredKeys.Add(Pair.Key);
		}
	}

	for (const FString& Key : ExpiredKeys)
	{
		Sessions.Remove(Key);
	}
}

bool FSOTS_BPGenBridgeServer::DispatchOnGameThread(const FString& Action, const FString& RequestId, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult)
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

void FSOTS_BPGenBridgeServer::RouteBpgenAction(const FString& Action, const TSharedPtr<FJsonObject>& Params, FSOTS_BPGenBridgeDispatchResult& OutResult)
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

	if (!CheckActionAllowed(Action, OutResult))
	{
		return;
	}

	if (!CheckDangerousGate(Action, Params, OutResult))
	{
		return;
	}

	bool bDryRun = false;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("dry_run"), bDryRun);
	}

	if (Action.Equals(TEXT("server_info"), ESearchCase::IgnoreCase))
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("bind_address"), BindAddress);
		ResultObj->SetNumberField(TEXT("port"), Port);
		ResultObj->SetStringField(TEXT("protocol_version"), GProtocolVersion);
		ResultObj->SetBoolField(TEXT("running"), bRunning);
		ResultObj->SetBoolField(TEXT("safe_mode"), bSafeMode);
		ResultObj->SetBoolField(TEXT("allow_non_loopback_bind"), bAllowNonLoopbackBind);
		ResultObj->SetNumberField(TEXT("max_request_bytes"), MaxRequestBytes);
		ResultObj->SetNumberField(TEXT("max_requests_per_second"), MaxRequestsPerSecond);
		ResultObj->SetNumberField(TEXT("max_requests_per_minute"), MaxRequestsPerMinute);
		ResultObj->SetNumberField(TEXT("uptime_seconds"), bRunning ? (FPlatformTime::Seconds() - ServerStartSeconds) : 0.0);
		ResultObj->SetStringField(TEXT("started_utc"), ServerStartUtc.GetTicks() > 0 ? ServerStartUtc.ToIso8601() : FString());
		ResultObj->SetStringField(TEXT("last_start_error"), LastStartError);
		ResultObj->SetStringField(TEXT("last_start_error_code"), LastStartErrorCode);

		TArray<TSharedPtr<FJsonValue>> AllowedValues;
		for (const FString& Entry : AllowedActions)
		{
			AllowedValues.Add(MakeShared<FJsonValueString>(Entry));
		}
		TArray<TSharedPtr<FJsonValue>> DeniedValues;
		for (const FString& Entry : DeniedActions)
		{
			DeniedValues.Add(MakeShared<FJsonValueString>(Entry));
		}
		ResultObj->SetArrayField(TEXT("allowed_actions"), AllowedValues);
		ResultObj->SetArrayField(TEXT("denied_actions"), DeniedValues);
		ResultObj->SetObjectField(TEXT("features"), BuildFeatureFlags(true, !AuthToken.IsEmpty(), (MaxRequestsPerSecond > 0 || MaxRequestsPerMinute > 0)));

		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("get_spec_schema"), ESearchCase::IgnoreCase))
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("spec_schema"), GSpecSchema);
		ResultObj->SetNumberField(TEXT("spec_version"), GSpecVersion);

		TArray<TSharedPtr<FJsonValue>> Supported;
		Supported.Add(MakeShared<FJsonValueNumber>(GSpecVersion));
		ResultObj->SetArrayField(TEXT("supported_versions"), Supported);

		TArray<TSharedPtr<FJsonValue>> Notes;
		Notes.Add(MakeShared<FJsonValueString>(TEXT("GraphSpec schema is forward-compatible; use migrations for drift.")));
		ResultObj->SetArrayField(TEXT("notes"), Notes);

		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("canonicalize_spec"), ESearchCase::IgnoreCase))
	{
		if (!Params.IsValid() || !Params->HasTypedField<EJson::Object>(TEXT("graph_spec")))
		{
			OutResult.bOk = false;
			OutResult.ErrorCode = TEXT("ERR_INVALID_PARAMS");
			OutResult.Errors.Add(TEXT("canonicalize_spec requires params.graph_spec."));
			return;
		}

		FSOTS_BPGenGraphSpec GraphSpec;
		const TSharedPtr<FJsonObject> GraphObj = Params->GetObjectField(TEXT("graph_spec"));
		FJsonObjectConverter::JsonObjectToUStruct(GraphObj.ToSharedRef(), FSOTS_BPGenGraphSpec::StaticStruct(), &GraphSpec, 0, 0);

		FSOTS_BPGenSpecCanonicalizeOptions Options;
		if (Params->HasTypedField<EJson::Object>(TEXT("options")))
		{
			const TSharedPtr<FJsonObject> OptionsObj = Params->GetObjectField(TEXT("options"));
			FJsonObjectConverter::JsonObjectToUStruct(OptionsObj.ToSharedRef(), FSOTS_BPGenSpecCanonicalizeOptions::StaticStruct(), &Options, 0, 0);
		}

		const FSOTS_BPGenCanonicalizeResult CanonResult = USOTS_BPGenBuilder::CanonicalizeGraphSpec(GraphSpec, Options);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		TSharedPtr<FJsonObject> CanonicalSpecJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenGraphSpec::StaticStruct(), &CanonResult.CanonicalSpec, CanonicalSpecJson.ToSharedRef(), 0, 0);
		CanonicalSpecJson->SetNumberField(TEXT("spec_version"), CanonResult.CanonicalSpec.SpecVersion);
		CanonicalSpecJson->SetStringField(TEXT("spec_schema"), CanonResult.CanonicalSpec.SpecSchema);
		ResultObj->SetObjectField(TEXT("canonical_spec"), CanonicalSpecJson);

		TArray<TSharedPtr<FJsonValue>> DiffNotes;
		for (const FString& Note : CanonResult.DiffNotes)
		{
			DiffNotes.Add(MakeShared<FJsonValueString>(Note));
		}
		ResultObj->SetArrayField(TEXT("diff_notes"), DiffNotes);

		TArray<TSharedPtr<FJsonValue>> MigrationNotes;
		for (const FString& Note : CanonResult.MigrationNotes)
		{
			MigrationNotes.Add(MakeShared<FJsonValueString>(Note));
		}
		ResultObj->SetArrayField(TEXT("migration_notes"), MigrationNotes);
		ResultObj->SetBoolField(TEXT("spec_migrated"), CanonResult.bSpecMigrated);

		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("health"), ESearchCase::IgnoreCase))
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("running"), bRunning);
		ResultObj->SetBoolField(TEXT("safe_mode"), bSafeMode);
		ResultObj->SetNumberField(TEXT("requests_in_second_window"), RequestsInWindow);
		ResultObj->SetNumberField(TEXT("requests_in_minute_window"), RequestsInMinute);
		ResultObj->SetNumberField(TEXT("total_requests"), TotalRequests);
		ResultObj->SetNumberField(TEXT("max_requests_per_second"), MaxRequestsPerSecond);
		ResultObj->SetNumberField(TEXT("max_requests_per_minute"), MaxRequestsPerMinute);
		ResultObj->SetNumberField(TEXT("max_request_bytes"), MaxRequestBytes);
		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("set_safe_mode"), ESearchCase::IgnoreCase))
	{
		bool bEnable = false;
		if (!Params.IsValid() || !Params->TryGetBoolField(TEXT("enabled"), bEnable))
		{
			OutResult.bOk = false;
			OutResult.ErrorCode = TEXT("ERR_INVALID_PARAMS");
			OutResult.Errors.Add(TEXT("set_safe_mode requires params.enabled."));
			return;
		}

		bSafeMode = bEnable;
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("safe_mode"), bSafeMode);
		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("emergency_stop"), ESearchCase::IgnoreCase))
	{
		bSafeMode = true;
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("stopped"), true);
		ResultObj->SetBoolField(TEXT("safe_mode"), true);
		OutResult.bOk = true;
		OutResult.Result = ResultObj;

		Async(EAsyncExecution::Thread, [WeakSelf = AsWeak()]
		{
			if (TSharedPtr<FSOTS_BPGenBridgeServer> Self = WeakSelf.Pin())
			{
				Self->Stop();
			}
		});
		return;
	}

	if (Action.Equals(TEXT("begin_session"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenBridgeServer::FSessionState Session;
		Session.SessionId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
		Session.LastAccessSeconds = FPlatformTime::Seconds();
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), Session.LastBlueprintPath);
		}
		Sessions.Add(Session.SessionId, Session);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("session_id"), Session.SessionId);
		ResultObj->SetNumberField(TEXT("idle_seconds"), SessionIdleSeconds);
		ResultObj->SetStringField(TEXT("started_utc"), FDateTime::UtcNow().ToIso8601());
		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("end_session"), ESearchCase::IgnoreCase))
	{
		FString SessionId;
		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("session_id"), SessionId);
		}

		if (SessionId.IsEmpty())
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("Missing session_id for end_session."));
			return;
		}

		if (Sessions.Remove(SessionId) == 0)
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("Session not found."));
			return;
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("session_id"), SessionId);
		ResultObj->SetStringField(TEXT("status"), TEXT("ended"));
		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("batch"), ESearchCase::IgnoreCase) || Action.Equals(TEXT("session_batch"), ESearchCase::IgnoreCase))
	{
		const bool bSessionBatch = Action.Equals(TEXT("session_batch"), ESearchCase::IgnoreCase);
		FString SessionId;
		if (bSessionBatch && Params.IsValid())
		{
			Params->TryGetStringField(TEXT("session_id"), SessionId);
		}

		FSessionState* Session = nullptr;
		if (bSessionBatch)
		{
			if (SessionId.IsEmpty())
			{
				OutResult.bOk = false;
				OutResult.Errors.Add(TEXT("Missing session_id for session_batch."));
				return;
			}

			Session = Sessions.Find(SessionId);
			if (!Session)
			{
				OutResult.bOk = false;
				OutResult.Errors.Add(TEXT("Session not found."));
				return;
			}

			Session->LastAccessSeconds = FPlatformTime::Seconds();
		}

		FString BatchId;
		bool bAtomic = true;
		bool bStopOnError = true;
		TArray<TSharedPtr<FJsonValue>> CommandValues;
		const TArray<TSharedPtr<FJsonValue>>* CommandValuesPtr = nullptr;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("batch_id"), BatchId);
			Params->TryGetBoolField(TEXT("atomic"), bAtomic);
			Params->TryGetBoolField(TEXT("stop_on_error"), bStopOnError);
			const FStringView CommandFieldName(TEXT("commands"));
			if (Params->TryGetArrayField(CommandFieldName, CommandValuesPtr) && CommandValuesPtr)
			{
				CommandValues = *CommandValuesPtr;
			}
		}

		if (CommandValues.Num() == 0)
		{
			OutResult.bOk = false;
			OutResult.Errors.Add(TEXT("Batch requires a non-empty commands array."));
			return;
		}

		if (BatchId.IsEmpty())
		{
			BatchId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);
		}

		FScopeLock BatchLock(&BatchMutex);
		TUniquePtr<FScopedTransaction> Transaction;
		if (bAtomic)
		{
			Transaction = MakeUnique<FScopedTransaction>(NSLOCTEXT("SOTS_BPGenBridge", "BPGenBatch", "SOTS BPGen: Batch"));
		}

		const double BatchStartSeconds = FPlatformTime::Seconds();
		TArray<TSharedPtr<FJsonValue>> StepValues;
		TMap<FString, int32> ErrorCodeCounts;
		int32 OkSteps = 0;
		int32 FailedSteps = 0;

		for (int32 Index = 0; Index < CommandValues.Num(); ++Index)
		{
			const TSharedPtr<FJsonObject> CommandObj = CommandValues[Index].IsValid() ? CommandValues[Index]->AsObject() : nullptr;
			FSOTS_BPGenBridgeDispatchResult StepResult;
			FString StepAction;
			TSharedPtr<FJsonObject> StepParams = MakeShared<FJsonObject>();

			if (CommandObj.IsValid())
			{
				CommandObj->TryGetStringField(TEXT("action"), StepAction);
				if (CommandObj->HasTypedField<EJson::Object>(TEXT("params")))
				{
					StepParams = CommandObj->GetObjectField(TEXT("params"));
				}
			}

			if (StepAction.IsEmpty())
			{
				StepResult.bOk = false;
				StepResult.Errors.Add(TEXT("Batch command missing action."));
			}
			else if (StepAction.Equals(TEXT("batch"), ESearchCase::IgnoreCase) || StepAction.Equals(TEXT("session_batch"), ESearchCase::IgnoreCase))
			{
				StepResult.bOk = false;
				StepResult.Errors.Add(TEXT("Nested batch commands are not supported."));
			}
			else
			{
				const double StepStartSeconds = FPlatformTime::Seconds();
				RouteBpgenAction(StepAction, StepParams, StepResult);
				const double StepEndSeconds = FPlatformTime::Seconds();
				StepResult.DispatchMs = (StepEndSeconds - StepStartSeconds) * 1000.0;
			}

			UE_LOG(LogSOTS_BPGenBridge, Verbose, TEXT("BPGen batch id=%s step=%d action=%s ok=%s ms=%.2f"),
				*BatchId,
				Index,
				*StepAction,
				StepResult.bOk ? TEXT("true") : TEXT("false"),
				StepResult.DispatchMs);

			if (Session)
			{
				FString BlueprintPath;
				if (StepParams.IsValid() && StepParams->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath))
				{
					Session->LastBlueprintPath = BlueprintPath;
				}
			}

			TSharedPtr<FJsonObject> StepObj = MakeShared<FJsonObject>();
			StepObj->SetNumberField(TEXT("index"), Index);
			StepObj->SetStringField(TEXT("action"), StepAction);
			StepObj->SetBoolField(TEXT("ok"), StepResult.bOk);
			StepObj->SetNumberField(TEXT("ms"), StepResult.DispatchMs);

			if (!StepResult.ErrorCode.IsEmpty())
			{
				StepObj->SetStringField(TEXT("error_code"), StepResult.ErrorCode);
				ErrorCodeCounts.FindOrAdd(StepResult.ErrorCode)++;
			}

			TArray<TSharedPtr<FJsonValue>> StepErrors;
			for (const FString& Error : StepResult.Errors)
			{
				StepErrors.Add(MakeShared<FJsonValueString>(Error));
			}
			StepObj->SetArrayField(TEXT("errors"), StepErrors);

			TArray<TSharedPtr<FJsonValue>> StepWarnings;
			for (const FString& Warning : StepResult.Warnings)
			{
				StepWarnings.Add(MakeShared<FJsonValueString>(Warning));
			}
			StepObj->SetArrayField(TEXT("warnings"), StepWarnings);

			if (StepResult.Result.IsValid())
			{
				StepObj->SetObjectField(TEXT("result"), StepResult.Result);
			}
			else
			{
				StepObj->SetObjectField(TEXT("result"), MakeShared<FJsonObject>());
			}

			StepValues.Add(MakeShared<FJsonValueObject>(StepObj));

			if (StepResult.bOk)
			{
				OkSteps++;
			}
			else
			{
				FailedSteps++;
				if (bStopOnError)
				{
					break;
				}
			}
		}

		const double BatchEndSeconds = FPlatformTime::Seconds();
		TSharedPtr<FJsonObject> SummaryObj = MakeShared<FJsonObject>();
		SummaryObj->SetNumberField(TEXT("ok_steps"), OkSteps);
		SummaryObj->SetNumberField(TEXT("failed_steps"), FailedSteps);
		TSharedPtr<FJsonObject> ErrorCodesObj = MakeShared<FJsonObject>();
		for (const TPair<FString, int32>& Pair : ErrorCodeCounts)
		{
			ErrorCodesObj->SetNumberField(Pair.Key, Pair.Value);
		}
		SummaryObj->SetObjectField(TEXT("error_codes"), ErrorCodesObj);

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("batch_id"), BatchId);
		ResultObj->SetBoolField(TEXT("atomic"), bAtomic);
		ResultObj->SetArrayField(TEXT("steps"), StepValues);
		ResultObj->SetNumberField(TEXT("ms_total"), (BatchEndSeconds - BatchStartSeconds) * 1000.0);
		ResultObj->SetObjectField(TEXT("summary"), SummaryObj);

		if (Session)
		{
			ResultObj->SetStringField(TEXT("session_id"), Session->SessionId);
		}

		OutResult.bOk = FailedSteps == 0;
		OutResult.Result = ResultObj;
		if (FailedSteps > 0 && bStopOnError)
		{
			OutResult.Errors.Add(TEXT("Batch stopped after a failed step."));
		}
		if (bAtomic && FailedSteps > 0)
		{
			OutResult.Warnings.Add(TEXT("Atomic batch failed; changes may have partially applied."));
		}

		UE_LOG(LogSOTS_BPGenBridge, Log, TEXT("BPGen batch id=%s ok_steps=%d failed_steps=%d ms=%.2f"),
			*BatchId, OkSteps, FailedSteps, (BatchEndSeconds - BatchStartSeconds) * 1000.0);
		return;
	}

	if (Action.Equals(TEXT("prime_cache"), ESearchCase::IgnoreCase))
	{
		FString BlueprintPath;
		FString Scope = TEXT("global");
		bool bIncludeNodes = true;
		bool bIncludePins = false;
		int32 MaxItems = 5000;
		FString SessionId;

		if (Params.IsValid())
		{
			Params->TryGetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
			Params->TryGetStringField(TEXT("scope"), Scope);
			Params->TryGetBoolField(TEXT("include_nodes"), bIncludeNodes);
			Params->TryGetBoolField(TEXT("include_pins"), bIncludePins);
			Params->TryGetNumberField(TEXT("max_items"), MaxItems);
			Params->TryGetStringField(TEXT("session_id"), SessionId);
		}

		Scope.TrimStartAndEndInline();
		const FString ScopeLower = Scope.ToLower();
		if (ScopeLower == TEXT("blueprint"))
		{
			if (BlueprintPath.IsEmpty())
			{
				OutResult.bOk = false;
				OutResult.Errors.Add(TEXT("prime_cache scope 'blueprint' requires blueprint_asset_path."));
				return;
			}
		}
		else
		{
			Scope = TEXT("global");
		}

		if (MaxItems < 0)
		{
			MaxItems = 0;
		}

		UBlueprint* Blueprint = nullptr;
		const bool bBlueprintScope = ScopeLower == TEXT("blueprint");
		if (!BlueprintPath.IsEmpty() && bBlueprintScope)
		{
			Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
			if (!Blueprint)
			{
				OutResult.bOk = false;
				OutResult.Errors.Add(FString::Printf(TEXT("Failed to load Blueprint '%s'."), *BlueprintPath));
				return;
			}
		}

		FString PrimeError;
		const int32 Primed = FSOTS_BPGenSpawnerRegistry::PrimeCache(Blueprint, MaxItems, PrimeError);

		int32 DiscoveredNodes = 0;
		const FString DiscoveryBlueprintPath = bBlueprintScope ? BlueprintPath : FString();
		if (bIncludeNodes)
		{
			if (bIncludePins && MaxPinHarvestNodes == 0)
			{
				bIncludePins = false;
				OutResult.Warnings.Add(TEXT("prime_cache include_pins disabled (max_pin_harvest_nodes=0)."));
			}

			int32 EffectiveMaxItems = MaxItems;
			if (MaxDiscoveryResults > 0 && EffectiveMaxItems > MaxDiscoveryResults)
			{
				OutResult.Warnings.Add(FString::Printf(TEXT("prime_cache max_items clamped to %d (requested %d)."), MaxDiscoveryResults, EffectiveMaxItems));
				EffectiveMaxItems = MaxDiscoveryResults;
			}
			if (bIncludePins && MaxPinHarvestNodes > 0 && EffectiveMaxItems > MaxPinHarvestNodes)
			{
				OutResult.Warnings.Add(FString::Printf(TEXT("prime_cache pin harvest clamped to %d nodes (requested %d)."), MaxPinHarvestNodes, EffectiveMaxItems));
				EffectiveMaxItems = MaxPinHarvestNodes;
			}

			const FSOTS_BPGenNodeDiscoveryResult Discovery = USOTS_BPGenDiscovery::DiscoverNodesWithDescriptors(nullptr, DiscoveryBlueprintPath, FString(), EffectiveMaxItems, bIncludePins);
			DiscoveredNodes = Discovery.Descriptors.Num();
			OutResult.Warnings.Append(Discovery.Warnings);
			if (!Discovery.bSuccess)
			{
				OutResult.Errors.Append(Discovery.Errors);
			}
		}

		if (!PrimeError.IsEmpty())
		{
			OutResult.Warnings.Add(PrimeError);
		}

		if (!SessionId.IsEmpty())
		{
			if (FSessionState* Session = Sessions.Find(SessionId))
			{
				Session->bCachePrimed = true;
				Session->LastAccessSeconds = FPlatformTime::Seconds();
				if (!BlueprintPath.IsEmpty())
				{
					Session->LastBlueprintPath = BlueprintPath;
				}
			}
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("scope"), Scope);
		ResultObj->SetStringField(TEXT("blueprint_asset_path"), BlueprintPath);
		ResultObj->SetNumberField(TEXT("primed_spawners"), Primed);
		ResultObj->SetNumberField(TEXT("discovered_nodes"), DiscoveredNodes);
		ResultObj->SetBoolField(TEXT("include_nodes"), bIncludeNodes);
		ResultObj->SetBoolField(TEXT("include_pins"), bIncludePins);
		ResultObj->SetNumberField(TEXT("max_items"), MaxItems);
		OutResult.bOk = OutResult.Errors.Num() == 0;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("clear_cache"), ESearchCase::IgnoreCase))
	{
		FSOTS_BPGenSpawnerRegistry::Clear();
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("status"), TEXT("cleared"));
		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("set_limits"), ESearchCase::IgnoreCase))
	{
		if (Params.IsValid())
		{
			int32 Value = 0;
			if (Params->TryGetNumberField(TEXT("max_request_bytes"), Value))
			{
				MaxRequestBytes = FMath::Max(0, Value);
			}
			if (Params->TryGetNumberField(TEXT("max_requests_per_second"), Value))
			{
				MaxRequestsPerSecond = FMath::Max(0, Value);
				RateWindowStartSeconds = 0.0;
				RequestsInWindow = 0;
			}
			if (Params->TryGetNumberField(TEXT("max_requests_per_minute"), Value))
			{
				MaxRequestsPerMinute = FMath::Max(0, Value);
				MinuteWindowStartSeconds = 0.0;
				RequestsInMinute = 0;
			}
			if (Params->TryGetNumberField(TEXT("max_discovery_results"), Value))
			{
				MaxDiscoveryResults = FMath::Max(0, Value);
			}
			if (Params->TryGetNumberField(TEXT("max_pin_harvest_nodes"), Value))
			{
				MaxPinHarvestNodes = FMath::Max(0, Value);
			}
			if (Params->TryGetNumberField(TEXT("max_autofix_steps"), Value))
			{
				MaxAutoFixSteps = FMath::Max(0, Value);
			}
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetNumberField(TEXT("max_request_bytes"), MaxRequestBytes);
		ResultObj->SetNumberField(TEXT("max_requests_per_second"), MaxRequestsPerSecond);
		ResultObj->SetNumberField(TEXT("max_requests_per_minute"), MaxRequestsPerMinute);
		ResultObj->SetNumberField(TEXT("max_discovery_results"), MaxDiscoveryResults);
		ResultObj->SetNumberField(TEXT("max_pin_harvest_nodes"), MaxPinHarvestNodes);
		ResultObj->SetNumberField(TEXT("max_autofix_steps"), MaxAutoFixSteps);
		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
	}

	if (Action.Equals(TEXT("get_recent_requests"), ESearchCase::IgnoreCase))
	{
		TArray<TSharedPtr<FJsonValue>> RecentValues;
		{
			FScopeLock Lock(&RecentRequestsMutex);
			for (const FRecentRequestSummary& Summary : RecentRequests)
			{
				TSharedPtr<FJsonObject> SummaryObj = MakeShared<FJsonObject>();
				SummaryObj->SetStringField(TEXT("request_id"), Summary.RequestId);
				SummaryObj->SetStringField(TEXT("action"), Summary.Action);
				SummaryObj->SetBoolField(TEXT("ok"), Summary.bOk);
				SummaryObj->SetStringField(TEXT("error_code"), Summary.ErrorCode);
				SummaryObj->SetNumberField(TEXT("request_ms"), Summary.RequestMs);
				SummaryObj->SetStringField(TEXT("timestamp"), Summary.Timestamp);
				RecentValues.Add(MakeShared<FJsonValueObject>(SummaryObj));
			}
		}

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetArrayField(TEXT("requests"), RecentValues);
		OutResult.bOk = true;
		OutResult.Result = ResultObj;
		return;
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

			FString RepairMode;
			if (Params->TryGetStringField(TEXT("repair_mode"), RepairMode) && !RepairMode.IsEmpty())
			{
				GraphSpec.RepairMode = RepairMode;
			}
		}

		if (MaxAutoFixSteps > 0 && GraphSpec.AutoFixMaxSteps > MaxAutoFixSteps)
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("auto_fix_max_steps clamped to %d (requested %d)."), MaxAutoFixSteps, GraphSpec.AutoFixMaxSteps));
			GraphSpec.AutoFixMaxSteps = MaxAutoFixSteps;
		}

		if (bDryRun)
		{
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(FunctionDef.TargetBlueprintPath, FunctionDef.FunctionName, GraphSpec);
			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &DryRunResult, ResultJson.ToSharedRef(), 0, 0);
			AddChangeSummaryAlias(ResultJson);
			AddSpecMigrationAliases(ResultJson);
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
		AddChangeSummaryAlias(ResultJson);
		AddSpecMigrationAliases(ResultJson);
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

		if (Params.IsValid())
		{
			FString RepairMode;
			if (Params->TryGetStringField(TEXT("repair_mode"), RepairMode) && !RepairMode.IsEmpty())
			{
				GraphSpec.RepairMode = RepairMode;
			}
		}

		if (MaxAutoFixSteps > 0 && GraphSpec.AutoFixMaxSteps > MaxAutoFixSteps)
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("auto_fix_max_steps clamped to %d (requested %d)."), MaxAutoFixSteps, GraphSpec.AutoFixMaxSteps));
			GraphSpec.AutoFixMaxSteps = MaxAutoFixSteps;
		}

		if (bDryRun)
		{
			const FName TargetName = GraphSpec.Target.Name.IsEmpty() ? NAME_None : FName(*GraphSpec.Target.Name);
			const FSOTS_BPGenApplyResult DryRunResult = BuildDryRunApplyResult(GraphSpec.Target.BlueprintAssetPath, TargetName, GraphSpec);
			TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenApplyResult::StaticStruct(), &DryRunResult, ResultJson.ToSharedRef(), 0, 0);
			AddChangeSummaryAlias(ResultJson);
			AddSpecMigrationAliases(ResultJson);
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
		AddChangeSummaryAlias(ResultJson);
		AddSpecMigrationAliases(ResultJson);
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

		int32 EffectiveMaxResults = MaxResults;
		if (bIncludePins && MaxPinHarvestNodes == 0)
		{
			bIncludePins = false;
			OutResult.Warnings.Add(TEXT("discover_nodes include_pins disabled (max_pin_harvest_nodes=0)."));
		}
		if (MaxDiscoveryResults > 0 && EffectiveMaxResults > MaxDiscoveryResults)
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("discover_nodes max_results clamped to %d (requested %d)."), MaxDiscoveryResults, EffectiveMaxResults));
			EffectiveMaxResults = MaxDiscoveryResults;
		}
		if (bIncludePins && MaxPinHarvestNodes > 0 && EffectiveMaxResults > MaxPinHarvestNodes)
		{
			OutResult.Warnings.Add(FString::Printf(TEXT("discover_nodes pin harvest clamped to %d nodes (requested %d)."), MaxPinHarvestNodes, EffectiveMaxResults));
			EffectiveMaxResults = MaxPinHarvestNodes;
		}

		const FSOTS_BPGenNodeDiscoveryResult Discovery = USOTS_BPGenDiscovery::DiscoverNodesWithDescriptors(nullptr, BlueprintPath, SearchText, EffectiveMaxResults, bIncludePins);
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
			AddChangeSummaryAlias(ResultJson);
			AddSpecMigrationAliases(ResultJson);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = true;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryEnsure.Warnings);
			return;
		}

		const FSOTS_BPGenEnsureResult EnsureResult = USOTS_BPGenEnsure::EnsureFunction(nullptr, BlueprintPath, FunctionName, Signature, bCreateIfMissing, bUpdateIfExists);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &EnsureResult, ResultJson.ToSharedRef(), 0, 0);
		AddChangeSummaryAlias(ResultJson);
		AddSpecMigrationAliases(ResultJson);
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
			AddChangeSummaryAlias(ResultJson);
			AddSpecMigrationAliases(ResultJson);
			ResultJson->SetBoolField(TEXT("dry_run"), true);
			OutResult.bOk = true;
			OutResult.Result = ResultJson;
			OutResult.Warnings.Append(DryEnsure.Warnings);
			return;
		}

		const FSOTS_BPGenEnsureResult EnsureResult = USOTS_BPGenEnsure::EnsureVariable(nullptr, BlueprintPath, VarName, VarType, DefaultValue, bInstanceEditable, bExposeOnSpawn, bCreateIfMissing, bUpdateIfExists);
		TSharedPtr<FJsonObject> ResultJson = MakeShared<FJsonObject>();
		FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenEnsureResult::StaticStruct(), &EnsureResult, ResultJson.ToSharedRef(), 0, 0);
		AddChangeSummaryAlias(ResultJson);
		AddSpecMigrationAliases(ResultJson);
		OutResult.bOk = EnsureResult.bSuccess;
		OutResult.Result = ResultJson;
		OutResult.Warnings.Append(EnsureResult.Warnings);
		if (!EnsureResult.bSuccess && !EnsureResult.ErrorMessage.IsEmpty())
		{
			OutResult.Errors.Add(EnsureResult.ErrorMessage);
		}
		return;
	}

		if (Action.Equals(TEXT("create_data_asset"), ESearchCase::IgnoreCase))
		{
			FSOTS_BPGenDataAssetDef AssetDef;
			if (Params.IsValid())
			{
				Params->TryGetStringField(TEXT("asset_path"), AssetDef.AssetPath);
				Params->TryGetStringField(TEXT("asset_class_path"), AssetDef.AssetClassPath);
				Params->TryGetBoolField(TEXT("create_if_missing"), AssetDef.bCreateIfMissing);
				Params->TryGetBoolField(TEXT("update_if_exists"), AssetDef.bUpdateIfExists);
				if (Params->HasTypedField<EJson::Array>(TEXT("properties")))
				{
					const TArray<TSharedPtr<FJsonValue>> PropertyArray = Params->GetArrayField(TEXT("properties"));
					for (const TSharedPtr<FJsonValue>& Entry : PropertyArray)
					{
						if (!Entry.IsValid() || Entry->Type != EJson::Object)
						{
							continue;
						}
						FSOTS_BPGenAssetProperty Property;
						FJsonObjectConverter::JsonObjectToUStruct(Entry->AsObject().ToSharedRef(), FSOTS_BPGenAssetProperty::StaticStruct(), &Property, 0, 0);
						AssetDef.Properties.Add(MoveTemp(Property));
					}
				}
			}

			if (AssetDef.AssetPath.IsEmpty() || AssetDef.AssetClassPath.IsEmpty())
			{
				OutResult.bOk = false;
				OutResult.Errors.Add(TEXT("create_data_asset requires asset_path and asset_class_path."));
				return;
			}

			if (bDryRun)
			{
				FSOTS_BPGenAssetResult DryResult;
				DryResult.bSuccess = true;
				DryResult.AssetPath = AssetDef.AssetPath;
				DryResult.Message = TEXT("Dry run: create_data_asset skipped.");
				TSharedPtr<FJsonObject> DryJson = MakeShared<FJsonObject>();
				FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenAssetResult::StaticStruct(), &DryResult, DryJson.ToSharedRef(), 0, 0);
				DryJson->SetBoolField(TEXT("dry_run"), true);
				OutResult.bOk = true;
				OutResult.Result = DryJson;
				OutResult.Warnings.Append(DryResult.Warnings);
				return;
			}

			const FSOTS_BPGenAssetResult AssetResult = USOTS_BPGenBuilder::CreateDataAssetFromDef(nullptr, AssetDef);
			TSharedPtr<FJsonObject> ResultJsonAsset = MakeShared<FJsonObject>();
			FJsonObjectConverter::UStructToJsonObject(FSOTS_BPGenAssetResult::StaticStruct(), &AssetResult, ResultJsonAsset.ToSharedRef(), 0, 0);
			OutResult.bOk = AssetResult.bSuccess;
			OutResult.Result = ResultJsonAsset;
			OutResult.Errors.Append(AssetResult.Errors);
			if (!AssetResult.ErrorMessage.IsEmpty())
			{
				OutResult.Errors.Add(AssetResult.ErrorMessage);
			}
			OutResult.Warnings.Append(AssetResult.Warnings);
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
			AddChangeSummaryAlias(ResultJson);
			AddSpecMigrationAliases(ResultJson);
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
		AddChangeSummaryAlias(ResultJson);
		AddSpecMigrationAliases(ResultJson);
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
			AddChangeSummaryAlias(ResultJson);
			AddSpecMigrationAliases(ResultJson);
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
		AddChangeSummaryAlias(ResultJson);
		AddSpecMigrationAliases(ResultJson);
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
			AddChangeSummaryAlias(ResultJson);
			AddSpecMigrationAliases(ResultJson);
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
		AddChangeSummaryAlias(ResultJson);
		AddSpecMigrationAliases(ResultJson);
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

		Async(EAsyncExecution::Thread, [WeakSelf = AsWeak()]
		{
			if (TSharedPtr<FSOTS_BPGenBridgeServer> Self = WeakSelf.Pin())
			{
				Self->Stop();
			}
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
	const bool bHasRateLimit = MaxRequestsPerSecond > 0 || MaxRequestsPerMinute > 0;
	Response->SetObjectField(TEXT("server.features"), BuildFeatureFlags(true, bHasAuthToken, bHasRateLimit));
	if (DispatchResult.RequestMs > 0.0)
	{
		Response->SetNumberField(TEXT("server.request_ms"), DispatchResult.RequestMs);
	}
	if (DispatchResult.DispatchMs > 0.0)
	{
		Response->SetNumberField(TEXT("server.dispatch_ms"), DispatchResult.DispatchMs);
	}

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
	Response->SetNumberField(TEXT("server.serialize_ms"), 0.0);

	double SerializeStartSeconds = FPlatformTime::Seconds();
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseString);
		FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	}
	double SerializeMs = (FPlatformTime::Seconds() - SerializeStartSeconds) * 1000.0;

	Response->SetNumberField(TEXT("server.serialize_ms"), SerializeMs);
	ResponseString.Reset();
	{
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResponseString);
		FJsonSerializer::Serialize(Response.ToSharedRef(), Writer);
	}

	ResponseString.AppendChar('\n');
	return ResponseString;
}

bool FSOTS_BPGenBridgeServer::SendResponseAndClose(FSocket* ClientSocket, const FString& ResponseString) const
{
	if (!ClientSocket)
	{
		return false;
	}

	FString NormalizedResponse = ResponseString;
	while (NormalizedResponse.EndsWith(TEXT("\n")) || NormalizedResponse.EndsWith(TEXT("\r")))
	{
		NormalizedResponse.LeftChopInline(1, false);
	}
	NormalizedResponse.AppendChar('\n');

	UE_LOG(LogSOTS_BPGenBridge, Verbose, TEXT("BPGen sending response len=%d: %s"), NormalizedResponse.Len(), *NormalizedResponse);

	FTCHARToUTF8 Converter(*NormalizedResponse);
	const uint8* Buffer = reinterpret_cast<const uint8*>(Converter.Get());
	const int32 Length = Converter.Length();
	int32 TotalSent = 0;
	int32 Attempts = 0;
	while (TotalSent < Length && Attempts < 16)
	{
		int32 SentNow = 0;
		if (!ClientSocket->Send(Buffer + TotalSent, Length - TotalSent, SentNow))
		{
			FPlatformProcess::Sleep(0.001f);
			++Attempts;
			continue;
		}

		if (SentNow <= 0)
		{
			FPlatformProcess::Sleep(0.001f);
			++Attempts;
			continue;
		}

		TotalSent += SentNow;
		++Attempts;
	}

	ClientSocket->Close();
	return TotalSent == Length;
}
