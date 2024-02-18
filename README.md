# ActorSingleton

Tiny UE5 plugin that adds [AActorSingleton](https://github.com/sleeptightAnsiC/ActorSingleton/blob/main/Source/ActorSingleton/Public/ActorSingleton.h#L35) class.

Install like every other plugin, by cloning into project's `Plugins` directory.

For context see the information below or read the source code.

## About

This plugin has been created for my personal use, but I decided to publish it, since it has proven to be somehow usefull.

Imagine the following case: you need an [UObject](https://docs.unrealengine.com/5.3/en-US/API/Runtime/CoreUObject/UObject/UObject/) that lives within [UWorld](https://docs.unrealengine.com/5.3/en-US/API/Runtime/Engine/Engine/UWorld/), there can be only one spawned instance of said object and it must be accessible from the [Editor's Level Viewport](https://docs.unrealengine.com/5.3/en-US/editor-viewports-in-unreal-engine/). Basically, something like "[Singleton](https://en.wikipedia.org/wiki/Singleton_pattern)".

So what options do we have? Unreal Engine has [few ways of creating singletons](https://benui.ca/unreal/subsystem-singleton/), the best out of them is creating a class derived from [USubsytem](https://docs.unrealengine.com/5.3/en-US/API/Runtime/Engine/Subsystems/USubsystem/). The one that suits most of our needs is [UWorldSubsystem](https://docs.unrealengine.com/5.3/en-US/API/Runtime/Engine/Subsystems/UWorldSubsystem/), but said class isn't spawnable and won't appear in the Level Viewport, so it doesn't fully fit our needs.

The other option would be creating an [AActor](https://docs.unrealengine.com/5.3/en-US/API/Runtime/Engine/GameFramework/AActor/). Actors are easily accessible from Level Viewport. However, how do we ensure that there is only one spawned instance of said Actor?...

...and this is why this plugin exists. It introduces [AActorSingleton](https://github.com/sleeptightAnsiC/ActorSingleton/blob/main/Source/ActorSingleton/Public/ActorSingleton.h#L35) class. If you try to spawn multiple instances of said class, it will only allow to spawn the first instance and will automaticly destroy the rest.

## Usage

Derive from [AActorSingleton](https://github.com/sleeptightAnsiC/ActorSingleton/blob/main/Source/ActorSingleton/Public/ActorSingleton.h#L35) and make sure that [ActorSingleton::IsFinalParent](https://github.com/sleeptightAnsiC/ActorSingleton/blob/main/Source/ActorSingleton/Public/ActorSingleton.h#L49) returns `true` for your class.

Whenever you try to spawn a duplicate instace, you will get a meaningfull error about it. If you try to do this by placing an Actor to the Level Viewport you will even get a clear popup message:

![image](https://github.com/sleeptightAnsiC/ActorSingleton/assets/91839286/ef8cd4f1-9a0d-47e3-9522-77eb1351e80e)

#### Tested on Linux with UE 5.3.2 and clang
