# 타임코드 동기화 플러그인 테스트 체크리스트

## 1. 테스트 환경 준비

### 프로젝트 설정
- ✅ 언리얼 엔진 5.5 에디터 실행
- ✅ 플러그인 활성화 확인 (Edit → Plugins → TimecodeSync)
- ✅ 테스트용 레벨 생성/열기

### 테스트 액터 설정
- ✅ BP_TimecodeSyncTester 블루프린트 생성
- ✅ TimecodeComponent 추가
- ✅ RunIntegratedTest 함수 호출 로직 구성
- ✅ 레벨에 테스터 인스턴스 배치

## 2. 타임코드 유틸리티 테스트

### 자동화 테스트 실행
- ✅ Session Frontend 열기 (Window → Developer Tools → Session Frontend)
- ✅ Automation 탭 선택
- ✅ "TimecodeSync" 검색
- ✅ TimecodeSync.Utils.Conversion 테스트 실행
  - ✅ 결과: PASSED / FAILED
- ✅ TimecodeSync.Utils.DropFrame 테스트 실행
  - ✅ 결과: PASSED / FAILED

## 3. 통합 테스트 실행

### 테스트 실행 준비
- [ ] BP_TimecodeSyncTester에 RunIntegratedTest 설정
- [ ] Output Log 창 열기 (Window → Developer Tools → Output Log)

### 통합 테스트 실행 및 결과 확인
- [ ] Play 버튼 클릭하여 테스트 실행
- [ ] 테스트 결과 관찰:
  - [ ] UDP Connection: PASSED / FAILED
  - [ ] Message Serialization: PASSED / FAILED
  - [ ] Packet Loss Handling: PASSED / FAILED
  - [ ] Master/Slave Sync: PASSED / FAILED
  - [ ] Multiple Frame Rates: PASSED / FAILED
  - [ ] System Time Sync: PASSED / FAILED
  - [ ] Auto Role Detection: PASSED / FAILED
- [ ] 최종 통과율: ____%

## 4. 개별 테스트 실행 (실패한 테스트에 대해)

### UDP 연결 테스트
- [ ] TestUDPConnection 함수 호출 로직 구성
- [ ] 포트 번호 설정 (12345)
- [ ] 테스트 실행
- [ ] 결과: PASSED / FAILED
- [ ] 실패 시 원인:
  - [ ] 방화벽 문제
  - [ ] 포트 충돌
  - [ ] 소켓 초기화 오류
  - [ ] 기타: _________________

### 메시지 직렬화 테스트
- [ ] TestMessageSerialization 함수 호출 로직 구성
- [ ] 테스트 실행
- [ ] 결과: PASSED / FAILED
- [ ] 실패 시 원인:
  - [ ] 구조체 정의 문제
  - [ ] 직렬화 로직 오류
  - [ ] 문자열 처리 오류
  - [ ] 기타: _________________

### 마스터/슬레이브 동기화 테스트
- [ ] TestMasterSlaveSync 함수 호출 로직 구성
- [ ] 테스트 실행
- [ ] 결과: PASSED / FAILED
- [ ] 실패 시 원인:
  - [ ] 네트워크 통신 문제
  - [ ] 역할 설정 오류
  - [ ] 타임코드 생성 오류
  - [ ] 기타: _________________

### 자동 역할 감지 테스트
- [ ] TestAutoRoleDetection 함수 호출 로직 구성
- [ ] 테스트 실행
- [ ] 결과: PASSED / FAILED
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
- [ ] UDP 연결 관련 코드 검토
  - [ ] UTimecodeNetworkManager::Initialize() 확인
  - [ ] 소켓 생성 및 바인딩 코드 확인
  - [ ] 문제점: _________________

- [ ] 메시지 직렬화 관련 코드 검토
  - [ ] FTimecodeNetworkMessage::Serialize() 확인
  - [ ] FTimecodeNetworkMessage::Deserialize() 확인
  - [ ] 문제점: _________________

- [ ] 마스터/슬레이브 동기화 관련 코드 검토
  - [ ] UTimecodeComponent::SyncOverNetwork() 확인
  - [ ] 타임코드 생성 및 처리 로직 확인
  - [ ] 문제점: _________________

- [ ] 자동 역할 감지 관련 코드 검토
  - [ ] UTimecodeNetworkManager::DetermineRole() 확인
  - [ ] IP 기반 우선순위 로직 확인
  - [ ] 문제점: _________________

### 로그 추가 및 분석
- [ ] 네트워크 메시지 송수신 로그 추가
- [ ] 역할 결정 과정 로그 추가
- [ ] 타임코드 생성 및 처리 로그 추가
- [ ] 로그 분석 결과: _________________

## 7. 수정 및 재테스트

### 수정 사항
- [ ] UDP 연결 관련 수정: _________________
- [ ] 메시지 직렬화 관련 수정: _________________
- [ ] 마스터/슬레이브 동기화 관련 수정: _________________
- [ ] 자동 역할 감지 관련 수정: _________________

### 재테스트
- [ ] 통합 테스트 재실행
- [ ] 결과:
  - [ ] UDP Connection: PASSED / FAILED
  - [ ] Message Serialization: PASSED / FAILED
  - [ ] Packet Loss Handling: PASSED / FAILED
  - [ ] Master/Slave Sync: PASSED / FAILED
  - [ ] Multiple Frame Rates: PASSED / FAILED
  - [ ] System Time Sync: PASSED / FAILED
  - [ ] Auto Role Detection: PASSED / FAILED
- [ ] 최종 통과율: ____%

## 8. 테스트 보고서 작성

### 보고서 내용
- [ ] 테스트 요약 작성
- [ ] 세부 테스트 결과 정리
- [ ] 발견된 문제점 목록 작성
- [ ] 해결된 문제 목록 작성
- [ ] 잔여 문제 및 향후 계획 작성

### 추가 사항
- [ ] 스크린샷 및 로그 첨부
- [ ] 코드 수정 내용 기록
- [ ] 추가 개선 사항 제안
