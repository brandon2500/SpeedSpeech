// SpeedSpeech - Unreal Engine module.
using System.IO;
using UnrealBuildTool;

public class SpeedSpeech : ModuleRules
{
    public SpeedSpeech(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",
            "AudioMixer",   // USynthComponent
        });

        // The C core ships as a single-file amalgamation (tools/amalgamate.py) and is
        // compiled as C++ by Private/SpeedSpeechCore.cpp: no prebuilt binaries needed,
        // so the plugin builds on every platform Unreal supports.
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "..", "ThirdParty", "SpeedSpeechCore"));
    }
}
