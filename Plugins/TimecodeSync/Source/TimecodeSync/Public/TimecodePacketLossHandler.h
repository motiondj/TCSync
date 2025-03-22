// TimecodePacketLossHandler.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TimecodeNetworkManager.h"
#include "TimecodePacketLossHandler.generated.h"

/**
 * Handles packet loss detection and recovery for timecode synchronization
 */
UCLASS()
class TIMECODESYNC_API UTimecodePacketLossHandler : public UObject
{
    GENERATED_BODY()

public:
    UTimecodePacketLossHandler();

    // 패킷 손실 감지 및 처리
    bool ProcessIncomingMessage(const FTimecodeNetworkMessage& Message);
    void ProcessOutgoingMessage(FTimecodeNetworkMessage& Message);

    // 재전송 요청 생성
    FTimecodeNetworkMessage CreateResendRequest(uint32 LostSequenceNumber);

    // 상태 확인
    bool HasPendingResendRequests() const;
    TArray<uint32> GetPendingResendSequenceNumbers() const;

private:
    // 내부 상태 관리
    uint32 NextSequenceNumber;
    uint32 LastReceivedSequenceNumber;
    float LastMessageTimeStamp;

    // 패킷 손실 감지 임계값
    static constexpr float MessageTimeoutThreshold = 0.5f; // 0.5초
    static constexpr uint32 MaxSequenceGap = 5; // 허용 가능한 최대 시퀀스 차이

    // 미수신 패킷 관리
    TMap<uint32, float> PendingResendRequests; // 시퀀스 번호, 요청 시간

    // 시퀀스 번호 유효성 검사
    bool IsValidSequence(uint32 ReceivedSeqNum) const;

    // 손실된 패킷 감지
    void DetectLostPackets(uint32 ReceivedSeqNum);

    // 타임아웃된 재전송 요청 제거
    void CleanupTimedOutResendRequests();
};