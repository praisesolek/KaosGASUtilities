﻿// Copyright ©2024 Daniel Moss. ©2024 InterKaos Games. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "GameplayTagContainer.h"
#include "KaosBTDecorator_GameplayTagQuery.generated.h"

/**
 * Decorator to match against a GameplayTag Query.
 * Conditions is hidden due this being a query.
 */
UCLASS(MinimalAPI, HideCategories=(Condition))
class UKaosBTDecorator_GameplayTagQuery : public UBTDecorator
{
	GENERATED_BODY()


	UKaosBTDecorator_GameplayTagQuery(const FObjectInitializer& ObjectInitializer);

	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** Callback for when a tag in our query changes */
	void OnGameplayTagInQueryChanged(const FGameplayTag InTag, int32 NewCount, TWeakObjectPtr<UBehaviorTreeComponent> BehaviorTreeComponent, uint8* NodeMemory);

	virtual FString GetStaticDescription() const override;

	virtual void CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const override;

protected:
	UPROPERTY(EditAnywhere, Category=GameplayTagQuery, Meta=(ToolTips="Which Actor (from the blackboard) should be checked for this gameplay tag query?"))
	FBlackboardKeySelector ActorForGameplayTagQuery;

	/** Gameplay tag query to match */
	UPROPERTY(EditAnywhere, Category=GameplayTagQuery)
	FGameplayTagQuery GameplayTagQuery;

	UPROPERTY()
	TArray<FGameplayTag> QueryTags;

	/** called when execution flow controller becomes active */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** called when execution flow controller becomes inactive */
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual uint16 GetInstanceMemorySize() const override;

#if WITH_EDITOR
	/** Get the array of tags onto which we need to add delegates 
		The gameplay tag interface forces us to allocate everytime we 
		want to know the list of gameplay tags inside a query so we 
		do this only once and cache it.
	*/
	virtual void CacheGameplayTagsInsideQuery();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
