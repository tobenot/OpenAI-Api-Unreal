// Copyright (c) 2023 tobenot. All rights reserved.

#include "OpenAIChat.h"
#include "HttpModule.h"
#include "Http.h"
#include "OpenAIUtils.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Delegates/DelegateCombinations.h"
#include "OpenAIParser.h"

DEFINE_LOG_CATEGORY(LogChat);

UOpenAIChat::UOpenAIChat()
{
}

UOpenAIChat* UOpenAIChat::CreateChatInstance()
{
	return NewObject<UOpenAIChat>();
}

void UOpenAIChat::Init(const FChatSettings& chatSettings)
{
	ChatSettings = chatSettings;
}

void UOpenAIChat::StartChat()
{
	UE_LOG(LogChat, Log, TEXT("UOpenAIChat StartChat"));
	FString _apiKey;
	if (UOpenAIUtils::getUseApiKeyFromEnvironmentVars())
		_apiKey = UOpenAIUtils::GetEnvironmentVariable(TEXT("OPENAI_API_KEY"));
	else
		_apiKey = UOpenAIUtils::getApiKey();

	// checking parameters are valid
	if (_apiKey.IsEmpty())
	{
		OnResponseReceived.ExecuteIfBound({}, TEXT("Api key is not set"), false);
		OnResponseReceivedF.ExecuteIfBound({}, TEXT("Api key is not set"), false);
	}
	else
	{
		auto HttpRequest = FHttpModule::Get().CreateRequest();

		FString apiMethod;
		switch (ChatSettings.model)
		{
		case EOAChatEngineType::GPT_3_5_TURBO:
			apiMethod = "gpt-3.5-turbo";
			break;
		case EOAChatEngineType::GPT_4:
			apiMethod = "gpt-4";
			break;
		case EOAChatEngineType::GPT_4_32k:
			apiMethod = "gpt-4-32k";
			break;
		case EOAChatEngineType::GPT_4_TURBO:
			apiMethod = "gpt-4-0125-preview";
			break;
		}
		// TODO: Add additional params to match the ones listed in the curl response in: https://platform.openai.com/docs/api-reference/making-requests

		// convert parameters to strings
		FString tempHeader = "Bearer ";
		tempHeader += _apiKey;

		// set headers
		FString url = FString::Printf(TEXT("%s/chat/completions"), *UOpenAIUtils::GetApiBaseUrl());
		HttpRequest->SetURL(url);
		HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
		HttpRequest->SetHeader(TEXT("Authorization"), tempHeader);

		// build payload
		TSharedPtr<FJsonObject> _payloadObject = MakeShareable(new FJsonObject());
		_payloadObject->SetStringField(TEXT("model"), apiMethod);
		_payloadObject->SetNumberField(TEXT("max_tokens"), ChatSettings.maxTokens);

		// convert role enum to model string
		if (!(ChatSettings.messages.Num() == 0))
		{
			TArray<TSharedPtr<FJsonValue>> Messages;
			FString role;
			for (int i = 0; i < ChatSettings.messages.Num(); i++)
			{
				TSharedPtr<FJsonObject> Message = MakeShareable(new FJsonObject());
				switch (ChatSettings.messages[i].role)
				{
				case EOAChatRole::USER:
					role = "user";
					break;
				case EOAChatRole::ASSISTANT:
					role = "assistant";
					break;
				case EOAChatRole::SYSTEM:
					role = "system";
					break;
				}
				Message->SetStringField(TEXT("role"), role);
				Message->SetStringField(TEXT("content"), ChatSettings.messages[i].content);
				Messages.Add(MakeShareable(new FJsonValueObject(Message)));
			}
			_payloadObject->SetArrayField(TEXT("messages"), Messages);
		}

		// set the json-format setting
		if (ChatSettings.jsonFormat)
		{
			const TSharedPtr<FJsonObject> JsonObj = MakeShareable(new FJsonObject());
			JsonObj->SetStringField(TEXT("type"), "json_object");
			_payloadObject->SetObjectField(TEXT("response_format"), JsonObj);
		}

		// convert payload to string
		FString _payload;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&_payload);
		FJsonSerializer::Serialize(_payloadObject.ToSharedRef(), Writer);

		// commit request
		HttpRequest->SetVerb(TEXT("POST"));
		HttpRequest->SetContentAsString(_payload);

		UE_LOG(LogChat, Log, TEXT("UOpenAIChat ProcessHttpRequest"));

		// ensure fast connection, I will retry it
		HttpRequest->SetTimeout(10.f);
		
		HttpRequest->OnRequestProgress().BindUObject(this, &UOpenAIChat::HandleRequestProgress);
		HttpRequest->OnProcessRequestComplete().BindUObject(this, &UOpenAIChat::OnResponse);
		UE_LOG(LogChat, Log, TEXT("UOpenAIChat BindProcessRequestComplete"));
		
		if (HttpRequest->ProcessRequest())
		{
			UE_LOG(LogChat, Log, TEXT("UOpenAIChat StartProcessRequest"));
		}
		else
		{
			OnResponseReceived.ExecuteIfBound({}, TEXT("Error sending request"), false);
			OnResponseReceivedF.ExecuteIfBound({}, TEXT("Error sending request"), false);
		}
	}
}

