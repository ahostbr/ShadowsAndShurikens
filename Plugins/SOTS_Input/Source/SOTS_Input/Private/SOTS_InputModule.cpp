#include "SOTS_InputModule.h"

#define LOCTEXT_NAMESPACE "FSOTS_InputModule"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
namespace SOTS::Input::ConsoleCommands
{
	void Register();
	void Unregister();
}
#endif

void FSOTS_InputModule::StartupModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SOTS::Input::ConsoleCommands::Register();
#endif
}

void FSOTS_InputModule::ShutdownModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	SOTS::Input::ConsoleCommands::Unregister();
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSOTS_InputModule, SOTS_Input)
