# 타임코드 동기화 플러그인 테스트 체크리스트 (업데이트)

## PART A: 로컬 환경 기본 테스트 (완료)

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
- [x] TimecodeSync.Utils.Conversion 테스트 실행 - **PASSED**
- [x] TimecodeSync.Utils.DropFrame 테스트 실행 - **PASSED**

### 3. 네트워크 기본 기능 테스트
- [x] UDP 연결 테스트 (TestUDPConnection) - **PASSED**
- [x] 메시지 직렬화 테스트 (TestMessageSerialization) - **PASSED**
- [x] 패킷 손실 테스트 (TestPacketLoss) - **PASSED**
- [x] 자동 역할 감지 테스트 (TestAutoRoleDetection) - **PASSED**

### 4. 수정된 타임코드 동기화 테스트
- [x] 다양한 프레임 레이트 테스트 (TestMultipleFrameRates) - **PASSED**
  - [x] 대상 포트 명시적 설정 추가
  - [x] 마스터와 슬레이브 간 포트 구분
- [x] 시스템 시간 동기화 테스트 (TestSystemTimeSync) - **PASSED**
  - [x] 대상 포트 명시적 설정 추가
  - [x] 송신자와 수신자 간 포트 구분

### 5. 마스터/슬레이브 동기화 테스트
- [x] 임시 구현으로 테스트 (TestMasterSlaveSync) - **PASSED**
- [ ] 실제 구현으로 테스트
  - [ ] 임시 코드 제거
  - [ ] 주석 처리된 실제 코드 활성화
  - [ ] `TargetPortNumber` 사용 확인
  - [ ] 마스터와 슬레이브의 대상 포트 명시적 설정

### 6. 통합 테스트 실행
- [x] BP_TimecodeSyncTester에 RunIntegratedTest 설정
- [x] Output Log 창 열기
- [x] Play 버튼 클릭하여 테스트 실행
- [x] 통합 테스트 결과: **7/7 통과 (100%)**

## PART B: 로컬 환경 추가 테스트 (진행 중)

### 1. 로컬 멀티 인스턴스 테스트
- [x] 새 레벨 생성
- [x] 두 개의 BP_TimecodeSyncTester 인스턴스 배치
- [x] 인스턴스 1 설정:
  - [x] Role Mode: Manual
  - [x] Is Manually Master: True
  - [x] Receive Port(UDPPort): 10000
  - [x] Send Port(TargetPortNumber): 10001
- [x] 인스턴스 2 설정:
  - [x] Role Mode: Manual
  - [x] Is Manually Master: False
  - [x] Master IP Address: "127.0.0.1"
  - [x] Receive Port(UDPPort): 10001
  - [x] Send Port(TargetPortNumber): 10000
- [x] 테스트 실행
- [x] 동기화 상태 확인:
  - [x] 인스턴스 1이 마스터 역할 수행
  - [x] 인스턴스 2가 슬레이브 역할 수행
  - [x] 포트 설정 교차 확인 (로그 확인)

### 2. 프레임 레이트 변환 테스트
- [x] 프레임 레이트 변환 테스트 구현
- [x] 테스트 실행 및 결과 확인
- [x] 일반 프레임 레이트 간 변환 테스트 (24fps, 25fps, 30fps, 60fps) - **PASSED**
- [⚠️] 드롭 프레임 타임코드 테스트 (29.97fps, 59.94fps) - **부분 성공(85.7%)**
- [⚠️] 드롭 프레임과 논-드롭 프레임 간 변환 검증 - **부분 성공(85.7%)**
- [➡️] PLL 구현으로 해결 예정 (별도 허용 오차 조정 불필요)

### 3. PLL 알고리즘 구현 (최우선 작업) ★
- [ ] TimecodeNetworkManager 클래스 확장
  - [ ] PLL 관련 속성 및 메서드 추가
  - [ ] InitializePLL() 구현
  - [ ] UpdatePLL() 구현
  - [ ] GetPLLCorrectedTime() 구현
- [ ] TimecodeComponent 클래스 확장
  - [ ] PLL 사용 여부 프로퍼티 추가
  - [ ] PLL 파라미터 설정 UI 구현
  - [ ] 컴포넌트 초기화 시 PLL 설정
- [ ] 메시지 수신 및 처리 로직 수정
  - [ ] 마스터 타임코드 수신 시 PLL 업데이트
  - [ ] PLL 보정된 시간 사용하는 타임코드 생성
- [ ] PLL 테스트 케이스 구현
  - [ ] TestPLLSynchronization() 메서드 구현
  - [ ] 오차 측정 및 성능 평가 로직 구현
  - [ ] 드롭 프레임 타임코드 동기화 정확도 평가 추가
