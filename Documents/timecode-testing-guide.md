# 타임코드 동기화 플러그인 테스트 가이드

이 문서는 언리얼 엔진 5.5 타임코드 동기화 플러그인의 통합 테스트 및 디버깅을 위한 상세 가이드입니다. 각 단계별로 테스트 방법, 예상 결과 및 문제 해결 방법을 설명합니다.

## 1. 테스트 환경 준비

### 테스트 프로젝트 설정
1. 언리얼 엔진 5.5 에디터 실행
2. 타임코드 동기화 플러그인이 활성화되어 있는지 확인
   - Edit → Plugins 메뉴에서 TimecodeSync 플러그인 확인
   - 플러그인이 활성화되어 있지 않으면 체크박스 선택 후 에디터 재시작
3. 테스트용 빈 레벨 생성 또는 기존 레벨 열기

### 테스트 액터 설정
1. 콘텐츠 브라우저에서 우클릭 → Blueprint Class → Actor 선택
2. 새 블루프린트 이름을 "BP_TimecodeSyncTester"로 지정
3. 블루프린트 에디터에서 Components 패널 → Add Component → TimecodeComponent 추가
4. 이벤트 그래프에 테스트 실행 로직 구성:
   ```
   Event BeginPlay
     |
     +--> [RunIntegratedTest 함수 호출]
   ```
5. 저장 후 블루프린트 에디터 닫기
6. 레벨에 BP_TimecodeSyncTester 인스턴스 배치

## 2. 단위 테스트 실행

### 타임코드 유틸리티 테스트
1. Window → Developer Tools → Session Frontend 메뉴 선택
2. Automation 탭 클릭
3. 필터 필드에 "TimecodeSync" 입력
4. 나타나는 테스트 항목 확인:
   - TimecodeSync.Utils.Conversion
   - TimecodeSync.Utils.DropFrame
5. 해당 테스트 항목 선택 (체크박스 클릭)
6. "Run Tests" 버튼 클릭
7. 결과 확인:
   - 녹색: 테스트 통과
   - 빨간색: 테스트 실패
8. 테스트 결과 로그 확인:
   - 오른쪽 패널에서 상세 로그 확인
   - 실패한 경우 어떤 부분에서 실패했는지 확인

### 예상 결과 (성공 시)
```
TimecodeSync.Utils.Conversion: PASSED
TimecodeSync.Utils.DropFrame: PASSED
```

## 3. 통합 테스트 실행

### 통합 테스트 준비
1. BP_TimecodeSyncTester 블루프린트 편집
2. BeginPlay 이벤트에서 RunIntegratedTest 함수 호출하도록 설정
3. 저장 후 블루프린트 에디터 닫기

### 통합 테스트 실행
1. 에디터에서 Play 버튼 클릭
2. Output Log 창 열기 (Window → Developer Tools → Output Log)
3. 테스트 결과 로그 확인
4. 화면에 표시되는 테스트 결과 확인

### 통합 테스트 항목
통합 테스트는 다음 항목들을 순차적으로 실행합니다:
1. UDP 연결 테스트
2. 메시지 직렬화 테스트
3. 패킷 손실 테스트
4. 마스터/슬레이브 동기화 테스트
5. 다양한 프레임 레이트 테스트
6. 시스템 시간 동기화 테스트
7. 자동 역할 감지 테스트

### 예상 결과 (모든 테스트 성공 시)
```
=== Starting Integrated TimecodeSync Tests ===
- UDP Connection: PASSED
- Message Serialization: PASSED
- Packet Loss Handling: PASSED
- Master/Slave Sync: PASSED
- Multiple Frame Rates: PASSED
- System Time Sync: PASSED
- Auto Role Detection: PASSED
=== TimecodeSync Test Results ===
Passed Tests: 7/7 (100.0%)
```

## 4. 개별 테스트 실행 (필요 시)

### UDP 연결 테스트
1. BP_TimecodeSyncTester 블루프린트 편집
2. 이벤트 그래프에서 다음 로직 구성:
   ```
   Event BeginPlay
     |
     +--> [Create Object from Class (UTimecodeSyncNetworkTest)]
           |
           +--> [TestUDPConnection 함수 호출, 포트 번호 12345 입력]
   ```
3. Play 버튼 클릭
4. Output Log에서 결과 확인

### 메시지 직렬화 테스트
1. BP_TimecodeSyncTester 블루프린트 편집
2. 이벤트 그래프에서 다음 로직 구성:
   ```
   Event BeginPlay
     |
     +--> [Create Object from Class (UTimecodeSyncNetworkTest)]
           |
           +--> [TestMessageSerialization 함수 호출]
   ```
3. Play 버튼 클릭
4. Output Log에서 결과 확인

### 마스터/슬레이브 동기화 테스트
1. BP_TimecodeSyncTester 블루프린트 편집
2. 이벤트 그래프에서 다음 로직 구성:
   ```
   Event BeginPlay
     |
     +--> [Create Object from Class (UTimecodeSyncLogicTest)]
           |
           +--> [TestMasterSlaveSync 함수 호출, 지속시간 3.0 입력]
   ```
3. Play 버튼 클릭
4. Output Log에서 결과 확인

## 5. 멀티 인스턴스 테스트

