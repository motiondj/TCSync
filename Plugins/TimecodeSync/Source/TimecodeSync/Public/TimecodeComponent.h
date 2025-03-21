#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimecodeNetworkManager.h"
#include "TimecodeNetworkTypes.h"
#include "TimecodeComponent.generated.h"

// 타임코드 변경 이벤트를 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimecodeChanged, const FString&, NewTimecode);

// 타임코드 이벤트 트리거를 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimecodeEventTriggered, const FString&, EventName, float, EventTime);

// 네트워크 연결 상태 변경을 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNetworkConnectionChanged, ENetworkConnectionState, NewState);

// 역할 변경 이벤트를 위한 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRoleChanged, bool, IsMaster);

/**
 * 액터에 부착하여 타임코드 기능을 제공하는 컴포넌트
 */
UCLASS(ClassGroup = "Timecode", meta = (BlueprintSpawnableComponent))
class TIMECODESYNC_API UTimecodeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTimecodeComponent();

    /** 역할 설정 */

    // 역할 결정 모드 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role")
    ETimecodeRoleMode RoleMode;

    // 자동 모드에서 마스터 여부 (읽기 전용)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode Role")
    bool bIsMaster;

    // 수동 모드에서 마스터 여부 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual"))
    bool bIsManuallyMaster;

    // 마스터 IP 설정 (수동 슬레이브 모드에서 사용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual && !bIsManuallyMaster"))
    FString MasterIPAddress;

    // nDisplay 사용 여부 (자동 모드에서만 사용)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Automatic"))
    bool bUseNDisplay;

    /** 타임코드 설정 */

    // 프레임 레이트 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode", meta = (ClampMin = "1.0", ClampMax = "240.0"))
    float FrameRate;

    // 드롭 프레임 타임코드 사용 여부
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bUseDropFrameTimecode;

    // 자동 시작 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
    bool bAutoStart;

    /** 네트워크 설정 */

    // UDP 포트 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "1024", ClampMax = "65535"))
    int32 UDPPort;

    // 타겟 IP 설정 (마스터 모드에서 유니캐스트 전송 시)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    FString TargetIP;

    // 멀티캐스트 그룹 설정
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
    FString MulticastGroup;

    // 네트워크 동기화 간격 (초)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float SyncInterval;

    /** 상태 및 통계 (읽기 전용) */

    // 현재 타임코드 (읽기 전용)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode")
    FString CurrentTimecode;

    // 네트워크 연결 상태 (읽기 전용)
    UPROPERTY(BlueprintReadOnly, Category = "Network")
    ENetworkConnectionState ConnectionState;

    // 타임코드 실행 중 여부 (읽기 전용)
    UPROPERTY(BlueprintReadOnly, Category = "Timecode")
    bool bIsRunning;

    /** 이벤트 델리게이트 */

    // 타임코드 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeChanged OnTimecodeChanged;

    // 타임코드 이벤트 트리거
    UPROPERTY(BlueprintAssignable, Category = "Timecode")
    FOnTimecodeEventTriggered OnTimecodeEventTriggered;

    // 네트워크 연결 상태 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Network")
    FOnNetworkConnectionChanged OnNetworkConnectionChanged;

    // 역할 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Timecode Role")
    FOnRoleChanged OnRoleChanged;

    // 역할 모드 변경 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Timecode Role")
    FRoleModeChangedDelegate OnRoleModeChanged;

protected:
    // 컴포넌트 초기화 시 호출
    virtual void BeginPlay() override;

    // 컴포넌트 제거 시 호출
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // 매 프레임 호출
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    /** 타임코드 제어 함수 */

    // 타임코드 시작
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StartTimecode();

    // 타임코드 정지
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void StopTimecode();

    // 타임코드 재설정
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ResetTimecode();

    // 현재 타임코드 가져오기
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    FString GetCurrentTimecode() const;

    // 현재 타임코드를 초 단위로 가져오기
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    float GetCurrentTimeInSeconds() const;

    /** 타임코드 이벤트 관련 함수 */

    // 타임코드 이벤트 등록
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void RegisterTimecodeEvent(const FString& EventName, float EventTimeInSeconds);

    // 타임코드 이벤트 제거
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void UnregisterTimecodeEvent(const FString& EventName);

    // 모든 타임코드 이벤트 제거
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ClearAllTimecodeEvents();

    // 이벤트 트리거 상태 재설정
    UFUNCTION(BlueprintCallable, Category = "Timecode")
    void ResetEventTriggers();

    /** 네트워크 관련 함수 */

    // 네트워크 연결 상태 가져오기
    UFUNCTION(BlueprintCallable, Category = "Network")
    ENetworkConnectionState GetNetworkConnectionState() const;

    // 네트워크 연결 설정
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool SetupNetwork();

    // 네트워크 연결 종료
    UFUNCTION(BlueprintCallable, Category = "Network")
    void ShutdownNetwork();

    // 멀티캐스트 그룹 참가
    UFUNCTION(BlueprintCallable, Category = "Network")
    bool JoinMulticastGroup(const FString& InMulticastGroup);

    // 타겟 IP 설정
    UFUNCTION(BlueprintCallable, Category = "Network")
    void SetTargetIP(const FString& InTargetIP);

    /** 역할 관련 함수 */

    // 역할 모드 설정
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    void SetRoleMode(ETimecodeRoleMode NewMode);

    // 현재 역할 모드 가져오기
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    ETimecodeRoleMode GetRoleMode() const;

    // 수동 마스터 설정
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    void SetManualMaster(bool bInIsManuallyMaster);

    // 수동 마스터 여부 가져오기
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    bool GetIsManuallyMaster() const;

    // 마스터 IP 주소 설정
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    void SetMasterIPAddress(const FString& InMasterIP);

    // 마스터 IP 주소 가져오기
    UFUNCTION(BlueprintCallable, Category = "Timecode Role")
    FString GetMasterIPAddress() const;

    // 현재 마스터 여부 가져오기
    UFUNCTION(BlueprintPure, Category = "Timecode Role")
    bool GetIsMaster() const;

private:
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

    // 역할 변경 시 호출
    void OnRoleStateChanged(bool bNewIsMaster);

    // nDisplay 기반 역할 결정
    bool CheckNDisplayRole();

    // 네트워크 메시지 수신 콜백
    UFUNCTION()
    void OnTimecodeMessageReceived(const FTimecodeNetworkMessage& Message);

    // 네트워크 상태 변경 콜백
    UFUNCTION()
    void OnNetworkStateChanged(ENetworkConnectionState NewState);

    // 역할 모드 변경 콜백
    UFUNCTION()
    void OnNetworkRoleModeChanged(ETimecodeRoleMode NewMode);
};