// Copyright Epic Games, Inc. All Rights Reserved.

#include "ViewportCapSample.h"

#include "CommonRenderResources.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Misc/AssertionMacros.h"
#include "Modules/ModuleManager.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "ScreenRendering.h"
#include "Slate/SceneViewport.h"

IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, ViewportCapSample,
                              "ViewportCapSample");
