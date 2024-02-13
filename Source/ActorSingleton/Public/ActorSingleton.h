// Published under MIT License, created by https://github.com/sleeptightAnsiC

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GameFramework/Actor.h"
#include "ActorSingleton.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(ActorSingleton, Log, All);

/*================================================================================
=	Actor Singleton:
=
=	An Actor that is expected to have only one instance within UWorld.
=	If we try to spawn an Actor derived from AActorSingleton,
=		and there is already an existing instance of the same class (or sub-class),
=		said Actor will be automatically destroyed.
=
=	Pretty much the whole magic happens in the AActorSingleton::TryBecomeNewInstanceOrSelfDestroy
=
=	Make sure to override AActorSingleton::IsFinalSingletonClass,
=		to make it return 'true' for the final class!
=
================================================================================*/


/* Minimal implementation of Unreal Module (boilerplate) */
class FActorSingletonModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {};
	virtual void ShutdownModule() override {};
};


/* An Actor that is expected to have only one instance within UWorld
* If a new isntance is gets created, it will be automatically destroyed. */
UCLASS(Abstract)
class ACTORSINGLETON_API AActorSingleton : public AActor
{
	GENERATED_BODY()

	friend UActorSingletonManager;

public:

	/* Try to become a new single instance within current UWorld,
	* if instance already exists, call this->Destroy
	* Does nothing in few circumstances, e.g. when calling on CDO */
	void TryBecomeNewInstanceOrSelfDestroy();

	/* Tells whenever the instances of sub-class will be considered the same as instances of base class
	* e.g. B is a subclass of A, A::IsFinalSingletonClass returns 'true',
	*		so if there is already an instance of A, new instance of B will be destroyed (and vice-versa)
	* This function is only called on CDO, it basically works like a 'static' function,
	*		but since we have to support Blueprints, we cannot simply make it static,
	*		as BlueprintNativeEvent does NOT support static functions */
	UFUNCTION(BlueprintNativeEvent)
	bool IsFinalSingletonClass() const;
	virtual bool IsFinalSingletonClass_Implementation() const { return false; }

	/* Override to provide a custom HEADER for the message which appears in the Editor
	*	when you place a duplicate of Actor Singleton into Level Viewport
	* Unlike IsFinalSingletonClass, this function runs on object instance, not on CDO. */
	UFUNCTION(BlueprintNativeEvent)
	FText GetMessageTitle() const;
	virtual FText GetMessageTitle_Implementation() const;

	/* Override to provide a custom BODY for the message which appears in the Editor
	*	when you place a duplicate of Actor Singleton into Level Viewport
	* Unlike IsFinalSingletonClass, this function runs on object instance, not on CDO. */
	UFUNCTION(BlueprintNativeEvent)
	FText GetMessageBody() const;
	virtual FText GetMessageBody_Implementation() const;

	/* Gets a reference to the single instance of chosen AActorSingleton subclass within current UWorld,
	* may return 'nullptr' if it doesn't exist.
	* This is a BP version of this function. For better typesafety in C++, use AActorSingleton::Get<T> */
	UFUNCTION(BlueprintCallable, BlueprintPure,
		meta = (DisplayName = "Get Actor Singleton Instance", DeterminesOutputType = "Class", WorldContext = "WorldContext"))
	static AActorSingleton* GetInstance(const UWorld* const WorldContext, TSubclassOf<AActorSingleton> Class);

	/* Templated version of AActorSingleton::Get
	* Will cause a compilation error if you try to call it on class not derived from AActorSingleton. */
	template<class T>
	static T* GetInstance(const UWorld* const World)
	{
		static_assert(TIsDerivedFrom<T, AActorSingleton>::IsDerived, "T must be derived from AActorSingleton");
		return static_cast<T*>(AActorSingleton::GetInstance(World, T::StaticClass()));
	}

	/* Templated version of AActorSingleton::Get
	* Will cause a compilation error if you try to call it on class not derived from AActorSingleton. */
	template<class T>
	static T* GetInstance(const UObject* WorldContextObject)
	{
		const UWorld* const World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
		return AActorSingleton::GetInstance<T>(World);
	}

	/* Same as AActorSingleton::Get<T> but causes a crash in case of returning 'nullptr' */
	template<class T>
	static T* GetInstanceChecked(const UWorld* const World)
	{
		T* Instance = AActorSingleton::GetInstance<T>(World);
		check(Instance)
		return Instance;
	}

	/* Same as AActorSingleton::Get<T> but causes a crash in case of returning 'nullptr' */
	template<class T>
	static T* GetInstanceChecked(const UObject* WorldContextObject)
	{
		T* Instance = AActorSingleton::GetInstance<T>(WorldContextObject);
		check(Instance)
		return Instance;
	}

public:

	//~ Begin AActor Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End AActor Interface
};


/* Helper class for storing "static" references to AActorSingleton instances.
* Each subclass of AActorSingleton is expected to have only one spawned instance within each UWorld,
* that's why we use World Subsystem as it always has one instance per every UWorld. */
UCLASS(NotBlueprintable)
class ACTORSINGLETON_API UActorSingletonManager : public UWorldSubsystem
{
	GENERATED_BODY()

	friend AActorSingleton;

private:

	/* Gets all AActorSingleton in the current UWorld,
	* and calls AActorSingleton::TryBecomeNewInstanceOrSelfDestroy on all of them. */
	void FindInstancesAndDestroyDuplicates();

	/* Wrapper for UWorld::GetSubsystem<UActorSingletonManager>
	* May return 'nullptr' in case of Manager not being initialized yet. */
	static UActorSingletonManager* Get(const UWorld* const World);

	/* Wrapper for UWorld::GetSubsystem<UActorSingletonManager>
	* This version causes a crash in case of returning 'nullptr'. */
	static UActorSingletonManager* GetChecked(const UWorld* const World);

	UPROPERTY()
	TMap<TSubclassOf<AActorSingleton>, AActorSingleton*> Instances;

public:

	//~ Begin UWorldSubsystem Interface
	virtual void PostInitialize() override;
	//~ End UWorldSubsystem Interface
};
