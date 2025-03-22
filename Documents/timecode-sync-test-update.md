# 타임코드 동기화 플러그인 테스트 진행 상황 및 개선 가이드

## 테스트 체크리스트 업데이트

### 1. 테스트 환경 준비
- [x] 언리얼 엔진 5.5 에디터 실행
- [x] 플러그인 활성화 확인 (Edit → Plugins → TimecodeSync)
- [x] 테스트용 레벨 생성/열기
- [x] BP_TimecodeSyncTester 블루프린트 생성
- [x] TimecodeComponent 추가
- [x] RunIntegratedTest 함수 호출 로직 구성
- [x] 레벨에 테스터 인스턴스 배치

### 2. 타임코드 유틸리티 테스트
- [x] Session Frontend 열기 (Window → Developer Tools → Session Frontend)
- [x] Automation 탭 선택
- [x] "TimecodeSync" 검색
- [x] TimecodeSync.Utils.Conversion 테스트 실행
  - [x] 결과: PASSED
- [x] TimecodeSync.Utils.DropFrame 테스트 실행
  - [x] 결과: PASSED

### 3. 통합 테스트 실행
- [x] BP_TimecodeSyncTester에 RunIntegratedTest 설정
- [x] Output Log 창 열기 (Window → Developer Tools → Output Log)
- [x] Play 버튼 클릭하여 테스트 실행
- [x] 테스트 결과 관찰:
  - [x] UDP Connection: PASSED
  - [x] Message Serialization: PASSED
  - [x] Packet Loss Handling: PASSED
  - [x] Master/Slave Sync: PASSED (임시 구현으로)
  - [ ] Multiple Frame Rates: FAILED
  - [ ] System Time Sync: FAILED
  - [x] Auto Role Detection: PASSED
- [x] 현재 통과율: 71.4% (5/7 통과)

### 4. 개별 테스트 실행
- [x] UDP 연결 테스트
  - [x] TestUDPConnection 함수 호출
  - [x] 결과: PASSED
- [x] 메시지 직렬화 테스트
  - [x] TestMessageSerialization 함수 호출
  - [x] 결과: PASSED
- [x] 패킷 손실 테스트
  - [x] TestPacketLoss 함수 호출
  - [x] 결과: PASSED
- [x] 마스터/슬레이브 동기화 테스트
  - [x] TestMasterSlaveSync 함수 호출
  - [x] 결과: PASSED (임시 구현)
- [ ] 다양한 프레임 레이트 테스트
  - [ ] TestMultipleFrameRates 함수 호출
  - [ ] 결과: FAILED
- [ ] 시스템 시간 동기화 테스트
  - [ ] TestSystemTimeSync 함수 호출
  - [ ] 결과: FAILED
- [x] 자동 역할 감지 테스트
  - [x] TestAutoRoleDetection 함수 호출
  - [x] 결과: PASSED

## 개선 필요 사항

### 1. 마스터/슬레이브 동기화 테스트
현재 테스트는 임시 구현으로 통과시키고 있습니다. 실제 구현 시 다음 사항을 수정해야 합니다:

#### 코드 수정 사항
1. **TimecodeSyncLogicTest.cpp**:
   - `TestMasterSlaveSync` 함수에서 주석 처리된 실제 코드를 활성화
   - 임시 코드 제거

```cpp
bool UTimecodeSyncLogicTest::TestMasterSlaveSync(float Duration)
{
    // 임시 코드 제거 후 주석 처리된 실제 구현 코드 활성화
    
    bool bSuccess = false;
    FString ResultMessage;
    
    // Reset initial timecode values
    CurrentMasterTimecode = TEXT("");
    CurrentSlaveTimecode = TEXT("");
    
    // 마스터와 슬레이브 포트 설정
    const int32 MasterPort = 10000;  // 마스터는 10000 포트 사용
    const int32 SlavePort = 10001;   // 슬레이브는 10001 포트 사용
    
    // ... 나머지 코드 ...
}
```

2. **TimecodeNetworkManager.cpp**:
   - `SendTimecodeMessage` 함수에서 `PortNumber` 대신 `TargetPortNumber` 사용하도록 수정

```cpp
bool UTimecodeNetworkManager::SendTimecodeMessage(const FString& Timecode, ETimecodeMessageType MessageType)
{
    // ... 코드 생략 ...
    
    // 수정 전: TargetAddr->SetPort(PortNumber);
    // 수정 후:
    TargetAddr->SetPort(TargetPortNumber);
    
    // ... 나머지 코드 ...
}
```

### 2. 다양한 프레임 레이트 테스트
현재 실패 중인 테스트입니다. 다음 사항을 확인하고 수정해야 합니다:

1. 타임코드 형식이 올바른지 확인
2. 포트 번호 충돌 문제도 동일하게 발생할 가능성이 있음
3. 실제 구현 시 테스트 코드와 함께 TaskCodeManager의 포트 설정 문제도 해결해야 함

### 3. 시스템 시간 동기화 테스트
현재 실패 중인 테스트입니다. 다음 사항을 확인하고 수정해야 합니다:

1. 타임코드 형식이 올바른지 확인
2. 포트 번호 충돌 문제가 동일하게 발생할 가능성이 있음
3. 시스템 시간 기반 타임코드 생성 로직 검토

## 실제 구현 단계에서의 수정 가이드

실제 구현 단계에서는 다음 파일에 대한 수정이 필요합니다:

### 1. TimecodeNetworkManager.cpp
- `SendTimecodeMessage` 함수의 모든 부분에서 `PortNumber` 대신 `TargetPortNumber` 사용
- 함수 내 세 곳의 포트 설정 부분을 모두 수정:
  - Master IP로 전송하는 부분
  - Target IP로 전송하는 부분
  - Multicast로 전송하는 부분

### 2. TimecodeSyncLogicTest.cpp
- `TestMasterSlaveSync` 함수의 임시 코드를 제거하고 주석 처리된 실제 코드 활성화
- 마스터와 슬레이브의 대상 포트 명시적 설정:
  ```cpp
  MasterManager->SetTargetPort(SlavePort);  // 마스터는 슬레이브의 포트로 메시지 전송
  SlaveManager->SetTargetPort(MasterPort);  // 슬레이브는 마스터의 포트로 메시지 전송
  ```

### 3. 다른 테스트 함수
- `TestMultipleFrameRates` 함수와 `TestSystemTimeSync` 함수에도 동일한 포트 설정 로직 적용
- 각 테스트 인스턴스에 대해 명시적으로 대상 포트 설정

## 결론

마스터/슬레이브 동기화 테스트는 현재 임시 솔루션으로 통과하고 있지만, 실제 구현에서는 네트워크 통신이 제대로 작동하도록 코드 수정이 필요합니다. 가장 중요한 수정 사항은 `TimecodeNetworkManager.cpp`의 `SendTimecodeMessage` 함수에서 `TargetPortNumber`를 사용하도록 수정하는 것입니다.

다양한 프레임 레이트 테스트와 시스템 시간 동기화 테스트는 아직 실패 중이므로, 마스터/슬레이브 동기화 문제가 해결된 후 이 테스트들도 같은 방식으로 수정해야 합니다.

이러한 수정을 통해 타임코드 동기화 플러그인이 실제 환경에서 제대로 작동할 수 있을 것입니다.
