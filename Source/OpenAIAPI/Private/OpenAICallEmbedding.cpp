// Copyright (c) 2024 tobenot, See LICENSE in the project root for license information.


// UOpenAICallEmbedding.cpp

#include "OpenAICallEmbedding.h"
#include "OpenAIUtils.h" // If there are any utility methods needed, include the appropriate headers

UOpenAICallEmbedding::UOpenAICallEmbedding()
	: OpenAIEmbeddingInstance(nullptr)
{
}

UOpenAICallEmbedding::~UOpenAICallEmbedding()
{
	if (OpenAIEmbeddingInstance)
	{
		OpenAIEmbeddingInstance->ConditionalBeginDestroy();
	}
}

UOpenAICallEmbedding* UOpenAICallEmbedding::OpenAICallEmbedding(const FVector& EmbeddingSettingsInput)
{
	// Make sure to use the correct type for embedding settings instead of FVector
	UOpenAICallEmbedding* BPNode = NewObject<UOpenAICallEmbedding>();
	BPNode->EmbeddingSettings = EmbeddingSettingsInput; // Will need adjustments
	return BPNode;
}

void UOpenAICallEmbedding::Activate()
{
	if (!OpenAIEmbeddingInstance)
	{
		OpenAIEmbeddingInstance = UOpenAIEmbedding::CreateEmbeddingInstance();
	}

	// Similar to UOpenAICallChat, set up the embedding settings and binding to the callback
	// ...

	OpenAIEmbeddingInstance->StartEmbedding();
}

void UOpenAICallEmbedding::OnResponse(const FEmbeddingResult& Result, const FString& ErrorMessage, bool Success)
{
	OpenAIEmbeddingInstance->OnResponseReceived.Unbind();
	Finished.Broadcast(Result, ErrorMessage, Success);
	OpenAIEmbeddingInstance->ConditionalBeginDestroy();
	OpenAIEmbeddingInstance = nullptr;
}