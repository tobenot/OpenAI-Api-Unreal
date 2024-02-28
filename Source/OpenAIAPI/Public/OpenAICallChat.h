// Copyright Kellan Mythen 2023. All rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "OpenAIDefinitions.h"
#include "OpenAIChat.h"
#include "OpenAICallChat.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnResponseRecieved, const FChatCompletion, message, const FString&, errorMessage, bool, Success);

/**
 * 
 */
UCLASS()
class OPENAIAPI_API UOpenAICallChat : public UBlueprintAsyncActionBase
{
public:
	GENERATED_BODY()

public:
	UOpenAICallChat();
	~UOpenAICallChat();

	FChatSettings chatSettings;

	UPROPERTY(BlueprintAssignable, Category = "OpenAI")
	FOnResponseRecieved Finished;

private:
	UPROPERTY()
	UOpenAIChat* OpenAIChatInstance;

	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = "OpenAI")
	static UOpenAICallChat* OpenAICallChat(FChatSettings chatSettings);

	virtual void Activate() override;

	UFUNCTION()
	void OnResponse(const FChatCompletion& Message,const FString& ErrorMessage, bool Success);
};