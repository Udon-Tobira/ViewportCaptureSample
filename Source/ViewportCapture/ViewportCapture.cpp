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

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ViewportCapture,
                              "ViewportCapture");
DEFINE_LOG_CATEGORY_STATIC(LogViewportCapture, Display, All);

void UViewportCapture::StartCapturing() {
	check(GEngine);

	// insert Capture_RenderThread function after rendering without UI
	GEngine->GetPostRenderDelegateEx().AddUObject(
	    this, &ThisClass::Capture_RenderThread);
}

void UViewportCapture::Capture_RenderThread(FRDGBuilder& RDGBuilder) {
	ensure(IsInRenderingThread());
	check(RenderTextureTarget != nullptr);

	AddPass(
	    RDGBuilder, RDG_EVENT_NAME(__FUNCTION__),
	    [this](FRHICommandListImmediate& RHICmdList) {
		    IRendererModule* RendererModule =
		        &FModuleManager::GetModuleChecked<IRendererModule>(
		            TEXT("Renderer"));
		    check(RendererModule);

		    const FIntPoint TargetSize(RenderTextureTarget->SizeX,
		                               RenderTextureTarget->SizeY);
		    FRHITexture*    DestRenderTarget =
		        RenderTextureTarget->GetRenderTargetResource()->GetTextureRHI();

		    FRHIRenderPassInfo DestRPInfo(DestRenderTarget,
		                                  ERenderTargetActions::Load_Store,
		                                  DestRenderTarget);
		    RHICmdList.BeginRenderPass(DestRPInfo,
		                               TEXT("FrameCaptureResolveRenderTarget"));
		    {
			    RHICmdList.SetViewport(0, 0, 0.0f, TargetSize.X, TargetSize.Y, 1.0f);

			    FGraphicsPipelineStateInitializer GraphicsPSOInit;
			    RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
			    GraphicsPSOInit.BlendState      = TStaticBlendState<>::GetRHI();
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
					const bool bIsSourceBackBufferSameAsWindowSize = SceneTexture->GetSizeX() == WindowSize.X && SceneTexture->GetSizeY() == WindowSize.Y;
					const bool bIsSourceBackBufferSameAsTargetSize = TargetSize.X == SceneTexture->GetSizeX() && TargetSize.Y == SceneTexture->GetSizeY();

					if (bIsSourceBackBufferSameAsWindowSize || bIsSourceBackBufferSameAsTargetSize)
					{
						PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Point>::GetRHI(), SceneTexture);
					}
					else
					{
						PixelShader->SetParameters(RHICmdList, TStaticSamplerState<SF_Bilinear>::GetRHI(), SceneTexture);
					}
#else
			    // Widnow SizeŽæ‚Á‚Ä‚È‚¢‚Ì‚ÅBilinear‘O’ñ‚Å
			    FRHITexture* SceneTexture = GEngine->GameViewport->GetGameViewport()
			                                    ->GetRenderTargetTexture();
			    check(SceneTexture);
			    FRHIBatchedShaderParameters& BatchedParameters =
			        RHICmdList.GetScratchShaderParameters();
			    PixelShader->SetParameters(BatchedParameters,
			                               TStaticSamplerState<SF_Bilinear>::GetRHI(),
			                               SceneTexture);
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
			        FIntPoint(SceneTexture->GetSizeX(),
			                  SceneTexture->GetSizeY()), // Source texture size
			        VertexShader, EDRF_Default);
		    }
		    RHICmdList.EndRenderPass();
	    });
}
