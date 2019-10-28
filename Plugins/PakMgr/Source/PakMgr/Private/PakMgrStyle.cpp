// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "PakMgrStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FPakMgrStyle::StyleInstance = NULL;

//#define IMAGE_PLUGIN_BRUSH( RelativePath, ... ) FSlateImageBrush( FPakMgrStyle::InContent( RelativePath, ".png" ), __VA_ARGS__ )
//#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
//#define BOX_BRUSH(RelativePath, ...) FSlateBoxBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
//#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

void FPakMgrStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FPakMgrStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FPakMgrStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("PakMgrStyle"));
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon20x20(20.0f, 20.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef< FSlateStyleSet > FPakMgrStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable(new FSlateStyleSet("PakMgrStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("PakMgr")->GetBaseDir() / TEXT("Resources"));

	Style->Set("IPakMgrModule.OpenPluginWindow", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("FileTree.SelCon", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("FileTree.AddM", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("FileTree.GenRef", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("FileTree.SortRef", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("FileTree.SearchFiles", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));

	Style->Set("PakManager.GenPaks", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("PakManager.GenMani", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));
	Style->Set("PakManager.MaxSize", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), Icon40x40));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FPakMgrStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

const ISlateStyle& FPakMgrStyle::Get()
{
	return *StyleInstance;
}
