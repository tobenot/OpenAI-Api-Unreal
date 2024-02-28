// Copyright Kellan Mythen 2023. All rights Reserved.

#include "OpenAICallChat.h"
#include "OpenAIUtils.h"

UOpenAICallChat::UOpenAICallChat()
	: OpenAIChatInstance(nullptr)
{
}

UOpenAICallChat::~UOpenAICallChat()
{
	// Ensure that OpenAIChatInstance is destroyed when UOpenAICallChat is destroyed
	if (OpenAIChatInstance)
	{
		OpenAIChatInstance->ConditionalBeginDestroy();
	}
}

UOpenAICallChat* UOpenAICallChat::OpenAICallChat(FChatSettings chatSettingsInput)
{
	UOpenAICallChat* BPNode = NewObject<UOpenAICallChat>();
	BPNode->chatSettings = chatSettingsInput;
	return BPNode;
}

void UOpenAICallChat::Activate()
{
	// Create OpenAIChatInstance only when Activate is called
	if (!OpenAIChatInstance)
	{
		OpenAIChatInstance = UOpenAIChat::CreateChatInstance();
	}
	OpenAIChatInstance->Init(chatSettings);
	
	// Bind the OnResponse function to the OnResponseReceived delegate
	OpenAIChatInstance->OnResponseReceived.BindDynamic(this, &UOpenAICallChat::OnResponse);
	
	// Start the chat
	OpenAIChatInstance->StartChat();
}

void UOpenAICallChat::OnResponse(const FChatCompletion& Message,const FString& ErrorMessage, bool Success)
{
	// Unbind the OnResponse function from the OnResponseReceived delegate
	OpenAIChatInstance->OnResponseReceived.Unbind();

	// Broadcast the result
	Finished.Broadcast(Message, ErrorMessage, Success);

	// If desired, destroy OpenAIChatInstance after receiving the response
	OpenAIChatInstance->ConditionalBeginDestroy();
	OpenAIChatInstance = nullptr;
}
