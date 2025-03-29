// TimecodeSyncTestLogger.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeSyncTestLogger.generated.h"

// 로그 레벨 열거형
UENUM(BlueprintType)
enum class ETestLogLevel : uint8
{
    Error,      // 오류만 표시
    Warning,    // 경고 이상만 표시
    Info,       // 기본 정보 표시 (기본값)
    Verbose,    // 상세 정보 표시
    Debug       // 모든 디버그 정보 표시
};

// 로그 항목 구조체
USTRUCT(BlueprintType)
struct FTestLogEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Test")
    FString Message;

    UPROPERTY(BlueprintReadOnly, Category = "Test")
    ETestLogLevel Level;

    UPROPERTY(BlueprintReadOnly, Category = "Test")
    FString TestName;

    UPROPERTY(BlueprintReadOnly, Category = "Test")
    FDateTime Timestamp;
};

UCLASS(BlueprintType)
class TIMECODESYNC_API UTimecodeSyncTestLogger : public UObject
{
    GENERATED_BODY()

public:
    // 싱글톤 인스턴스 얻기
    static UTimecodeSyncTestLogger* Get();

    // 로그 레벨 설정
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void SetLogLevel(ETestLogLevel NewLevel);

    // 로그 기록 함수들
    void LogError(const FString& TestName, const FString& Message);
    void LogWarning(const FString& TestName, const FString& Message);
    void LogInfo(const FString& TestName, const FString& Message);
    void LogVerbose(const FString& TestName, const FString& Message);
    void LogDebug(const FString& TestName, const FString& Message);

    // 일반 로그 기록 (레벨 지정)
    void Log(const FString& TestName, const FString& Message, ETestLogLevel Level);

    // 테스트 결과 로깅
    void LogTestResult(const FString& TestName, bool bSuccess, const FString& Message);

    // 로그 내보내기
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    bool ExportLogs(const FString& FilePath);

    // 화면에 로그 표시
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void DisplayLogsOnScreen(float Duration = 10.0f, int32 MaxLines = 10);

    // 로그 초기화
    UFUNCTION(BlueprintCallable, Category = "TimecodeSyncTest")
    void ClearLogs();

private:
    // 싱글톤 인스턴스
    static UTimecodeSyncTestLogger* Instance;

    // 로그 레벨
    ETestLogLevel CurrentLogLevel;

    // 로그 저장소
    TArray<FTestLogEntry> Logs;
};