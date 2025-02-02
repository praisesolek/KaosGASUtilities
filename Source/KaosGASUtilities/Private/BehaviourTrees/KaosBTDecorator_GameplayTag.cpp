﻿// Copyright ©2024 Daniel Moss. ©2024 InterKaos Games. All rights reserved.

#include "BehaviourTrees/KaosBTDecorator_GameplayTag.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagAssetInterface.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

struct FKaosBTDecorator_GameplayTagMemory
{
	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	/** Array of handles for our gameplay tag query delegates */
	TArray<TTuple<FGameplayTag, FDelegateHandle>> GameplayTagEventHandles;
};

UKaosBTDecorator_GameplayTag::UKaosBTDecorator_GameplayTag(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Kaos Gameplay Tag";

	INIT_DECORATOR_NODE_NOTIFY_FLAGS();

	// Accept only actors
	ActorForGameplayTagCheck.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UKaosBTDecorator_GameplayTag, ActorForGameplayTagCheck), AActor::StaticClass());

	// Default to using Self Actor
	ActorForGameplayTagCheck.SelectedKeyName = FBlackboard::KeySelf;
}

bool UKaosBTDecorator_GameplayTag::MatchContainer(const FGameplayTagContainer& SelectedGameplayTags) const
{
	switch (GameplayTagContainerMatchingType)
	{
	case EKaosBTDecoratorGameplayTagContainerMatching::HasAny:
		return GameplayTags.HasAny(GameplayTags);
	case EKaosBTDecoratorGameplayTagContainerMatching::HasAll:
		return GameplayTags.HasAll(GameplayTags);
	case EKaosBTDecoratorGameplayTagContainerMatching::HasAnyExact:
		return GameplayTags.HasAnyExact(GameplayTags);
	case EKaosBTDecoratorGameplayTagContainerMatching::HasAllExact:
		return GameplayTags.HasAllExact(GameplayTags);
	}
	//Should never reach here.
	ensureMsgf(false, TEXT("No valid enum for GameplayTagContainerMatchingType"));
	return false;
}

bool UKaosBTDecorator_GameplayTag::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		// Not calling super here since it returns true
		return false;
	}

	const IGameplayTagAssetInterface* GameplayTagAssetInterface = Cast<IGameplayTagAssetInterface>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(ActorForGameplayTagCheck.GetSelectedKeyID()));
	if (!GameplayTagAssetInterface)
	{
		// Not calling super here since it returns true
		return false;
	}

	FGameplayTagContainer SelectedActorTags;
	GameplayTagAssetInterface->GetOwnedGameplayTags(SelectedActorTags);

	switch (GameplayTagMatchType)
	{
	case EKaosBTDecoratorMatchType::GameplayTag:
		return bExact ? SelectedActorTags.HasTagExact(GameplayTag) : SelectedActorTags.HasTag(GameplayTag);
	case EKaosBTDecoratorMatchType::GameplayTagContainer:
		return MatchContainer(SelectedActorTags);
	}

	//Should never reach here.
	ensureMsgf(false, TEXT("No valid enum for GameplayTagMatchType"));
	return false;
}

void UKaosBTDecorator_GameplayTag::OnGameplayTagsChanged(const FGameplayTag InTag, int32 NewCount, TWeakObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent, uint8* NodeMemory)
{
	if (!BehaviorTreeComponent.IsValid())
	{
		return;
	}

	ConditionalFlowAbort(*BehaviorTreeComponent, EBTDecoratorAbortRequest::ConditionResultChanged);
}

FString UKaosBTDecorator_GameplayTag::GetStaticDescription() const
{
	switch (GameplayTagMatchType)
	{
	case EKaosBTDecoratorMatchType::GameplayTag:
		return FString::Printf(TEXT("%s: GameplayTag to check: %s"), *Super::GetStaticDescription(), *GameplayTag.ToString());
	case EKaosBTDecoratorMatchType::GameplayTagContainer:
		{
			FString TagDesc = TEXT("GameplayTags to check:\n");
			for (auto It = GameplayTags.CreateConstIterator(); It; ++It)
			{
				TagDesc += It->ToString();
				TagDesc += TEXT("\n");
			}
			return FString::Printf(TEXT("%s: %sContainer Match Type: %s"), *Super::GetStaticDescription(), *TagDesc,
			                       *StaticEnum<EKaosBTDecoratorGameplayTagContainerMatching>()->GetNameStringByValue(static_cast<int64>(GameplayTagContainerMatchingType)));;
		}
	}
	return Super::GetStaticDescription();
}