void UOpenAIChat::OnResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool WasSuccessful)
{
	UE_LOG(LogChat, Log, TEXT("UOpenAIChat OnResponse"));
	// print response as debug message
	if (!WasSuccessful)
	{
		if(Response)
		{
			UE_LOG(LogChat, Warning, TEXT("Error processing request. \n%s \n%s"), *Response->GetContentAsString(), *Response->GetURL());
			OnResponseReceived.ExecuteIfBound({}, *Response->GetContentAsString(), false);
			OnResponseReceivedF.ExecuteIfBound({}, *Response->GetContentAsString(), false);
			return;
		}else
		{
			const FString ErrorMessage = "Error processing request. Response is nullptr.";
			UE_LOG(LogChat, Warning, TEXT("%s"), *ErrorMessage);
			OnResponseReceived.ExecuteIfBound({}, ErrorMessage, false);
			OnResponseReceivedF.ExecuteIfBound({}, ErrorMessage, false);
			return;
		}
		
	}

	TSharedPtr<FJsonObject> responseObject;
	TSharedRef<TJsonReader<>> reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
	if (FJsonSerializer::Deserialize(reader, responseObject))
	{
		bool err = responseObject->HasField("error");

		if (err)
		{
			FString errorMessage = "";
			TSharedPtr<FJsonObject> errorObject = responseObject->GetObjectField("error");
			if (errorObject->HasField("message"))
			{
				errorMessage = errorObject->GetStringField("message");
			}
			UE_LOG(LogChat, Warning, TEXT("%s"), *Response->GetContentAsString());
			OnResponseReceived.ExecuteIfBound({}, FString::Printf(TEXT("Api error %s"), *errorMessage), false);
			OnResponseReceivedF.ExecuteIfBound({}, FString::Printf(TEXT("Api error %s"), *errorMessage), false);
			return;
		}

		OpenAIParser parser(ChatSettings);
		FChatCompletion _out = parser.ParseChatCompletion(*responseObject);

		OnResponseReceived.ExecuteIfBound(_out, "", true);
		OnResponseReceivedF.ExecuteIfBound(_out, "", true);
	}
}

UOpenAIChat* UOpenAIChat::Chat(const FChatSettings& ChatSettings,
	TFunction<void(const FChatCompletion& Message, const FString& ErrorMessage,  bool Success)> Callback)
{
	UOpenAIChat* OpenAIChatInstance = CreateChatInstance();
	OpenAIChatInstance->Init(ChatSettings);

	//OpenAIChatInstance->AddToRoot();
	
	// Use Lambda as the callback function
	auto OnResponseCallback = [Callback, OpenAIChatInstance](const FChatCompletion& Message, const FString& ErrorMessage, bool Success)
	{
		// Automatically print ErrorMessage if not successful
		if (!Success)
		{
			UE_LOG(LogChat, Error, TEXT("Chat request failed. Error: %s"), *ErrorMessage);
		}

		if (Callback)
		{
			// Call the provided callback function
			Callback(Message, ErrorMessage, Success);
		}
		else
		{
			// Handle the case where Callback is null (optional)
			UE_LOG(LogChat, Error, TEXT("Chat Callback is null."));
		}
		
		//OpenAIChatInstance->RemoveFromRoot();
		
		// Destroy OpenAIChatInstance after receiving the response
		OpenAIChatInstance->ConditionalBeginDestroy();
	};

	// Bind the lambda callback function to OnResponseReceived delegate
	OpenAIChatInstance->OnResponseReceivedF.BindLambda(OnResponseCallback);

	// Start the chat
	OpenAIChatInstance->StartChat();

	return OpenAIChatInstance;
}

void UOpenAIChat::HandleRequestProgress(FHttpRequestPtr Request, int32 BytesSent, int32 BytesReceived)
{
	UE_LOG(LogChat, Log, TEXT("UOpenAIChat Heartbeat - Sent: %d, Received: %d"), BytesSent, BytesReceived);
}