### 로컬 테스트 환경 구성
1. 새 레벨 생성 (File → New Level → Empty Level)
2. 두 개의 BP_TimecodeSyncTester 인스턴스 배치
3. 각 인스턴스 선택 후 디테일 패널에서 TimecodeComponent 설정:
   - 인스턴스 1:
     - Role Mode: Manual
     - Is Manually Master: True
   - 인스턴스 2:
     - Role Mode: Manual
     - Is Manually Master: False
     - Master IP Address: "127.0.0.1"
4. Play 버튼 클릭
5. 두 인스턴스의 동기화 상태 관찰

### 다중 컴퓨터 테스트 (선택 사항)
1. 두 대 이상의 컴퓨터에 동일한 프로젝트 설정
2. 각 컴퓨터에 BP_TimecodeSyncTester 설정:
   - 컴퓨터 1:
     - Role Mode: Manual
     - Is Manually Master: True
   - 컴퓨터 2:
     - Role Mode: Manual
     - Is Manually Master: False
     - Master IP Address: 컴퓨터 1의 IP 주소
3. 양쪽에서 동시에 Play 실행
4. 동기화 상태 확인

## 6. 디버깅 프로세스

### 실패한 테스트 분석
1. 실패한 테스트 항목 식별
2. Output Log에서 관련 오류 메시지 확인
3. 실패 원인 파악을 위한 로그 분석

### 코드 검토 포인트
1. **UDP 연결 문제**:
   - `UTimecodeNetworkManager::Initialize()` 함수 검토
   - 소켓 생성 및 바인딩 코드 확인
   - 포트 설정 및 충돌 확인

2. **메시지 직렬화 문제**:
   - `FTimecodeNetworkMessage::Serialize()` 함수 검토
   - `FTimecodeNetworkMessage::Deserialize()` 함수 검토
   - 데이터 타입 및 바이트 순서 처리 확인

3. **동기화 문제**:
   - `UTimecodeComponent::SyncOverNetwork()` 함수 검토
   - 타임코드 생성 및 처리 로직 확인
   - 이벤트 핸들러 연결 상태 확인

4. **자동 역할 감지 문제**:
   - `UTimecodeNetworkManager::DetermineRole()` 함수 검토
   - IP 기반 우선순위 로직 확인
   - 충돌 해결 메커니즘 검토

### 로그 추가 포인트
디버깅을 위해 다음 지점에 로그를 추가합니다:

1. 네트워크 메시지 송수신 지점:
   ```cpp
   // 메시지 전송 지점
   UE_LOG(LogTimecodeNetwork, Display, TEXT("Sending message: Type=%d, Timecode=%s"), 
     (int32)Message.MessageType, *Message.Timecode);

   // 메시지 수신 지점
   UE_LOG(LogTimecodeNetwork, Display, TEXT("Received message: Type=%d, Timecode=%s"), 
     (int32)Message.MessageType, *Message.Timecode);
   ```

2. 역할 결정 지점:
   ```cpp
   UE_LOG(LogTimecodeNetwork, Display, TEXT("Role determination: IP=%s, Result=%s"), 
     *LocalIP, bIsMaster ? TEXT("MASTER") : TEXT("SLAVE"));
   ```

3. 타임코드 생성 및 처리:
   ```cpp
   UE_LOG(LogTimecodeComponent, Display, TEXT("Generated timecode: %s (%.3f seconds)"), 
     *GeneratedTimecode, ElapsedTimeSeconds);
   ```

## 7. 특정 테스트 실패 시 해결 방법

### UDP 연결 실패
1. 방화벽 설정 확인
2. 포트 번호 충돌 확인 (다른 포트 번호 시도)
3. `UTimecodeNetworkManager::Initialize()` 함수에서 소켓 초기화 코드 점검
4. 로컬호스트(127.0.0.1) 통신이 가능한지 확인

### 메시지 직렬화 실패
1. `FTimecodeNetworkMessage` 구조체 정의 확인
2. 직렬화/역직렬화 함수 로직 검토
3. 문자열 인코딩/디코딩 처리 확인
4. 메모리 할당 및 버퍼 크기 확인

### 마스터/슬레이브 동기화 실패
1. 네트워크 메시지 송수신 확인
2. 역할 설정이 올바른지 확인
3. 타임코드 생성 및 처리 로직 검토
4. 이벤트 핸들러가 올바르게 연결되었는지 확인

### 자동 역할 감지 실패
1. 자동 역할 감지 알고리즘 검토
2. IP 기반 우선순위 로직 확인
3. 네트워크 설정 확인
4. 두 인스턴스가 서로 통신 가능한지 확인

## 8. 보고서 작성

테스트 완료 후 다음 내용을 포함한 보고서를 작성합니다:

1. **테스트 요약**
   - 전체 테스트 수
   - 성공한 테스트 수/비율
   - 실패한 테스트 수/비율

2. **세부 테스트 결과**
   - 각 테스트 항목의 결과
   - 실패한 항목의 오류 내용

3. **발견된 문제점**
   - 중요도별 문제 목록
   - 각 문제의 재현 방법

4. **해결된 문제**
   - 해결된 문제 목록
   - 적용된 해결 방법

5. **잔여 문제 및 향후 계획**
   - 미해결 문제 목록
   - 해결 계획 및 우선순위
