#include "UE5PMS_Nvidia_BlueprintLibrary.h"
#include "UE5PMS_NvidiaBackend.h"

FUE5PMS_NvGpuMetrics UUE5PMS_Nvidia_BlueprintLibrary::GetNvidiaStats(bool& bIsValid, FString& ErrorMessage)
{
	FUE5PMS_NvGpuMetrics M;

	bool bOK = FPMS_NvApiBackend::Get().QueryMetrics(M);

	bIsValid = bOK && M.bIsValid;
	ErrorMessage = M.ErrorMessage;

	return M;
}
