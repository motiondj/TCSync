#include "TimecodeSyncEditorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateTypes.h"
#include "Styling/CoreStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Brushes/SlateImageBrush.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"

#define IMAGE_BRUSH(RelativePath, ...) FSlateImageBrush(ResourcesPath / RelativePath + TEXT(".png"), __VA_ARGS__)

TSharedPtr<FSlateStyleSet> FTimecodeSyncEditorStyle::StyleSet = nullptr;

void FTimecodeSyncEditorStyle::Initialize()
{
    if (!StyleSet.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Starting initialization"));
        
        // Create new style set
        TSharedRef<FSlateStyleSet> NewStyleSet = Create();
        
        // Register the style set
        if (FSlateStyleRegistry::FindSlateStyle(GetStyleSetName()) != nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Style already registered, unregistering first"));
            FSlateStyleRegistry::UnRegisterSlateStyle(GetStyleSetName());
        }

        FSlateStyleRegistry::RegisterSlateStyle(*NewStyleSet);
        StyleSet = NewStyleSet;
        
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Successfully initialized style set"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Style set already initialized"));
    }
}

void FTimecodeSyncEditorStyle::Shutdown()
{
    if (StyleSet.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Shutting down"));
        FSlateStyleRegistry::UnRegisterSlateStyle(GetStyleSetName());
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

    // Set style path
    TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("TimecodeSync"));
    if (!Plugin.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("TimecodeSyncEditorStyle: Failed to find TimecodeSync plugin"));
        return StyleRef;
    }

    FString PluginBaseDir = Plugin->GetBaseDir();
    FString ResourcesPath = PluginBaseDir / TEXT("Resources");
    
    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Plugin Base Directory: %s"), *PluginBaseDir);
    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Resources Directory: %s"), *ResourcesPath);
    
    // Check if directory exists and is accessible
    if (!FPaths::DirectoryExists(ResourcesPath))
    {
        UE_LOG(LogTemp, Error, TEXT("TimecodeSyncEditorStyle: Resources directory does not exist: %s"), *ResourcesPath);
        return StyleRef;
    }

    // Check directory contents
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *ResourcesPath, TEXT("*.png"));
    UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Found %d PNG files in Resources directory"), Files.Num());
    for (const FString& File : Files)
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Found file: %s"), *File);
    }

    // Check if icon files exist and are accessible
    FString Icon20xPath = ResourcesPath / TEXT("ButtonIcon_20x.png");
    FString Icon40xPath = ResourcesPath / TEXT("ButtonIcon_40x.png");
    
    // Verify file existence and accessibility
    bool bIcon20xExists = FPaths::FileExists(Icon20xPath);
    bool bIcon40xExists = FPaths::FileExists(Icon40xPath);
    
    if (!bIcon20xExists)
    {
        UE_LOG(LogTemp, Error, TEXT("TimecodeSyncEditorStyle: Icon file not found: %s"), *Icon20xPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Found 20x icon at: %s"), *Icon20xPath);
    }

    if (!bIcon40xExists)
    {
        UE_LOG(LogTemp, Error, TEXT("TimecodeSyncEditorStyle: Icon file not found: %s"), *Icon40xPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Found 40x icon at: %s"), *Icon40xPath);
    }

    // Set content root before creating brushes
    StyleRef->SetContentRoot(ResourcesPath);

    // Create brushes with proper error handling
    try
    {
        // Add image brush with fallback to default style
        if (bIcon40xExists)
        {
            StyleRef->Set("TimecodeSyncEditor.OpenTimecodeSyncUI", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), FVector2D(40.0f, 40.0f)));
            UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Successfully set OpenTimecodeSyncUI style"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Using default style for OpenTimecodeSyncUI"));
            StyleRef->Set("TimecodeSyncEditor.OpenTimecodeSyncUI", new FSlateNoResource());
        }

        // Add button styles with fallback
        if (bIcon20xExists)
        {
            StyleRef->Set("TimecodeSyncEditor.OpenTimecodeSyncUI.Small", new IMAGE_BRUSH(TEXT("ButtonIcon_20x"), FVector2D(20.0f, 20.0f)));
            UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Successfully set OpenTimecodeSyncUI.Small style"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Using default style for OpenTimecodeSyncUI.Small"));
            StyleRef->Set("TimecodeSyncEditor.OpenTimecodeSyncUI.Small", new FSlateNoResource());
        }

        if (bIcon40xExists)
        {
            StyleRef->Set("TimecodeSyncEditor.OpenTimecodeSyncUI.Large", new IMAGE_BRUSH(TEXT("ButtonIcon_40x"), FVector2D(40.0f, 40.0f)));
            UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Successfully set OpenTimecodeSyncUI.Large style"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("TimecodeSyncEditorStyle: Using default style for OpenTimecodeSyncUI.Large"));
            StyleRef->Set("TimecodeSyncEditor.OpenTimecodeSyncUI.Large", new FSlateNoResource());
        }
    }
    catch (const std::exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("TimecodeSyncEditorStyle: Exception while creating brushes: %s"), UTF8_TO_TCHAR(e.what()));
    }

    return StyleRef;
}

#undef IMAGE_BRUSH