void UKaosBTDecorator_GameplayTag::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	const FKaosBTDecorator_GameplayTagMemory* MyMemory = CastInstanceNodeMemory<FKaosBTDecorator_GameplayTagMemory>(NodeMemory);
	ensureMsgf(MyMemory->GameplayTagEventHandles.Num() == 0, TEXT("Dangling gameplay tag event handles for decorator %s"), *GetStaticDescription());
}

void UKaosBTDecorator_GameplayTag::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		// Not calling super here since it does nothing
		return;
	}

	const AActor* SelectedActor = Cast<AActor>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(ActorForGameplayTagCheck.GetSelectedKeyID()));
	if (!SelectedActor)
	{
		// Not calling super here since it does nothing
		return;
	}

	FKaosBTDecorator_GameplayTagMemory* MyMemory = CastInstanceNodeMemory<FKaosBTDecorator_GameplayTagMemory>(NodeMemory);
	MyMemory->CachedAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelectedActor);

	if (MyMemory->CachedAbilitySystemComponent.IsValid())
	{
		switch (GameplayTagMatchType)
		{
		case EKaosBTDecoratorMatchType::GameplayTag:
			{
				FDelegateHandle GameplayTagEventCallbackDelegate = MyMemory->CachedAbilitySystemComponent.Get()->RegisterGameplayTagEvent(GameplayTag, EGameplayTagEventType::Type::AnyCountChange).AddUObject(
					this, &UKaosBTDecorator_GameplayTag::OnGameplayTagsChanged, TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp), NodeMemory);
				MyMemory->GameplayTagEventHandles.Emplace(GameplayTag, GameplayTagEventCallbackDelegate);
			}
			break;
		case EKaosBTDecoratorMatchType::GameplayTagContainer:
			{
				for (const FGameplayTag& CurrentTag : GameplayTags)
				{
					FDelegateHandle GameplayTagEventCallbackDelegate = MyMemory->CachedAbilitySystemComponent.Get()->RegisterGameplayTagEvent(CurrentTag, EGameplayTagEventType::Type::AnyCountChange).AddUObject(
						this, &UKaosBTDecorator_GameplayTag::OnGameplayTagsChanged, TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp), NodeMemory);
					MyMemory->GameplayTagEventHandles.Emplace(CurrentTag, GameplayTagEventCallbackDelegate);
				}
			}
			break;
		}
	}
}

void UKaosBTDecorator_GameplayTag::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FKaosBTDecorator_GameplayTagMemory* MyMemory = CastInstanceNodeMemory<FKaosBTDecorator_GameplayTagMemory>(NodeMemory);

	if (MyMemory->CachedAbilitySystemComponent.IsValid())
	{
		for (const TTuple<FGameplayTag, FDelegateHandle>& GameplayTagEvent : MyMemory->GameplayTagEventHandles)
		{
			MyMemory->CachedAbilitySystemComponent.Get()->RegisterGameplayTagEvent(GameplayTagEvent.Key, EGameplayTagEventType::Type::AnyCountChange).Remove(GameplayTagEvent.Value);
		}
	}

	MyMemory->GameplayTagEventHandles.Reset();
	MyMemory->CachedAbilitySystemComponent = nullptr;
}

uint16 UKaosBTDecorator_GameplayTag::GetInstanceMemorySize() const
{
	return sizeof(FKaosBTDecorator_GameplayTagMemory);
}

void UKaosBTDecorator_GameplayTag::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	const UBlackboardData* BBAsset = GetBlackboardAsset();
	if (ensure(BBAsset))
	{
		ActorForGameplayTagCheck.ResolveSelectedKey(*BBAsset);
	}
}
