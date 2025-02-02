﻿// Copyright ©2024 Daniel Moss. ©2024 InterKaos Games. All rights reserved.

#include "BehaviourTrees/KaosBTDecorator_GameplayTagQuery.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KaosBTDecorator_GameplayTagQuery)

struct FKaosBTDecorator_GameplayTagQueryMemory
{
	TWeakObjectPtr<UAbilitySystemComponent> CachedAbilitySystemComponent;

	/** Array of handles for our gameplay tag query delegates */
	TArray<TTuple<FGameplayTag, FDelegateHandle>> GameplayTagEventHandles;
};

UKaosBTDecorator_GameplayTagQuery::UKaosBTDecorator_GameplayTagQuery(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Kaos Gameplay Tag Query";

	INIT_DECORATOR_NODE_NOTIFY_FLAGS();

	// Accept only actors
	ActorForGameplayTagQuery.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UKaosBTDecorator_GameplayTagQuery, ActorForGameplayTagQuery), AActor::StaticClass());

	// Default to using Self Actor
	ActorForGameplayTagQuery.SelectedKeyName = FBlackboard::KeySelf;
}

bool UKaosBTDecorator_GameplayTagQuery::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		// Not calling super here since it returns true
		return false;
	}

	const IGameplayTagAssetInterface* GameplayTagAssetInterface = Cast<IGameplayTagAssetInterface>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(ActorForGameplayTagQuery.GetSelectedKeyID()));
	if (!GameplayTagAssetInterface)
	{
		// Not calling super here since it returns true
		return false;
	}

	FGameplayTagContainer SelectedActorTags;
	GameplayTagAssetInterface->GetOwnedGameplayTags(SelectedActorTags);

	return GameplayTagQuery.Matches(SelectedActorTags);
}

void UKaosBTDecorator_GameplayTagQuery::OnGameplayTagInQueryChanged(const FGameplayTag InTag, int32 NewCount, TWeakObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent, uint8* NodeMemory)
{
	if (!BehaviorTreeComponent.IsValid())
	{
		return;
	}

	ConditionalFlowAbort(*BehaviorTreeComponent, EBTDecoratorAbortRequest::ConditionResultChanged);
}

FString UKaosBTDecorator_GameplayTagQuery::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *GameplayTagQuery.GetDescription());
}

void UKaosBTDecorator_GameplayTagQuery::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	const FKaosBTDecorator_GameplayTagQueryMemory* MyMemory = CastInstanceNodeMemory<FKaosBTDecorator_GameplayTagQueryMemory>(NodeMemory);
	ensureMsgf(MyMemory->GameplayTagEventHandles.Num() == 0, TEXT("Dangling gameplay tag event handles for decorator %s"), *GetStaticDescription());
}

void UKaosBTDecorator_GameplayTagQuery::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		// Not calling super here since it does nothing
		return;
	}

	const AActor* SelectedActor = Cast<AActor>(BlackboardComp->GetValue<UBlackboardKeyType_Object>(ActorForGameplayTagQuery.GetSelectedKeyID()));
	if (!SelectedActor)
	{
		// Not calling super here since it does nothing
		return;
	}

	FKaosBTDecorator_GameplayTagQueryMemory* MyMemory = CastInstanceNodeMemory<FKaosBTDecorator_GameplayTagQueryMemory>(NodeMemory);
	MyMemory->CachedAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelectedActor);

	if (MyMemory->CachedAbilitySystemComponent.IsValid())
	{
		for (const FGameplayTag& CurrentTag : QueryTags)
		{
			FDelegateHandle GameplayTagEventCallbackDelegate = MyMemory->CachedAbilitySystemComponent.Get()->RegisterGameplayTagEvent(CurrentTag, EGameplayTagEventType::Type::AnyCountChange).AddUObject(
				this, &UKaosBTDecorator_GameplayTagQuery::OnGameplayTagInQueryChanged, TWeakObjectPtr<UBehaviorTreeComponent>(&OwnerComp), NodeMemory);
			MyMemory->GameplayTagEventHandles.Emplace(CurrentTag, GameplayTagEventCallbackDelegate);
		}
	}
}

void UKaosBTDecorator_GameplayTagQuery::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FKaosBTDecorator_GameplayTagQueryMemory* MyMemory = CastInstanceNodeMemory<FKaosBTDecorator_GameplayTagQueryMemory>(NodeMemory);

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

uint16 UKaosBTDecorator_GameplayTagQuery::GetInstanceMemorySize() const
{
	return sizeof(FKaosBTDecorator_GameplayTagQueryMemory);
}

#if WITH_EDITOR
void UKaosBTDecorator_GameplayTagQuery::CacheGameplayTagsInsideQuery()
{
	QueryTags.Reset();
	GameplayTagQuery.GetGameplayTagArray(QueryTags);
}

void UKaosBTDecorator_GameplayTagQuery::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property == nullptr)
	{
		return;
	}

	CacheGameplayTagsInsideQuery();
}
#endif	// WITH_EDITOR

void UKaosBTDecorator_GameplayTagQuery::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	const UBlackboardData* BBAsset = GetBlackboardAsset();
	if (ensure(BBAsset))
	{
		ActorForGameplayTagQuery.ResolveSelectedKey(*BBAsset);
	}
}
