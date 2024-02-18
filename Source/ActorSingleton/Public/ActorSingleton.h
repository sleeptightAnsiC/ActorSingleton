// Published under MIT License, created by https://github.com/sleeptightAnsiC

#pragma once

#include "CoreMinimal.h"
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

	/* If set to 'true', all sub-classess will be considered as duplicates.
	* By default, this function returns true for any non-Abstract class,
	* 	but you can override it, if you wish to have base class that is abstract.
	* This function only runs on CDO, so any conditional logic won't make any sense.
	* It's basically static function. */
	UFUNCTION(BlueprintNativeEvent)
	bool IsFinalParent() const;
	virtual bool IsFinalParent_Implementation() const
	{
		const UClass* const ThisClass = GetClass();
		const bool bAbstract = ThisClass->HasAnyClassFlags(EClassFlags::CLASS_Abstract);
		return !bAbstract;
	};

	/* Override to provide a custom HEADER for the message which appears in the Editor
	*	when you place a duplicate of Actor Singleton into Level Viewport
	* Unlike IsFinalParent, this function runs on object instance, not on CDO. */
	UFUNCTION(BlueprintNativeEvent)
	FText GetMessageTitle() const;
	virtual FText GetMessageTitle_Implementation() const;

	/* Override to provide a custom BODY for the message which appears in the Editor
	*	when you place a duplicate of Actor Singleton into Level Viewport
	* Unlike IsFinalParent, this function runs on object instance, not on CDO. */
	UFUNCTION(BlueprintNativeEvent)
	FText GetMessageBody() const;
	virtual FText GetMessageBody_Implementation() const;

	/* Gets a reference to the single instance of chosen AActorSingleton subclass within current UWorld,
	* may return 'nullptr' if it doesn't exist.
	* This is a BP version of this function. For better typesafety in C++, use AActorSingleton::Get<T> */
	UFUNCTION(BlueprintCallable, BlueprintPure,
		meta = (DisplayName = "Get Actor Singleton Instance", DeterminesOutputType = "Class", WorldContext = "WorldContext"))
	static AActorSingleton* GetInstance(const UObject* const WorldContext, TSubclassOf<AActorSingleton> Class);

	/* Templated version of AActorSingleton::GetInstance */
	template<class T>
	static T* GetInstance(const UObject* WorldContext)
	{
		static_assert(TIsDerivedFrom<T, AActorSingleton>::IsDerived);
		check(IsValid(WorldContext))
		checkCode(
			UObject* CDO = T::StaticClass()->GetDefaultObject();
			TSubclassOf<AActorSingleton> FinalParent = static_cast<AActorSingleton*>(CDO)->GetFinalParent();
			check(FinalParent)
		)
		return static_cast<T*>(AActorSingleton::GetInstance(WorldContext, T::StaticClass()));
	}

	//~ Begin AActor Interface
	virtual void OnConstruction(const FTransform& Transform) override;
	//~ End AActor Interface

private:

	/* Try to become a new single instance within current UWorld,
		* if instance already exists, call this->Destroy
		* Does nothing in few circumstances, e.g. when calling on CDO */
	void TryBecomeNewInstanceOrSelfDestroy();

	TSubclassOf<AActorSingleton> GetFinalParent();
};


/* Helper class for storing "static" references to AActorSingleton instances.
* Each subclass of AActorSingleton is expected to have only one spawned instance within each UWorld,
* that's why we use World Subsystem as it always has one instance per every UWorld. */
UCLASS(NotBlueprintable)
class ACTORSINGLETON_API UActorSingletonManager : public UWorldSubsystem
{
	GENERATED_BODY()

	friend AActorSingleton;

public:

	//~ Begin UWorldSubsystem Interface
	virtual void PostInitialize() override;
	//~ End UWorldSubsystem Interface

private:

	/* Gets all AActorSingleton in the current UWorld,
	* and calls AActorSingleton::TryBecomeNewInstanceOrSelfDestroy on all of them. */
	void FindInstancesAndDestroyDuplicates();

	/* Wrapper for UWorld::GetSubsystem<UActorSingletonManager>
	* May return 'nullptr' in case of Manager not being initialized yet. */
	static UActorSingletonManager* Get(const UObject* const WorldContext);

	UPROPERTY()
	TMap<TSubclassOf<AActorSingleton>, AActorSingleton*> Instances;
};

