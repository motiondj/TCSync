// TimecodeSyncTestLogger.cpp
#include "Tests/TimecodeSyncTestLogger.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// 정적 싱글톤 인스턴스 초기화
UTimecodeSyncTestLogger* UTimecodeSyncTestLogger::Instance = nullptr;

UTimecodeSyncTestLogger* UTimecodeSyncTestLogger::Get()
{
    if (Instance == nullptr)
    {
        Instance = NewObject<UTimecodeSyncTestLogger>();
        Instance->AddToRoot(); // GC 방지
        Instance->CurrentLogLevel = ETestLogLevel::Info; // 기본 로그 레벨
    }

    return Instance;
}

void UTimecodeSyncTestLogger::SetLogLevel(ETestLogLevel NewLevel)
{
    CurrentLogLevel = NewLevel;
    LogInfo(TEXT("Logger"), FString::Printf(TEXT("Log level set to: %d"), static_cast<int32>(NewLevel)));
}

void UTimecodeSyncTestLogger::LogError(const FString& TestName, const FString& Message)
{
    Log(TestName, Message, ETestLogLevel::Error);
}

void UTimecodeSyncTestLogger::LogWarning(const FString& TestName, const FString& Message)
{
    Log(TestName, Message, ETestLogLevel::Warning);
}

void UTimecodeSyncTestLogger::LogInfo(const FString& TestName, const FString& Message)
{
    Log(TestName, Message, ETestLogLevel::Info);
}

void UTimecodeSyncTestLogger::LogVerbose(const FString& TestName, const FString& Message)
{
    Log(TestName, Message, ETestLogLevel::Verbose);
}

void UTimecodeSyncTestLogger::LogDebug(const FString& TestName, const FString& Message)
{
    Log(TestName, Message, ETestLogLevel::Debug);
}

void UTimecodeSyncTestLogger::Log(const FString& TestName, const FString& Message, ETestLogLevel Level)
{
    // 로그 레벨 확인
    if (static_cast<int32>(Level) > static_cast<int32>(CurrentLogLevel))
    {
        return; // 현재 로그 레벨보다 상세한 로그는 무시
    }

    // 로그 항목 생성
    FTestLogEntry Entry;
    Entry.TestName = TestName;
    Entry.Message = Message;
    Entry.Level = Level;
    Entry.Timestamp = FDateTime::Now();

    // 로그 저장
    Logs.Add(Entry);

    // UE 로그 시스템에도 출력
    FString LevelStr;
    FColor ScreenColor;

    switch (Level)
    {
    case ETestLogLevel::Error:
        LevelStr = TEXT("ERROR");
        ScreenColor = FColor::Red;
        UE_LOG(LogTemp, Error, TEXT("[TimecodeSyncTest][%s] %s: %s"), *LevelStr, *TestName, *Message);
        break;

    case ETestLogLevel::Warning:
        LevelStr = TEXT("WARNING");
        ScreenColor = FColor::Yellow;
        UE_LOG(LogTemp, Warning, TEXT("[TimecodeSyncTest][%s] %s: %s"), *LevelStr, *TestName, *Message);
        break;

    case ETestLogLevel::Info:
        LevelStr = TEXT("INFO");
        ScreenColor = FColor::White;
        UE_LOG(LogTemp, Display, TEXT("[TimecodeSyncTest][%s] %s: %s"), *LevelStr, *TestName, *Message);
        break;

    case ETestLogLevel::Verbose:
        LevelStr = TEXT("VERBOSE");
        ScreenColor = FColor::Cyan;
        UE_LOG(LogTemp, Verbose, TEXT("[TimecodeSyncTest][%s] %s: %s"), *LevelStr, *TestName, *Message);
        break;

    case ETestLogLevel::Debug:
        LevelStr = TEXT("DEBUG");
        ScreenColor = FColor::Silver;
        UE_LOG(LogTemp, VeryVerbose, TEXT("[TimecodeSyncTest][%s] %s: %s"), *LevelStr, *TestName, *Message);
        break;
    }

    // 화면에 표시 (Error, Warning, Info 레벨만)
    if (Level <= ETestLogLevel::Info && GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, ScreenColor,
            FString::Printf(TEXT("[%s] %s: %s"), *LevelStr, *TestName, *Message));
    }
}

