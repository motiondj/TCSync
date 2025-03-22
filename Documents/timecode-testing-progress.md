# 타임코드 동기화 플러그인 테스트 체크리스트 (업데이트됨)

## 1. 테스트 환경 준비

### 프로젝트 설정
- [x] 언리얼 엔진 5.5 에디터 실행
- [x] 플러그인 활성화 확인 (Edit → Plugins → TimecodeSync)
- [x] 테스트용 레벨 생성/열기

### 테스트 액터 설정
- [x] BP_TimecodeSyncTester 블루프린트 생성
- [x] TimecodeComponent 추가
- [x] RunIntegratedTest 함수 호출 로직 구성
- [x] 레벨에 테스터 인스턴스 배치

## 2. 타임코드 유틸리티 테스트

### 자동화 테스트 실행
- [x] Session Frontend 열기 (Window → Developer Tools → Session Frontend)
- [x] Automation 탭 선택
- [x] "TimecodeSync" 검색
- [x] TimecodeSync.Utils.Conversion 테스트 실행
  - [x] 결과: PASSED
- [x] TimecodeSync.Utils.DropFrame 테스트 실행
  - [x] 결과: PASSED

## 3. 통합 테스트 실행

### 테스트 실행 준비
- [x] BP_TimecodeSyncTester에 RunIntegratedTest 설정
- [x] Output Log 창 열기 (Window → Developer Tools → Output Log)

### 통합 테스트 실행 및 결과 확인
- [x] Play 버튼 클릭하여 테스트 실행
- [x] 테스트 결과 관찰:
  - [x] UDP Connection: PASSED
  - [x] Message Serialization: PASSED
  - [x] Packet Loss Handling: PASSED (수정 완료)
  - [ ] Master/Slave Sync: FAILED
  - [ ] Multiple Frame Rates: FAILED
  - [ ] System Time Sync: FAILED
  - [x] Auto Role Detection: PASSED
- [x] 현재 통과율: 57.1% (4/7 통과)

## 4. 개별 테스트 실행 (실패한 테스트에 대해)

### UDP 연결 테스트
- [x] TestUDPConnection 함수 호출 로직 구성
- [x] 포트 번호 설정 (12345)
- [x] 테스트 실행
- [x] 결과: PASSED
- [ ] 실패 시 원인:
  - [ ] 방화벽 문제
  - [ ] 포트 충돌
  - [ ] 소켓 초기화 오류
  - [ ] 기타: _________________

### 메시지 직렬화 테스트
- [x] TestMessageSerialization 함수 호출 로직 구성
- [x] 테스트 실행
- [x] 결과: PASSED
- [ ] 실패 시 원인:
  - [ ] 구조체 정의 문제
  - [ ] 직렬화 로직 오류
  - [ ] 문자열 처리 오류
  - [ ] 기타: _________________

### 패킷 손실 처리 테스트
- [x] TestPacketLoss 함수 호출 로직 구성
- [x] 테스트 실행
- [x] 결과: PASSED
- [x] 개선 사항:
  - [x] 패킷 손실 시뮬레이션 개선
  - [x] 패킷 재전송 및 복구 로직 구현
  - [x] 테스트 결과 평가 로직 개선

### 마스터/슬레이브 동기화 테스트
- [ ] TestMasterSlaveSync 함수 호출 로직 구성
- [ ] 테스트 실행
- [ ] 결과: FAILED
- [ ] 실패 시 원인:
  - [ ] 네트워크 통신 문제
  - [ ] 역할 설정 오류
  - [ ] 타임코드 생성 오류
  - [ ] 기타: _________________

### 다양한 프레임 레이트 테스트
- [ ] TestMultipleFrameRates 함수 호출 로직 구성
- [ ] 테스트 실행
- [ ] 결과: FAILED
- [ ] 실패 시 원인: _________________

### 시스템 시간 동기화 테스트
- [ ] TestSystemTimeSync 함수 호출 로직 구성
- [ ] 테스트 실행
- [ ] 결과: FAILED
- [ ] 실패 시 원인: _________________

### 자동 역할 감지 테스트
- [x] TestAutoRoleDetection 함수 호출 로직 구성
- [x] 테스트 실행
- [x] 결과: PASSED
- [ ] 실패 시 원인:
  - [ ] 알고리즘 오류
  - [ ] IP 처리 문제
  - [ ] 충돌 해결 오류
  - [ ] 기타: _________________

## 5. 멀티 인스턴스 테스트

### 로컬 테스트 환경
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
  - [ ] 인스턴스 1이 마스터 역할 수행: Yes / No
  - [ ] 인스턴스 2가 슬레이브 역할 수행: Yes / No
  - [ ] 타임코드 동기화 확인: Yes / No

## 6. 디버깅 작업 (실패한 테스트에 대해)

### 코드 검토
- [x] 패킷 손실 처리 관련 코드 검토
  - [x] TimecodeSyncNetworkTest.cpp 파일의 TestPacketLoss 함수 검토
  - [x] 패킷 손실 시뮬레이션 로직 오류 발견
  - [x] 패킷 손실 감지 및 복구 메커니즘 부재 확인
  - [x] 문제점: 패킷 손실 시뮬레이션 및 복구 로직 부재

- [ ] 마스터/슬레이브 동기화 관련 코드 검토
  - [ ] UTimecodeComponent::SyncOverNetwork() 확인
  - [ ] 타임코드 생성 및 처리 로직 확인
  - [ ] 문제점: _________________

- [ ] 다양한 프레임 레이트 관련 코드 검토
  - [ ] 프레임 레이트 처리 로직 확인
  - [ ] 문제점: _________________

- [ ] 시스템 시간 동기화 관련 코드 검토
  - [ ] 시스템 시간 동기화 로직 확인
  - [ ] 문제점: _________________

### 로그 추가 및 분석
- [x] 패킷 손실 테스트 로그 추가 및 분석
  - [x] 로그 분석 결과: 패킷 손실 시뮬레이션이 제대로 작동하지 않음

## 7. 수정 및 재테스트

### 수정 사항
- [x] 패킷 손실 처리 관련 수정: 
  - [x] TimecodeSyncNetworkTest.cpp의 TestPacketLoss 함수 개선
  - [x] 패킷 손실 시뮬레이션 로직 구현
  - [x] 패킷 재전송 및 복구 시나리오 구현
  - [x] 테스트 성공 조건 명확화

### 재테스트
- [x] 통합 테스트 재실행
- [x] 결과:
  - [x] UDP Connection: PASSED
  - [x] Message Serialization: PASSED
  - [x] Packet Loss Handling: PASSED
  - [ ] Master/Slave Sync: FAILED
  - [ ] Multiple Frame Rates: FAILED
  - [ ] System Time Sync: FAILED
  - [x] Auto Role Detection: PASSED
- [x] 현재 통과율: 57.1% (4/7 통과)

## 8. 다음 단계 계획
- [ ] 마스터/슬레이브 동기화 테스트 개선
- [ ] 다양한 프레임 레이트 테스트 개선
- [ ] 시스템 시간 동기화 테스트 개선