#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TimecodeUtils.generated.h"

/**
 * 타임코드 생성 및 처리 유틸리티 클래스
 */
UCLASS()
class TIMECODESYNC_API UTimecodeUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * 시간(초)을 SMPTE 타임코드 문자열로 변환
     * @param TimeInSeconds - 변환할 시간(초)
     * @param FrameRate - 프레임 레이트
     * @param bUseDropFrame - 드롭 프레임 타임코드 사용 여부
     * @return 형식화된 타임코드 문자열 (HH:MM:SS:FF)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    static FString SecondsToTimecode(float TimeInSeconds, float FrameRate, bool bUseDropFrame = false);

    /**
     * SMPTE 타임코드 문자열을 시간(초)으로 변환
     * @param Timecode - 변환할 타임코드 문자열 (HH:MM:SS:FF)
     * @param FrameRate - 프레임 레이트
     * @param bUseDropFrame - 드롭 프레임 타임코드 사용 여부
     * @return 시간(초)
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    static float TimecodeToSeconds(const FString& Timecode, float FrameRate, bool bUseDropFrame = false);

    /**
     * 현재 시스템 시간을 타임코드로 변환
     * @param FrameRate - 프레임 레이트
     * @param bUseDropFrame - 드롭 프레임 타임코드 사용 여부
     * @return 현재 시간의 타임코드 문자열
     */
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    static FString GetCurrentSystemTimecode(float FrameRate, bool bUseDropFrame = false);
};
