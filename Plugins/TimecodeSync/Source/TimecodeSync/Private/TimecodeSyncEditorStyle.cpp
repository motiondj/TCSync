#include "TimecodeSyncEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Brushes/SlateImageBrush.h"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)

TSharedPtr<FSlateStyleSet> FTimecodeSyncEditorStyle::StyleSet = nullptr;

void FTimecodeSyncEditorStyle::Initialize()
{
    if (!StyleSet.IsValid())
    {
        StyleSet = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);
    }
}

void FTimecodeSyncEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
        ensure(StyleSet.IsUnique());
        StyleSet.Reset();
    }
}

FName FTimecodeSyncEditorStyle::GetStyleSetName()
{
    static FName StyleName(TEXT("TimecodeSyncEditorStyle"));
    return StyleName;
}

TSharedRef<FSlateStyleSet> FTimecodeSyncEditorStyle::Create()
{
    TSharedRef<FSlateStyleSet> StyleRef = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

    // 스타일 경로 설정
    StyleRef->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("TimecodeSync"))->GetBaseDir() / TEXT("Resources"));

    // 이미지 브러시 추가
    StyleRef->Set("TimecodeSyncEditor.OpenTimecodeSyncUI", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), FVector2D(40.0f, 40.0f)));

    return StyleRef;
}

#undef IMAGE_BRUSH