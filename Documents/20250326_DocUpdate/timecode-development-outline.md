# 타임코드 동기화 플러그인 개발 아웃라인

## 1. 개요

본 문서는 언리얼 엔진 5.5 기반의 타임코드 동기화 플러그인의 전체적인 개발 개요와 아키텍처를 설명합니다. 이 플러그인은 여러 언리얼 엔진 인스턴스 간의 정확한 시간 동기화와 SMPTE 표준 기반 타임코드 처리 기능을 제공합니다.

### 1.1 플러그인 목적

- 다수의 언리얼 엔진 인스턴스 간 시간 동기화
- SMPTE 타임코드 표준 지원 (드롭 프레임 포함)
- 네트워크 기반 마스터/슬레이브 시간 동기화
- 시간 기반 이벤트 트리거 시스템
- nDisplay와의 통합 지원

### 1.2 주요 구성 요소

- PLL(Phase-Locked Loop) 시간 동기화 모듈
- SMPTE 타임코드 변환 모듈
- 네트워크 통신 관리 모듈
- 이벤트 시스템
- 에디터 인터페이스

## 2. 아키텍처 및 모듈 구조

### 2.1 핵심 모듈

#### 2.1.1 PLL 동기화 모듈
- 담당: 네트워크를 통한 시간 동기화
- 주요 클래스: `UPLLSynchronizer`
- 주요 기능:
  - 시간 차이(위상 오차) 계산
  - 주파수 및 위상 조정을 통한 시간 보정
  - 다양한 네트워크 조건에서의 안정적 동기화

#### 2.1.2 SMPTE 타임코드 모듈
- 담당: 시간 값과 타임코드 형식 간 변환
- 주요 클래스: `USMPTETimecodeConverter`
- 주요 기능:
  - 초 단위 시간을 타임코드 형식으로 변환
  - 타임코드 문자열을 초 단위 시간으로 변환
  - 드롭 프레임 타임코드 처리 (29.97fps, 59.94fps)

#### 2.1.3 네트워크 통신 모듈
- 담당: 시간 정보의 네트워크 전송 관리
- 주요 클래스: `UTimecodeNetworkManager`
- 주요 기능:
  - 마스터/슬레이브 역할 관리
  - UDP 소켓 통신
  - 타임코드 메시지 직렬화/역직렬화
  - 연결 상태 모니터링

### 2.2 통합 컴포넌트

#### 2.2.1 타임코드 컴포넌트
- 담당: 사용자 인터페이스 제공 및 모듈 통합
- 주요 클래스: `UTimecodeComponent`
- 주요 기능:
  - 다양한 동작 모드 지원
  - 핵심 모듈 간 데이터 흐름 관리
  - 시간 기반 이벤트 관리
  - 블루프린트 인터페이스 제공

#### 2.2.2 에디터 모듈
- 담당: 편집기 내 설정 및 모니터링 인터페이스
- 주요 클래스: `FTimecodeSyncEditorModule`, `STimecodeSyncEditorUI`
- 주요 기능:
  - 타임코드 설정 UI
  - 역할 및 네트워크 구성 UI
  - 타임코드 모니터링 도구

## 3. 주요 기능 및 워크플로우

### 3.1 동작 모드

플러그인은 다음 네 가지 동작 모드를 지원합니다:

1. **PLL_Only**: PLL 동기화만 사용하여 시간 동기화
2. **SMPTE_Only**: SMPTE 타임코드 변환만 사용
3. **Integrated**: PLL 동기화와 SMPTE 타임코드 통합 사용
4. **Raw**: 가공되지 않은 시간 값 직접 사용

### 3.2 역할 관리

플러그인은 두 가지 역할 결정 모드를 지원합니다:

1. **자동 모드**:
   - 네트워크 환경에 따라 자동으로 역할 결정
   - nDisplay 환경에서는 nDisplay 역할을 자동으로 감지

2. **수동 모드**:
   - 사용자가 마스터/슬레이브 역할 직접 지정
   - 마스터 IP 주소 설정 가능

### 3.3 데이터 흐름

1. **마스터 노드**:
   - 시간 소스 → (선택적) PLL 처리 → (선택적) SMPTE 변환 → 네트워크 전송

2. **슬레이브 노드**:
   - 네트워크 수신 → (선택적) PLL 처리 → (선택적) SMPTE 변환 → 시간 업데이트

### 3.4 이벤트 시스템

시간 기반 이벤트를 등록하고 트리거하는 메커니즘:

1. 이벤트 등록: 이벤트 이름과 트리거 시간 지정
2. 시간 모니터링: 현재 시간과 등록된 이벤트 시간 비교
3. 이벤트 트리거: 지정된 시간에 이벤트 델리게이트 호출
4. 네트워크 전파: 마스터 노드에서 트리거된 이벤트를 슬레이브로 전파

## 4. 개발 단계

### 4.1 1단계: 모듈화 및 핵심 기능 구현 (완료)

- PLL 모듈 구현
- SMPTE 타임코드 모듈 구현
- 네트워크 통신 기본 기능 구현
- 타임코드 컴포넌트 기본 구조 개발
- 모듈 통합 및 기본 테스트

### 4.2 2단계: 안정성 및 고급 기능 (완료)

- 네트워크 안정성 개선
- 동작 모드 시스템 구현
- 이벤트 시스템 구현
- 에러 처리 및 복구 메커니즘
- 에디터 UI 개발

### 4.3 3단계: 통합 및 최적화 (현재 단계)

