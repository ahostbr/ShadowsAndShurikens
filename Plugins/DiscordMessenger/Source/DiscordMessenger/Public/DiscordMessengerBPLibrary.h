// Copyright © 2023 David ten Kate - All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "DiscordMessengerBPLibrary.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(DiscordMessenger, Log, All);

// Define the author of the message
USTRUCT(BlueprintType, Category="DiscordMessenger")
struct FDiscordAuthor
{
	GENERATED_BODY()
	
	// Name of the author
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Name;

	// URL that will be embedded in the authors title
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Url;
	
	// URL to grab the icon of the author from
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString IconUrl;

	// Check if the Discord author struct has been defined (internal use)
	bool IsEmpty() const
	{
		return Name.IsEmpty() && Url.IsEmpty() && IconUrl.IsEmpty();
	}
};

// Define fields to include with the embed
USTRUCT(BlueprintType, Category="DiscordMessenger")
struct FDiscordField
{
	GENERATED_BODY()
	
	// Name of the field
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Name;

	// Text of the field
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Value;
	
	// Should the field be inline in the embed?
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	bool IsInline = false;

	// Check if the Discord field struct has been defined (internal use)
	bool IsEmpty() const
	{
		return Name.IsEmpty() && Value.IsEmpty();
	}
};

// Include an embed with the message
USTRUCT(BlueprintType, Category="DiscordMessenger")
struct FDiscordEmbed
{
	GENERATED_BODY()
	
	// Title of the embed
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Title;

	// Description of the embed
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Description;

	// Color of the embed
	UPROPERTY(BlueprintReadWrite, Category = "Discord")
	FLinearColor Color = FLinearColor::White;
	
	// URL that will embedded in the title of the embed
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Url;

	// Include the author of the message
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FDiscordAuthor Author;

	// Include fields in the embed
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	TArray<FDiscordField> Fields;

	// Embed an image in the embed (only requires the name of one of the files that is included with the message)
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Image;

	// Include a footer in the embed
	UPROPERTY(BlueprintReadWrite, Category="Discord")
	FString Footer;

	// Check if the embed has been defined (internal use)
	bool IsEmpty() const
	{
		return Title.IsEmpty() && Description.IsEmpty() && Url.IsEmpty() && Author.IsEmpty() && 
			Fields.Num() <= 0 && Image.IsEmpty() && Footer.IsEmpty();
	}
};

UCLASS()
class UDiscordMessengerBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	/**
	 * Send a message to a Discord webhook
	 * @param Webhook The URL of the Discord webhook to send the message to
	 * @param Content The message to include (optional as long as an embed or atleast one file is included with the message)
	 * @param Embed Include an embed with the message (optional)
	 * @param Files Include files with the message (optional, max limit of files is 10, max file size is 8MB)
	 */
	UFUNCTION(BlueprintCallable, Category="DiscordMessenger", meta=(AutoCreateRefTerm="Embed, Files"))
	static void SendDiscordMessage(
		const FString& Webhook,
		const FString& Content,
		const FDiscordEmbed& Embed,
		const TArray<FString>& Files
		);
};