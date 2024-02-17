// Published under MIT License, created by https://github.com/sleeptightAnsiC

using UnrealBuildTool;

public class ActorSingleton : ModuleRules
{
	public ActorSingleton(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
		});

		// we only want this to be included for editor builds but not packaged builds
		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
			});
		}
	}
}
