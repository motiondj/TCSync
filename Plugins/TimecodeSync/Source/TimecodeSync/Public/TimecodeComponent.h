#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeComponent.generated.h"

// 타임코드 변경 이벤트를 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeChanged, const FString&, NewTimecode);

// 타임코드 이벤트 트리거를 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimecodeEventTriggered, const FString&, EventName, float, EventTime);

// 네트워크 연결 상태 변경을 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkConnectionChanged, ENetworkConnectionState, NewState);

/**
 * 액터에 부착하여 타임코드 기능을 제공하는 컴포넌트
 */
UCLASS(ClassGroup = "Timecode", meta = (BlueprintSpawnableComponent))
class TIMECODESYNC_API UTimecodeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimecodeComponent();

    // 타임코드 역할 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bIsMaster;

    // nDisplay 사용 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bUseNDisplay;

    // 프레임 레이트 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
    float FrameRate;

    // UDP 포트 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 UDPPort;

    // 타겟 IP 설정 (마스터 모드에서 유니캐스트 전송 시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    FString TargetIP;

    // 멀티캐스트 그룹 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    FString MulticastGroup;

    // 자동 시작 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bAutoStart;

    // 네트워크 동기화 간격 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float SyncInterval;

    // 현재 타임코드 (읽기 전용)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode")
    FString CurrentTimecode;

    // 타임코드 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeChanged OnTimecodeChanged;

    // 타임코드 이벤트 트리거
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeEventTriggered OnTimecodeEventTriggered;

    // 네트워크 연결 상태 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkConnectionChanged OnNetworkConnectionChanged;

protected:
    // 컴포넌트 초기화 시 호출
    virtual void BeginPlay() override;

    // 컴포넌트 제거 시 호출
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 매 프레임 호출
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // 타임코드 시작
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StartTimecode();

    // 타임코드 정지
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StopTimecode();

    // 타임코드 재설정
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ResetTimecode();

    // 타임코드 이벤트 등록
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void RegisterTimecodeEvent(const FString& EventName, float EventTimeInSeconds);

    // 타임코드 이벤트 제거
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void UnregisterTimecodeEvent(const FString& EventName);

    // 네트워크 연결 상태 가져오기
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetNetworkConnectionState() const;

    // 네트워크 연결 설정
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SetupNetwork();

    // 네트워크 연결 종료
    UFUNCTION(BlueprintCallable, Category = "Network")
    void ShutdownNetwork();

private:
    // 타임코드 실행 중 여부
    bool bIsRunning;

    // 경과 시간 (초)
    float ElapsedTimeSeconds;

    // 타임코드 이벤트 맵 (이벤트 이름 -> 트리거 시간)
    TMap<FString, float> TimecodeEvents;

    // 이미 트리거된 이벤트 추적
    TSet<FString> TriggeredEvents;

    // 네트워크 동기화 타이머
    float SyncTimer;

    // 네트워크 관리자
    UPROPERTY()
    UTimecodeNetworkManager* NetworkManager;

    // 내부 타임코드 업데이트 함수
    void UpdateTimecode(float DeltaTime);

    // 타임코드 이벤트 확인 함수
    void CheckTimecodeEvents();

    // 네트워크 동기화 함수
    void SyncOverNetwork();

    // 네트워크 메시지 수신 콜백
    UFUNCTION()
    void OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message);

    // 네트워크 상태 변경 콜백
    UFUNCTION()
    void OnNetworkStateChanged(ENetworkConnectionState NewState);
};