- nDisplay 통합 구현
- 패킷 손실 대응 기능 개선
- 테스트 시스템 개발
- 성능 최적화
- 모니터링 및 디버깅 도구 개선

### 4.4 4단계: 문서화 및 배포 준비 (진행 중)

- 사용자 문서 작성
- API 문서 생성
- 예제 프로젝트 개발
- 플러그인 패키징
- 출시 준비

## 5. 주요 API 및 인터페이스

### 5.1 타임코드 컴포넌트 API

```cpp
// 타임코드 시작/중지/리셋
void StartTimecode();
void StopTimecode();
void ResetTimecode();

// 현재 타임코드 상태 조회
FString GetCurrentTimecode() const;
float GetCurrentTimeInSeconds() const;

// 타임코드 이벤트 관리
void RegisterTimecodeEvent(const FString& EventName, float EventTimeInSeconds);
void UnregisterTimecodeEvent(const FString& EventName);
void ClearAllTimecodeEvents();

// 모드 설정
void SetTimecodeMode(ETimecodeMode NewMode);
ETimecodeMode GetTimecodeMode() const;

// 역할 설정
void SetRoleMode(ETimecodeRoleMode NewMode);
void SetManualMaster(bool bInIsManuallyMaster);
bool GetIsMaster() const;

// PLL 설정
void SetUsePLL(bool bInUsePLL);
void SetPLLParameters(float Bandwidth, float Damping);
```

### 5.2 블루프린트 노출 이벤트

```cpp
// 타임코드 변경 이벤트
UPROPERTY(BlueprintAssignable)
FOnTimecodeChanged OnTimecodeChanged;

// 타임코드 이벤트 트리거
UPROPERTY(BlueprintAssignable)
FOnTimecodeEventTriggered OnTimecodeEventTriggered;

// 네트워크 상태 변경
UPROPERTY(BlueprintAssignable)
FOnNetworkConnectionChanged OnNetworkConnectionChanged;

// 역할 변경
UPROPERTY(BlueprintAssignable)
FOnRoleChanged OnRoleChanged;
```

## 6. 사용 시나리오

### 6.1 기본 마스터/슬레이브 시간 동기화

여러 언리얼 엔진 인스턴스 간 시간 동기화:

1. 하나의 인스턴스를 마스터로 설정
2. 다른 인스턴스들을 슬레이브로 설정
3. 네트워크 통신을 통한 시간 동기화
4. PLL 알고리즘을 통한 정확한 시간 조정

### 6.2 방송 환경 통합

방송 장비와의 통합:

1. SMPTE 타임코드 형식 활용 (드롭 프레임 포함)
2. 외부 타임코드 신호와의 동기화
3. 시간 기반 이벤트를 통한 방송 워크플로우 자동화

### 6.3 다중 디스플레이 환경

nDisplay와 통합한 다중 디스플레이 환경:

1. nDisplay 역할 자동 감지
2. 여러 디스플레이 노드 간 시간 동기화
3. 디스플레이 간 콘텐츠 일관성 유지

### 6.4 가상 제작 환경

LED 월 및 카메라 트래킹을 사용한 가상 제작:

1. 여러 LED 월 서버 간 시간 동기화
2. 카메라 트래킹 데이터와 환경 변경 동기화
3. 방송 표준 타임코드와의 통합

## 7. 테스트 및 검증

### 7.1 단위 테스트

- PLL 알고리즘 정확도 테스트
- SMPTE 타임코드 변환 테스트
- 네트워크 메시지 직렬화/역직렬화 테스트

### 7.2 통합 테스트

- 마스터/슬레이브 역할 전환 테스트
- 다양한 네트워크 환경에서의 동기화 테스트
- 패킷 손실 대응 테스트
- 모드 전환 및 동작 테스트

### 7.3 성능 테스트

- CPU 및 메모리 사용량 모니터링
- 네트워크 대역폭 사용량 테스트
- 장시간 실행 안정성 테스트

### 7.4 nDisplay 환경 테스트

- nDisplay 역할 감지 테스트
- 다중 노드 타임코드 동기화 테스트
- 미디어 콘텐츠 동기화 테스트

## 8. 향후 개발 계획

### 8.1 확장 기능

- LTC/MTC 출력 지원
- 외부 제어 프로토콜 통합 (OSC, Art-Net, NDI)
- 고급 중복화 및 장애 복구 시스템
- 고급 모니터링 대시보드

### 8.2 성능 최적화

- 멀티스레딩 최적화
- 메모리 사용량 감소
- 네트워크 프로토콜 효율화

### 8.3 플랫폼 확장

- 다양한 언리얼 엔진 버전 지원
- 다양한 운영체제 플랫폼 지원
- 클라우드 기반 시간 동기화 옵션

## 9. 결론

타임코드 동기화 플러그인은 모듈화된 아키텍처를 통해 PLL 기반 시간 동기화와 SMPTE 타임코드 처리 기능을 제공합니다. 이 플러그인은 다양한 동작 모드와 역할 관리 옵션을 통해 여러 사용 시나리오에 유연하게 대응할 수 있습니다. 현재 3단계 개발이 진행 중이며, nDisplay 통합 및 성능 최적화에 중점을 두고 있습니다.

시간 동기화의 정확성과 안정성을 보장하는 이 플러그인은 다중 서버 환경, 방송 시스템 통합, 가상 제작 환경 등 다양한 분야에서 활용될 수 있습니다. 향후 개발 계획을 통해 더 다양한 기능과 플랫폼 지원이 이루어질 예정입니다.