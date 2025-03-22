# 타임코드 동기화 플러그인 테스트 체크리스트

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
- [ ] 새 레벨 생성
- [ ] 두 개의 BP_TimecodeSyncTester 인스턴스 배치
- [ ] 인스턴스 1 설정:
  - [ ] Role Mode: Manual
  - [ ] Is Manually Master: True
- [ ] 인스턴스 2 설정:
  - [ ] Role Mode: Manual
  - [ ] Is Manually Master: False
  - [ ] Master IP Address: "127.0.0.1"
- [ ] 테스트 실행
- [ ] 동기화 상태 확인:
  - [ ] 인스턴스 1이 마스터 역할 수행
  - [ ] 인스턴스 2가 슬레이브 역할 수행
  - [ ] 타임코드 동기화 확인

### 2. 프레임 레이트 변환 테스트
- [ ] 24fps, 25fps, 30fps, 60fps 타임코드 변환 테스트
- [ ] 드롭 프레임 타임코드 (29.97fps, 59.94fps) 테스트
- [ ] 드롭 프레임과 논-드롭 프레임 간 변환 검증

### 3. 안정성 및 오류 복구 테스트
- [ ] 장시간 실행 테스트 (최소 1시간)
- [ ] 메모리 누수 및 리소스 사용량 모니터링
- [ ] 네트워크 연결 끊김/복구 상황 시뮬레이션
- [ ] 마스터 중단 시 슬레이브 동작 확인
- [ ] 시스템 시간 변경 시 동작 확인

### 4. 로컬 nDisplay 통합 테스트
- [ ] nDisplay 설정 활성화
- [ ] 자동 역할 모드에서 초기화 확인
- [ ] 역할 할당 동작 확인

## PART C: 코드 정리 및 개선 (예정)

### 1. 코드 리팩토링
- [ ] 마스터/슬레이브 동기화 코드 최적화
- [ ] 포트 설정 관련 로직 개선
- [ ] 중복 코드 제거

### 2. 코드 문서화
- [ ] 주요 함수와 클래스에 주석 추가
- [ ] 코드 스타일 일관성 유지
- [ ] 예외 처리 및 오류 메시지 개선

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

### 2. 성능 및 호환성 문서화
- [ ] 지원되는 언리얼 엔진 버전
- [ ] 권장 네트워크 환경
- [ ] 성능 요구사항
- [ ] 알려진 제한사항 및 해결책

### 3. 최종 검증
- [ ] 모든 테스트 항목 재확인
- [ ] 사용자 피드백 수집 및 반영
- [ ] 최종 버전 릴리스 준비

## 수정 내역 요약

### 1. 주요 수정 사항
1. **TimecodeNetworkManager.cpp**의 `SendTimecodeMessage` 함수 수정:
   - `PortNumber` 대신 `TargetPortNumber` 사용하도록 변경
   - 세 곳에서 포트 설정 부분 수정:
     - 마스터 IP로 전송하는 부분
     - Target IP로 전송하는 부분
     - Multicast로 전송하는 부분

2. **TimecodeSyncLogicTest.cpp**의 `TestMultipleFrameRates` 함수 수정:
   - 대상 포트를 명시적으로 설정하는 코드 추가
   - 마스터와 슬레이브 간 포트 구분

3. **TimecodeSyncLogicTest.cpp**의 `TestSystemTimeSync` 함수 수정:
   - 대상 포트를 명시적으로 설정하는 코드 추가
   - 송신자와 수신자 간 포트 구분

### 2. nDisplay 통합 현황
nDisplay와의 통합 기능이 이미 구현되어 있으며, `TimecodeComponent.cpp` 파일에 다음 기능이 포함됩니다:

1. nDisplay 모듈 지원 여부 검사
2. nDisplay 관련 설정 (bUseNDisplay)
3. nDisplay 역할 확인 함수 (CheckNDisplayRole)
4. 자동 역할 모드에서 nDisplay 역할 활용

nDisplay 환경에서는 nDisplay 클러스터의 마스터 노드가 타임코드 마스터가 되고, 다른 슬레이브 노드들은 타임코드 슬레이브가 되도록 설계되어 있습니다.
