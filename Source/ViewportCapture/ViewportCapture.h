// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"

#include "ViewportCapture.generated.h"

UCLASS(BlueprintType)
class VIEWPORTCAPTURE_API UViewportCapture: public UObject {
	GENERATED_BODY()

	// Blueprint functions
public:
	UFUNCTION(BlueprintCallable)
	void StartCapturing();

	// Blueprint properties
public:
	UPROPERTY(BlueprintReadWrite)
	UTextureRenderTarget2D* RenderTextureTarget;

	// Internal callback called after rendering without UI on render thread
private:
	void Capture_RenderThread(FRDGBuilder& RDGBuilder);
};