void UTimecodeSyncTestLogger::LogTestResult(const FString& TestName, bool bSuccess, const FString& Message)
{
    FString ResultStr = bSuccess ? TEXT("PASSED") : TEXT("FAILED");

    // 성공 여부에 따라 로그 레벨 구분
    if (bSuccess)
    {
        LogInfo(TestName, FString::Printf(TEXT("%s - %s"), *ResultStr, *Message));
    }
    else
    {
        LogError(TestName, FString::Printf(TEXT("%s - %s"), *ResultStr, *Message));
    }

    // 중요한 테스트 결과는 항상 화면에 표시
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, bSuccess ? FColor::Green : FColor::Red,
            FString::Printf(TEXT("[TEST] %s: %s"), *TestName, *ResultStr));
        GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::White, Message);
    }
}

bool UTimecodeSyncTestLogger::ExportLogs(const FString& FilePath)
{
    FString LogContent;

    // 헤더
    LogContent += TEXT("TimecodeSync Test Logs\n");
    LogContent += TEXT("=====================\n\n");
    LogContent += FString::Printf(TEXT("Generated: %s\n"), *FDateTime::Now().ToString());
    LogContent += FString::Printf(TEXT("Log entries: %d\n\n"), Logs.Num());

    // 로그 항목
    for (const FTestLogEntry& Entry : Logs)
    {
        FString LevelStr;

        switch (Entry.Level)
        {
        case ETestLogLevel::Error:   LevelStr = TEXT("ERROR  "); break;
        case ETestLogLevel::Warning: LevelStr = TEXT("WARNING"); break;
        case ETestLogLevel::Info:    LevelStr = TEXT("INFO   "); break;
        case ETestLogLevel::Verbose: LevelStr = TEXT("VERBOSE"); break;
        case ETestLogLevel::Debug:   LevelStr = TEXT("DEBUG  "); break;
        }

        LogContent += FString::Printf(TEXT("[%s][%s] %s: %s\n"),
            *Entry.Timestamp.ToString(TEXT("%H:%M:%S.%s")),
            *LevelStr,
            *Entry.TestName,
            *Entry.Message);
    }

    // 파일 저장
    return FFileHelper::SaveStringToFile(LogContent, *FilePath);
}

void UTimecodeSyncTestLogger::DisplayLogsOnScreen(float Duration, int32 MaxLines)
{
    if (!GEngine || Logs.Num() == 0)
        return;

    // 최근 로그부터 표시
    int32 StartIndex = FMath::Max(0, Logs.Num() - MaxLines);

    for (int32 i = StartIndex; i < Logs.Num(); i++)
    {
        const FTestLogEntry& Entry = Logs[i];

        // 로그 레벨에 따른 색상
        FColor Color;

        switch (Entry.Level)
        {
        case ETestLogLevel::Error:   Color = FColor::Red; break;
        case ETestLogLevel::Warning: Color = FColor::Yellow; break;
        case ETestLogLevel::Info:    Color = FColor::White; break;
        case ETestLogLevel::Verbose: Color = FColor::Cyan; break;
        case ETestLogLevel::Debug:   Color = FColor::Silver; break;
        }

        // 화면에 표시
        GEngine->AddOnScreenDebugMessage(-1, Duration, Color,
            FString::Printf(TEXT("%s: %s"), *Entry.TestName, *Entry.Message));
    }
}

void UTimecodeSyncTestLogger::ClearLogs()
{
    Logs.Empty();
    LogInfo(TEXT("Logger"), TEXT("Logs cleared"));
}