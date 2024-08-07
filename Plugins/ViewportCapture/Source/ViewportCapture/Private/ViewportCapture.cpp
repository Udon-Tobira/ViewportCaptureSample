// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewportCapture.h"

#include "CommonRenderResources.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/AssertionMacros.h"
#include "Modules/ModuleManager.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ScreenRendering.h"
#include "Slate/SceneViewport.h"

// IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ViewportCapture,
//                               "ViewportCapture");
DEFINE_LOG_CATEGORY_STATIC(LogViewportCapture, Display, All);

void UViewportCapture::StartCapturing() {
	// if callback is not bound
	if (!PostRenderDelegateExHandle.IsValid()) {
		check(GEngine);
		check(RenderTextureTarget != nullptr);

		// insert Capture_RenderThread function after rendering without UI
		PostRenderDelegateExHandle = GEngine->GetPostRenderDelegateEx().AddUObject(
		    this, &ThisClass::Capture_RenderThread);
	}
}

void UViewportCapture::StopCapturing() {
	// remove callback
	GEngine->GetPostRenderDelegateEx().Remove(PostRenderDelegateExHandle);

	// invalidate callback handle
	PostRenderDelegateExHandle.Reset();
}

UViewportCapture::~UViewportCapture() {
	StopCapturing();
}

void UViewportCapture::Capture_RenderThread(FRDGBuilder& RDGBuilder) {
	ensure(IsInRenderingThread());
	check(RenderTextureTarget != nullptr);
	GEngine->GameViewport->GetGameViewport()->GetRenderTargetTexture();
	FTexture2DRHIRef ref;

	AddPass(
	    RDGBuilder, RDG_EVENT_NAME(__FUNCTION__),
	    [this](FRHICommandListImmediate& RHICmdList) {
		    IRendererModule* RendererModule =
		        &FModuleManager::GetModuleChecked<IRendererModule>(
		            TEXT("Renderer"));
		    check(RendererModule);

		    const FIntPoint TargetSize(RenderTextureTarget->SizeX,
		                               RenderTextureTarget->SizeY);
		    FRHITexture*    DestRHITexture =
		        RenderTextureTarget->GetRenderTargetResource()->GetTextureRHI();

		    FRHIRenderPassInfo DestRPInfo(
		        DestRHITexture, ERenderTargetActions::Load_Store, DestRHITexture);
		    RHICmdList.BeginRenderPass(DestRPInfo,
		                               TEXT("FrameCaptureResolveRenderTarget"));
		    {
			    RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);

			    FGraphicsPipelineStateInitializer GraphicsPSOInit;
			    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			    GraphicsPSOInit.BlendState =
			        TStaticBlendState<CW_RGB>::GetRHI(); // disable alpha value
			    GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
			    GraphicsPSOInit.DepthStencilState =
			        TStaticDepthStencilState<false, CF_Always>::GetRHI();

			    const ERHIFeatureLevel::Type FeatureLevel = GMaxRHIFeatureLevel;

			    FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(FeatureLevel);
			    check(ShaderMap);
			    TShaderMapRef<FScreenVS> VertexShader(ShaderMap);
			    TShaderMapRef<FScreenPS> PixelShader(ShaderMap);

			    GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI =
			        GFilterVertexDeclaration.VertexDeclarationRHI;
			    GraphicsPSOInit.BoundShaderState.VertexShaderRHI =
			        static_cast<FRHIVertexShader*>(
			            VertexShader.GetRHIShaderBase(SF_Vertex));
			    GraphicsPSOInit.BoundShaderState.PixelShaderRHI =
			        static_cast<FRHIPixelShader*>(
			            PixelShader.GetRHIShaderBase(SF_Pixel));
			    GraphicsPSOInit.PrimitiveType = PT_TriangleList;

			    SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

#if 0
					const bool bIsSourceBackBufferSameAsWindowSize = SrcRHISceneTexture->GetSizeX() == WindowSize.X && SrcRHISceneTexture->GetSizeY() == WindowSize.Y;
					const bool bIsSourceBackBufferSameAsTargetSize = TargetSize.X == SrcRHISceneTexture->GetSizeX() && TargetSize.Y == SrcRHISceneTexture->GetSizeY();

					if (bIsSourceBackBufferSameAsWindowSize || bIsSourceBackBufferSameAsTargetSize)
					{
						PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(), SrcRHISceneTexture);
					}
					else
					{
						PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), SrcRHISceneTexture);
					}
#else
			    // Widnow Size����ĂȂ��̂�Bilinear�O���
			    FRHITexture* SrcRHISceneTexture =
			        GEngine->GameViewport->GetGameViewport()
			            ->GetRenderTargetTexture();

			    check(SrcRHISceneTexture);
			    FRHIBatchedShaderParameters& BatchedParameters =
			        RHICmdList.GetScratchShaderParameters();
			    PixelShader->SetParameters(BatchedParameters,
			                               TStaticSamplerState<SF_Bilinear>::GetRHI(),
			                               SrcRHISceneTexture);
			    RHICmdList.SetBatchedShaderParameters(
			        RHICmdList.GetBoundPixelShader(), BatchedParameters);
#endif
			    FVector2D ViewRectSize;
			    GEngine->GameViewport->GetViewportSize(ViewRectSize);
			    RendererModule->DrawRectangle(
			        RHICmdList, 0, 0,               // Dest X, Y
			        TargetSize.X, TargetSize.Y,     // Dest W, H
			        0, 0,                           // Source X, Y
			        ViewRectSize.X, ViewRectSize.Y, // Source W, H
			        TargetSize,                     // Dest buffer size
			        FIntPoint(SrcRHISceneTexture->GetSizeX(),
			                  SrcRHISceneTexture->GetSizeY()), // Source texture size
			        VertexShader, EDRF_Default);
		    }
		    RHICmdList.EndRenderPass();
	    });
}
