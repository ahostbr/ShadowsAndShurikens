using UnrealBuildTool;
using System;
using System.IO;

public class UE5PMS : ModuleRules
{
    public UE5PMS(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "RenderCore",
            "RHI"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Projects"
        });

        // Default vendor flags
        PublicDefinitions.Add("UE5PMS_HAS_NVAPI=0");
        PublicDefinitions.Add("UE5PMS_HAS_ADLX=0");
        PublicDefinitions.Add("UE5PMS_HAS_RHI=1"); // always on

        // ModuleDirectory = <Project>/Plugins/UE5PMS/Source/UE5PMS
        // ThirdPartyPath = <Project>/Plugins/UE5PMS/ThirdParty
        ThirdPartyPath = Path.GetFullPath(
            Path.Combine(ModuleDirectory, "..", "..", "ThirdParty")
        );

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            ConfigureNvAPI(Target);
            ConfigureADLX(Target);
        }
    }

    private string ThirdPartyPath;

    private void ConfigureNvAPI(ReadOnlyTargetRules Target)
    {
        string nvRoot    = Path.Combine(ThirdPartyPath, "NVAPI");
        string nvInclude = Path.Combine(nvRoot, "Include");
        string nvLibDir  = Path.Combine(nvRoot, "Lib", "Win64");
        string nvLib     = Path.Combine(nvLibDir, "nvapi64.lib");

        if (!Directory.Exists(nvRoot) ||
            !Directory.Exists(nvInclude) ||
            !Directory.Exists(nvLibDir) ||
            !File.Exists(nvLib))
        {
            return; // leave UE5PMS_HAS_NVAPI=0
        }

        // For #include "nvapi.h"
        PublicSystemIncludePaths.Add(nvInclude);

        PublicAdditionalLibraries.Add(nvLib);

        // If you ship the DLL alongside the plugin, you can add a runtime dependency here later.

        PublicDefinitions.Remove("UE5PMS_HAS_NVAPI=0");
        PublicDefinitions.Add("UE5PMS_HAS_NVAPI=1");
    }

    private void ConfigureADLX(ReadOnlyTargetRules Target)
    {
        // Your current layout:
        //   ThirdParty/SDK/ADLXHelper/Windows/Cpp/ADLXHelper.h
        //   ThirdParty/SDK/Include/ADLX.h
        string adlxRoot  = Path.Combine(ThirdPartyPath, "SDK");
        string helperHdr = Path.Combine(adlxRoot, "ADLXHelper", "Windows", "Cpp", "ADLXHelper.h");
        string coreHdr   = Path.Combine(adlxRoot, "Include", "ADLX.h");

        if (!File.Exists(helperHdr) || !File.Exists(coreHdr))
        {
            return; // leave UE5PMS_HAS_ADLX=0
        }

        // Let the compiler see "SDK/..." includes
        PublicSystemIncludePaths.Add(adlxRoot);

        // Optional: static import lib if/when you add it
        string libPath = Path.Combine(adlxRoot, "Lib", "Win64", "adlx64.lib");
        if (File.Exists(libPath))
        {
            PublicAdditionalLibraries.Add(libPath);
        }

        PublicDefinitions.Remove("UE5PMS_HAS_ADLX=0");
        PublicDefinitions.Add("UE5PMS_HAS_ADLX=1");
    }
}
