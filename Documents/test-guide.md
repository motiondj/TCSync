# 타임코드 동기화 플러그인 테스트 가이드

이 문서는 언리얼 엔진 5.5 타임코드 동기화 플러그인의 테스트 방법에 대한 가이드입니다.

## 1. 테스트 환경 설정

### 테스트 액터 설정

1. 에디터에서 새 레벨 생성
2. 콘텐츠 브라우저에서 우클릭 → Blueprint Class → Actor 선택
3. 이름을 "BP_TimecodeSyncTester"로 지정
4. 블루프린트 에디터에서 Components 패널 → Add Component → TimecodeComponent 추가
5. 다음 이벤트 그래프를 설정:

```
Event BeginPlay
  |
  +--> [Create TimecodeSyncNetworkTest Object]
  |
  +--> [Create TimecodeSyncLogicTest Object]
  |
  +--> Call RunIntegratedTest
```

6. 레벨에 BP_TimecodeSyncTester 인스턴스 배치
7. Play 버튼 클릭하여 테스트 실행

### 자동화 테스트 실행

1. Window → Developer Tools → Session Frontend 열기
2. Automation 탭 선택
3. Filter 필드에 "TimecodeSync" 입력
4. 관련 테스트가 표시됨:
   - TimecodeSync.Utils.Conversion
   - TimecodeSync.Utils.DropFrame
5. Run Tests 버튼 클릭

## 2. 개별 테스트 실행

### 타임코드 유틸리티 테스트

타임코드 변환 기능의 정확성을 테스트합니다.

1. Session Frontend → Automation 탭에서 실행
2. 테스트 항목:
   - 시간 → 타임코드 변환
   - 타임코드 → 시간 변환
   - 드롭 프레임 타임코드 처리

### 네트워크 통신 테스트

UDP 통신 및 메시지 직렬화/역직렬화를 테스트합니다.

1. BP_TimecodeSyncTester 블루프린트에서 실행
2. 테스트 항목:
   - UDP 소켓 연결
   - 메시지 직렬화/역직렬화
   - 패킷 손실 처리

### 마스터/슬레이브 동기화 테스트

마스터/슬레이브 역할 할당 및 동기화를 테스트합니다.

1. BP_TimecodeSyncTester 블루프린트에서 실행
2. 테스트 항목:
   - 마스터/슬레이브 동기화
   - 다양한 프레임 레이트 지원
   - 시스템 시간 동기화
   - 자동 역할 감지

## 3. 멀티 인스턴스 테스트

여러 컴퓨터 또는 프로세스 간 동기화를 테스트합니다.

### 로컬 테스트 (단일 컴퓨터)

1. 새 레벨 생성
2. 두 개의 BP_TimecodeSyncTester 인스턴스 배치
3. 각각 다른 설정 적용:
   - 인스턴스 1: 수동 모드, 마스터 역할
   - 인스턴스 2: 수동 모드, 슬레이브 역할, 마스터 IP 설정
4. Play 버튼 클릭하여 두 인스턴스 간 동기화 확인

### 네트워크 테스트 (다중 컴퓨터)

1. 두 대 이상의 컴퓨터에 동일한 프로젝트 설정
2. 각 컴퓨터에 BP_TimecodeSyncTester 배치
3. 컴퓨터 1: 수동 모드, 마스터 역할
4. 컴퓨터 2: 수동 모드, 슬레이브 역할, 마스터 IP를 컴퓨터 1의 IP로 설정
5. 동시에 실행하여 동기화 확인

## 4. 디버깅 및 문제 해결

### 디버그 로그 활성화

1. 에디터에서 Window → Developer Tools → Output Log 열기
2. 콘솔 명령 입력: `log LogTimecodeComponent verbose`
3. 테스트 실행 중 상세 로그 확인

### 일반적인 문제 해결

1. 연결 문제:
   - 방화벽 설정 확인
   - UDP 포트 열려있는지 확인
   - IP 주소 정확히 입력했는지 확인

2. 타임코드 동기화 문제:
   - 프레임 레이트 설정 일치 여부 확인
   - 드롭 프레임 설정 일치 여부 확인
   - 네트워크 지연시간 확인

3. 역할 감지 문제:
   - 수동 모드에서 마스터/슬레이브 설정 확인
   - 자동 모드에서 IP 주소 우선순위 확인
   - nDisplay 설정 확인 (사용 시)

## 5. 성능 테스트

1. 장시간 동기화 테스트:
   - 여러 시간 동안 테스트 실행
   - 타임코드 드리프트 확인
   - 메모리 누수 확인

2. 네트워크 스트레스 테스트:
   - 높은 부하 상황에서 테스트
   - 대역폭 제한 환경 시뮬레이션
   - 패킷 손실 시나리오 테스트

## 테스트 결과 분석

테스트 결과는 Output Log와 화면에 표시되는 디버그 메시지로 확인할 수 있습니다. 다음 항목을 체크하세요:

1. 단위 테스트 통과 여부
2. 마스터/슬레이브 동기화 정확도
3. 다양한 프레임 레이트 지원
4. 네트워크 안정성
5. 자동 역할 감지 정확도
