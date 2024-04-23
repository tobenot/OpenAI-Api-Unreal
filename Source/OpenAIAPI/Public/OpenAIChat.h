// Copyright (c) 2023 tobenot. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "OpenAIDefinitions.h"
#include "OpenAIChat.generated.h"

DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnResponseReceivedPin, const FChatCompletion&, Message, const FString&, ErrorMessage, bool, Success);
DECLARE_DELEGATE_ThreeParams(FOnResponseReceivedF, const FChatCompletion&, const FString&, bool);
DECLARE_LOG_CATEGORY_EXTERN(LogChat, Log, All);

/**
 * 
 */
UCLASS(BlueprintType)
class OPENAIAPI_API UOpenAIChat : public UObject
{
	GENERATED_BODY()

public:
	UOpenAIChat();
	~UOpenAIChat();

	UFUNCTION(BlueprintCallable, Category = "OpenAI")
	static UOpenAIChat* CreateChatInstance();

	void Init(const FChatSettings& chatSettings);

	UFUNCTION(BlueprintCallable, Category = "OpenAI")
	void StartChat();
	
	UFUNCTION(BlueprintCallable, Category = "OpenAI")
	void CancelRequest();

	UPROPERTY()
	FOnResponseReceivedPin OnResponseReceived;
	
	FOnResponseReceivedF OnResponseReceivedF;

private:
	FChatSettings ChatSettings;

	void OnResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool WasSuccessful);
public:
	static UOpenAIChat* Chat(const FChatSettings& ChatSettings, TFunction<void(const FChatCompletion& Message, const FString& ErrorMessage, bool Success)> Callback);

private:
	void HandleRequestProgress(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived);

private:
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;
};