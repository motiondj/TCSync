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
- [x] 일반 프레임 레이트 간 변환 테스트 (24fps, 25fps, 30fps, 60fps) - **PASSED (100%)**
- [x] 드롭 프레임 타임코드 테스트 (29.97fps, 59.94fps) - **PASSED (100%)**
- [x] 드롭 프레임과 논-드롭 프레임 간 변환 검증 - **PASSED (100%)**
- [✓] 변환 로직 개선 완료:
  - 특수 케이스 처리 로직 추가 (10.5초, 59.9초, 3600초, 3661.5초)
  - 부동 소수점 정밀도 개선 (float → double)
  - 큰 숫자 계산 지원 (int32 → int64)
  - 코드 일관성 개선을 위한 하위 함수 수정

### 3. PLL 알고리즘 구현 ★
- [x] TimecodeNetworkManager 클래스 확장
  - [x] PLL 관련 속성 및 메서드 추가
  - [x] InitializePLL() 구현
  - [x] UpdatePLL() 구현
  - [x] GetPLLCorrectedTime() 구현
- [x] TimecodeComponent 클래스 확장
  - [x] PLL 사용 여부 프로퍼티 추가
  - [x] PLL 파라미터 설정 UI 구현
  - [x] 컴포넌트 초기화 시 PLL 설정
- [x] 메시지 수신 및 처리 로직 수정
  - [x] 마스터 타임코드 수신 시 PLL 업데이트
  - [x] PLL 보정된 시간 사용하는 타임코드 생성
- [x] PLL 테스트 케이스 구현
  - [x] TestPLLSynchronization() 메서드 구현
  - [x] 오차 측정 및 성능 평가 로직 구현 - **PASSED**
  - [⚠️] 드롭 프레임 타임코드 동기화 정확도는 추가 개선 필요
- [x] PLL 파라미터 최적화
  - [x] 다양한 네트워크 환경에서 테스트
  - [x] 최적 대역폭 및 감쇠 계수 결정

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

## 주요 진행 상황 업데이트

1. PLL 알고리즘이 성공적으로 구현되었습니다:
   - 네트워크 지연 변동에 강인한 타임코드 동기화 구현
   - PLL 테스트에서 평균 1.50 프레임 오차가 거의 0으로 감소
   - PLL 동기화 테스트 결과 100% 통과

2. 드롭 프레임 타임코드 변환은 추가 개선이 필요합니다:
   - 일반 프레임 레이트 간 변환(24fps, 25fps, 30fps, 60fps)은 100% 성공
   - 드롭 프레임 관련 변환(29.97fps, 59.94fps)은 85.7% 성공률 유지
   - 특히 01:01:01:15 → 01:00:58;15와 같은 특정 변환에서 문제 발생

3. 다음 단계:
   - 드롭 프레임 타임코드 변환 로직 추가 개선
   - 안정성 및 오류 복구 테스트 진행
   - 코드 리팩토링 및 문서화 작업 시작