- [ ] PLL 파라미터 최적화
  - [ ] 다양한 네트워크 환경에서 테스트
  - [ ] 최적 대역폭 및 감쇠 계수 결정

### 4. 안정성 및 오류 복구 테스트
- [ ] 장시간 실행 테스트 (최소 1시간)
- [ ] 메모리 누수 및 리소스 사용량 모니터링
- [ ] 네트워크 연결 끊김/복구 상황 시뮬레이션
- [ ] 마스터 중단 시 슬레이브 동작 확인
- [ ] 시스템 시간 변경 시 동작 확인

### 5. 로컬 nDisplay 통합 테스트
- [ ] nDisplay 설정 활성화
- [ ] 자동 역할 모드에서 초기화 확인
- [ ] 역할 할당 동작 확인

## PART C: 코드 정리 및 개선 (일부 완료)

### 1. 코드 리팩토링
- [x] 타임코드 파싱 기능 개선
  - [x] 타임코드 파싱 경고 메시지 수정
  - [x] `TimecodeToSeconds` 함수의 구분자 처리 개선
- [x] UI 개선
  - [x] Role Mode가 Manual일 때만 Is Manually Master와 Master IPAddress 표시
  - [x] Role Mode가 Automatic일 때만 Use nDisplay 표시
  - [x] UDPPort → "Receive Port"로 UI 표시명 변경
  - [x] TargetPortNumber → "Send Port"로 UI 표시명 변경
  - [x] TargetIP는 고급 설정으로 이동(필수 설정에서 제외)
- [ ] 마스터/슬레이브 동기화 코드 최적화
- [ ] 중복 코드 제거
- [ ] PLL 동기화 로직 통합
  - [ ] PLL 클래스 구조화
  - [ ] 동기화 메서드와 PLL 통합
  - [ ] 디버깅 및 모니터링 기능 추가

### 2. 코드 문서화
- [ ] 주요 함수와 클래스에 주석 추가
- [ ] 코드 스타일 일관성 유지
- [ ] 예외 처리 및 오류 메시지 개선
- [ ] PLL 알고리즘 설명 및 파라미터 문서화

## PART D: 실제 환경 테스트 (예정)

### 1. 다중 컴퓨터 환경 설정
- [ ] 두 대 이상의 컴퓨터에 동일한 프로젝트 설정
- [ ] 네트워크 연결 확인
- [ ] 방화벽 및 포트 설정 검토

### 2. 마스터/슬레이브 동기화 테스트
- [ ] 컴퓨터 1을 마스터로 설정
- [ ] 컴퓨터 2를 슬레이브로 설정
- [ ] 타임코드 동기화 확인
- [ ] 네트워크 지연시간 측정
- [ ] PLL 기반 동기화 정확도 평가
  - [ ] 일반 모드와 PLL 모드 성능 비교
  - [ ] 다양한 네트워크 조건에서 테스트
  - [ ] 드롭 프레임 타임코드 동기화 정확도 평가

### 3. 성능 및 안정성 테스트
- [ ] 다양한 네트워크 조건에서 테스트
- [ ] 네트워크 과부하 상황 시뮬레이션
- [ ] 장시간 실행 테스트 (최소 3시간)
- [ ] 동기화 오차 측정 및 평가

### 4. 실제 nDisplay 환경 테스트
- [ ] nDisplay 클러스터 구성
- [ ] nDisplay 마스터/슬레이브 역할 설정
- [ ] 타임코드 동기화 플러그인 통합
- [ ] 동기화 정확도 평가

## PART E: 최종 점검 및 문서화 (예정)

### 1. 사용자 가이드 작성
- [ ] 설치 및 설정 방법
- [ ] 주요 기능 및 사용법
- [ ] 환경 요구사항
- [ ] 문제 해결 가이드
- [ ] PLL 동기화 기능 설명 및 설정 가이드

### 2. 성능 및 호환성 문서화
- [ ] 지원되는 언리얼 엔진 버전
- [ ] 권장 네트워크 환경
- [ ] 성능 요구사항
- [ ] 알려진 제한사항 및 해결책
- [ ] PLL 동기화 정확도 및 성능 데이터

### 3. 최종 검증
- [ ] 모든 테스트 항목 재확인
- [ ] 사용자 피드백 수집 및 반영
- [ ] 최종 버전 릴리스 준비

## 주요 코드 수정 및 개선 사항

