// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "QRCodeBlueprintFunctionLibrary.generated.h"


UENUM()
enum class QR_IMAGE_FORMAT : uint8
{
	/** Portable Network Graphics. */
	PNG = 0,

	/** Joint Photographic Experts Group. */
	JPEG = 1,
};


/**
 * 
 */
UCLASS()
class QRCODE_API UQRCodeBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "UQRCodeBlueprintFunctionLibrary|GenerateQRCodeBitmap")
		static void GenerateQRCodeBitmap(const int32& Width, const int32& Height, const FString& Name, const FString& Outfile, int32 Margin = 0);

	UFUNCTION(BlueprintCallable, Category = "UQRCodeBlueprintFunctionLibrary|GenerateQRCodeTexture")
		static UTexture2D* GenerateQRCodeTexture(const int32& Width, const int32& Height, const FString& Name, int32 Margin = 0);

	UFUNCTION(BlueprintCallable, Category = "UQRCodeBlueprintFunctionLibrary|GenerateQRCodeBitmap")
		static bool GenerateQRCodeImageByType(const int32& Width, const int32& Height, const FString& Name, const FString& Outfile, QR_IMAGE_FORMAT ImageFormat, int32 Margin = 0);

};

