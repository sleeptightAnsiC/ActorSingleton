// Published under MIT License, created by https://github.com/sleeptightAnsiC

#include "ActorSingleton.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/StructuredLog.h"
#include "Misc/MessageDialog.h"

#if WITH_EDITOR
#include "Subsystems/EditorActorSubsystem.h"
#include "Editor.h"
#endif //WITH_EDITOR

IMPLEMENT_MODULE(FActorSingletonModule, ActorSingleton)

DEFINE_LOG_CATEGORY(ActorSingleton);


void AActorSingleton::TryBecomeNewInstanceOrSelfDestroy()
{
	/* Do nothing, if 'this' is either...
	*	...not valid (such case has never happened but always worth cathing)
	*	...being destroyed (IsValid does NOT catch this in some cases)
	*	...marked as Transient (we omit "dummy" Actors that are often being used by the Editor) */
	if(
		!ensure(IsValid(this))
		|| this->IsActorBeingDestroyed()
		|| this->HasAnyFlags(EObjectFlags::RF_Transient)
		)
	{
		return;
	}

	/* Do nothing, if 'this' is CDO */
	if (this == GetClass()->GetDefaultObject())
	{
		return;
	}

	UWorld* ThisWorld = GetWorld();

	auto* ActorSingletonManager = UActorSingletonManager::Get(ThisWorld);

	/* UActorSingletonManager::Get can fail (and this is expected)
	* There are cases where UActorSingletonManager might not be Initialized yet,
	*	e.g. during AActor::OnConstruction when opening Map in the Editor.
	* We deal with said problem by re-firing this function later in the UActorSingletonManager::PostInitialize
	* This is not an ideal solution but works fine for now, see UActorSingletonManager::PostInitialize */
	if(!ActorSingletonManager)
	{
		return;
	}

	TSubclassOf<AActorSingleton> ParentClass = GetFinalParent();

	if(!ensure(ParentClass))
	{
		return;
	}

	TMap<TSubclassOf<AActorSingleton>, AActorSingleton*>& InstancesMap = ActorSingletonManager->Instances;
	if (!InstancesMap.Contains(ParentClass))
	{
		InstancesMap.Add(ParentClass, nullptr);
	}
	AActorSingleton*& CurrentInstance = InstancesMap[ParentClass];

	if (this == CurrentInstance)
	{
		return;
	}

	/* You are allowed to destroy singleton instance on your own,
	*	so we expect that reference to the instance may not be valid anymore.
	* In this case, start treating 'this' as new singleton instance. */
	if (!IsValid(CurrentInstance))
	{
		CurrentInstance = this;

		UE_LOGFMT(ActorSingleton, Warning,
			"'{ActorName}' is now a Singleton instance of class '{ClassName}' in the World '{WorldName}'! "
			"Adding/Spawning more instances of the same class in the same World will resul in them being destroyed!",
			AActor::GetDebugName(this), ParentClass->GetFName(), ThisWorld->GetFName());

		return;
	}

	/* At this point we know that 'this' is a duplicate and we gonna destroy it so let's log an error about it.
	* We consider such case as an error, because when it happens, you're doing something wrong. */
	UE_LOGFMT(ActorSingleton, Error,
		"World '{WorldName}' can have only one instance of '{ClassName}'! Destroying '{ActorName}' ...",
		ThisWorld->GetFName(), ParentClass->GetFName(), AActor::GetDebugName(this));

#if WITH_EDITOR
	/* In case of placing an Actor in the Level Viewport, we canNOT simply Destroy it.
	* Instead, we must "tell" the Editor to delete it, which will fire some additional clean up logic.
	* Also we're showing a message to the Editor user telling what is happening, so we can avoid confusion.
	*
	* FIXME: Current implementation is fine but has few caveats:
	* 1. it "touches" the Level despite that no actual changes have been done
	* 2. if user uses 'undo' after deletion, the duplicate object will be restored
	* 3. if user's Actor does something after being placed, we won't be able to revert it
	* These are pretty bad... I should probably find another way to achieve the same goal.
	*
	* TODO: Possible solutions for the issues listed above:
	* 1. Prevent Actor from being placed into the Level in the first place
	* 	I have no idea if this can be achieved with Unreal Engine...
	* 	If that's possible, then there is probably some kind of Interface in AActor that allows it
	* 		but otherwise I have no clue where to look for solution
	* 2. Instead of deletion, we can simply use Editor's 'undo' feature
	* 	The problem with this one is that we would never know for sure how many times call the 'undo'
	* 		because Actor, when placed, can do some other stuff around which adds up to the undo/redo buffer.
	*	We would need to find out how many times we need to call the 'undo'
	*	However, this option seems the most promising and possible to implement :)
	*/
	if (ThisWorld->IsEditorWorld() && !ThisWorld->IsPlayInEditor())
	{
		/* Show Dialogue Message */
		const FText MessageTitle = this->GetMessageTitle();
		const FText MessageBody = this->GetMessageBody();
		FMessageDialog::Debugf(MessageBody, MessageTitle);

		/* Delete 'this' via UEditorActorSubsystem */
		auto* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
		check(EditorActorSubsystem)
		EditorActorSubsystem->ClearActorSelectionSet();
		EditorActorSubsystem->SetActorSelectionState(this, true);
		EditorActorSubsystem->DeleteSelectedActors(ThisWorld);
		GEngine->ForceGarbageCollection(true);

		/* Garbage Actor still seems to be selected in the Details Panel despite already being destroyed.
		* 'UEditorActorSubsystem::DeleteSelectedActors' doesn't handle this by itself,
		* so we are clearing the Actor selection on the very next tick which fixes this issue. */
		ThisWorld->GetTimerManager().SetTimerForNextTick(
			[&]()->void
			{
				EditorActorSubsystem->SelectNothing();
			}
		);

		return;
	}
#endif //WITH_EDITOR

	/* If the function call still keeps goint till this point,
	*	it means we can just safely Destroy the Actor. */
	this->Destroy(true, true);
}