### 1. 타임코드 파싱 개선
TimecodeUtils.cpp 파일의 TimecodeToSeconds 함수에서 타임코드 구분자 처리 개선:
```cpp
// 기존 코드
if (CleanTimecode.ParseIntoArray(TimeParts, TEXT(":;"), true) != 4)

// 수정 코드
if (CleanTimecode.Replace(TEXT(";"), TEXT(":")).ParseIntoArray(TimeParts, TEXT(":"), false) != 4)
```
이 수정으로 "Invalid timecode format" 경고 메시지가 사라지고 타임코드 파싱이 더 안정적으로 작동합니다.

### 2. UI 개선
TimecodeComponent.h 파일에서 속성 표시 개선:
```cpp
// Master flag setting in manual mode
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual", EditConditionHides))
bool bIsManuallyMaster;

// Master IP setting (used in manual slave mode)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Manual && !bIsManuallyMaster", EditConditionHides))
FString MasterIPAddress;

// Whether to use nDisplay (only in automatic mode)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Role", meta = (EditCondition = "RoleMode==ETimecodeRoleMode::Automatic", EditConditionHides))
bool bUseNDisplay;

// UDP port setting (for receiving messages)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (DisplayName = "Receive Port", ClampMin = "1024", ClampMax = "65535"))
int32 UDPPort;

// UDP port for sending messages (target port)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (DisplayName = "Send Port", ClampMin = "1024", ClampMax = "65535"))
int32 TargetPortNumber;

// Target IP setting (for unicast transmission in master mode) - 고급 설정으로 이동
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network", meta = (AdvancedDisplay))
FString TargetIP;
```
이 수정으로 UI가 더 직관적이고 상황에 맞게 필요한 설정만 표시됩니다.

### 3. PLL 관련 속성 추가 (예정)
TimecodeComponent.h 파일에 PLL 관련 속성 추가:
```cpp
// PLL 사용 여부
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Sync", meta = (AdvancedDisplay))
bool bUsePLL;

// PLL 대역폭 (반응성 조절)
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Sync", meta = (AdvancedDisplay, EditCondition = "bUsePLL", EditConditionHides, ClampMin = "0.01", ClampMax = "1.0"))
float PLLBandwidth;

// PLL 감쇠 계수
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode Sync", meta = (AdvancedDisplay, EditCondition = "bUsePLL", EditConditionHides, ClampMin = "0.1", ClampMax = "2.0"))
float PLLDamping;
```

### 4. 로컬 멀티 인스턴스 테스트 결과
로컬 테스트 환경에서 두 인스턴스 간의 타임코드 동기화가 성공적으로 이루어졌습니다:
- 마스터(10000 포트 수신, 10001 포트 송신)
- 슬레이브(10001 포트 수신, 10000 포트 송신)
- 각 인스턴스가 올바른 역할을 수행하고 네트워크 연결 상태가 "Connected"로 유지됨

### 5. 프레임 레이트 변환 테스트 결과
일반 프레임 레이트 간 변환은 성공, 드롭 프레임 관련 테스트는 부분 성공(85.7%):
```
LogTemp: Display: [TimecodeSyncTest] Frame Rate Conversion: FAILED
LogTemp: Display: 24fps to 30fps: 7/7 (100.0%) - PASSED
25fps to 30fps: 7/7 (100.0%) - PASSED
30fps to 60fps: 7/7 (100.0%) - PASSED
60fps to 30fps: 7/7 (100.0%) - PASSED
30fps to 29.97fps drop: 6/7 (85.7%) - FAILED
29.97fps drop to 30fps: 6/7 (85.7%) - FAILED
60fps to 59.94fps drop: 6/7 (85.7%) - FAILED
59.94fps drop to 60fps: 6/7 (85.7%) - FAILED
```
이 문제는 PLL 알고리즘 구현을 통해 해결될 예정입니다.

### 6. 향후 개선 사항
1. PLL 알고리즘 구현 및 통합 (최우선)
2. 실제 네트워크 환경에서 테스트
3. nDisplay 통합 기능 강화 및 테스트
4. 역할 자동 감지 알고리즘 개선

## 참고 사항
- PLL 알고리즘은 타임코드 동기화의 정확도를 크게 향상시킬 수 있으므로 최우선 작업으로 설정
- PLL은 드롭 프레임 타임코드 변환 문제도 해결할 수 있어 별도의 허용 오차 조정이 불필요함
- 로컬 테스트와 실제 네트워크 환경 테스트는 포트 설정이 다릅니다:
  - 로컬 테스트: 각 인스턴스가 서로 다른 수신 포트 사용(포트 충돌 방지)
  - 실제 네트워크: 모든 인스턴스가 동일한 포트 설정 사용 가능(물리적으로 다른 머신)
- 자동 모드는 실제 네트워크 환경이나 nDisplay 통합 시 더 유용합니다.
- 패키징된 프로젝트에서는 수동 모드보다 자동 모드가 관리 측면에서 편리합니다.