/* virtual */ FText AActorSingleton::GetMessageTitle_Implementation() const
{
	 return FText::FromString("ActorSingleton - Destroyed Duplicate");
}


/* virtual */ FText AActorSingleton::GetMessageBody_Implementation() const
{
	return FText::FromString(
		"Duplicate instance was found and will be destroyed!"
		"\nThere is already one instance existing in current UWorld!"
		"\n(check log for more detailed error)"
		);
}


/* static */ AActorSingleton* AActorSingleton::GetInstance(const UObject* const  WorldContext, TSubclassOf<AActorSingleton> Class)
{
	/* I don't really remember why I placed 'ensure' here but for sure I had a good reason.
	* Now when I read this code it makes more sense to just crash in this place
	* 	since you're most likely doing something wrong by passing invalid WorldContext.
	* FIXME: Add proper note or replace 'ensure' with 'check' */
	if (!ensure(IsValid(WorldContext)))
	{
		return nullptr;
	}

	const auto* ActorSingletonManager = UActorSingletonManager::Get(WorldContext);

	/* This case can happen but is very rare
	*		and I don't really understand why this is possible in the first place.
	* It has happened to me when compiling a Blueprint of an Actor that was placed in the Level Viewport
	*		or when opening Content Browser with the same Blueprint. */
	if (!ensure(IsValid(ActorSingletonManager)))
	{
		return nullptr;
	}

	auto& InstancesMap = ActorSingletonManager->Instances;
	AActorSingleton* CDO = static_cast<AActorSingleton*>(Class->GetDefaultObject());
	TSubclassOf<AActorSingleton> ParentClass = CDO->GetFinalParent();
	if (
		ensure(ParentClass)
		&& InstancesMap.Contains(ParentClass)
		)
	{
		return InstancesMap[ParentClass];
	}

	return nullptr;
}


/* virtual override */ void AActorSingleton::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	TryBecomeNewInstanceOrSelfDestroy();
}


TSubclassOf<AActorSingleton> AActorSingleton::GetFinalParent()
{
	TArray<TSubclassOf<AActorSingleton>> InheritanceChain;
	/* Go through the UClass::GetSuperClass chain, from current class to 'AActorSingleton',
	* and store said chain as we gonna traverse it backwards later. */
	for (TSubclassOf<AActorSingleton> ItClass = GetClass(); ItClass != AActorSingleton::StaticClass(); ItClass = ItClass->GetSuperClass())
	{
		InheritanceChain.Add(ItClass);
	}
	// InheritanceChain.Add(AActorSingleton::StaticClass());

	/* Traverse throught ClassInheritanceChain from the back (top parrent) to the front (ThisClass)
	* We do this to find the highest parrent class that isn't Abstract,
	*	in such case we stop traversing and start treating said class as new 'ThisClass'. */
	for (int32 i = InheritanceChain.Num() - 1; i >= 0; --i)
	{
		UObject* ItCDO = InheritanceChain[i]->GetDefaultObject();
		const bool bItFinalParent = static_cast<AActorSingleton*>(ItCDO)->IsFinalParent();
		if (bItFinalParent)
		{
			return InheritanceChain[i];
		}
	}

	/* No nonAbstract Class found */
	return nullptr;
}


/* static */ UActorSingletonManager* UActorSingletonManager::Get(const UObject* const WorldContext)
{
	check(IsValid(WorldContext))
	const UWorld* const World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::Assert);
	return World->GetSubsystem<UActorSingletonManager>();
}


void UActorSingletonManager::FindInstancesAndDestroyDuplicates()
{
	TArray<AActor*> PossibleInstances;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActorSingleton::StaticClass(), PossibleInstances);
	for (AActor* Actor : PossibleInstances)
	{
		static_cast<AActorSingleton*>(Actor)->TryBecomeNewInstanceOrSelfDestroy();
	}
}


/* virtual override */ void UActorSingletonManager::PostInitialize()
{
	Super::PostInitialize();
	FindInstancesAndDestroyDuplicates();
}
