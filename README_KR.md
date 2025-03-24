# 언리얼 엔진 5.5용 타임코드 동기화 플러그인

[English Version](README.md)

![버전](https://img.shields.io/badge/버전-1.0.0-blue)
![UE 버전](https://img.shields.io/badge/언리얼%20엔진-5.5-brightgreen)
![라이센스](https://img.shields.io/badge/라이센스-MIT-green)

## 개요

타임코드 동기화 플러그인은 언리얼 엔진 5.5에서 정밀한 타임코드 관리 및 동기화를 위한 강력한 솔루션을 제공합니다. 이 플러그인은 SMPTE 표준에 따른 드롭 프레임 타임코드 처리를 포함하여 다양한 프레임 레이트 간의 정확한 타임코드 변환을 지원합니다.

## 주요 기능

- **다중 장치 타임코드 동기화**: 높은 정밀도로 여러 장치 간 타임코드 동기화
- **프레임 레이트 변환**: 모든 표준 프레임 레이트(24, 25, 30, 60fps) 간 타임코드 변환
- **드롭 프레임 지원**: 29.97 및 59.94fps를 위한 SMPTE 드롭 프레임 표준의 완전한 구현
- **PLL 알고리즘**: 안정적인 동기화를 위해 99.9% 오차 감소를 제공하는 위상 고정 루프(PLL) 알고리즘
- **실시간 타임코드 생성**: 시스템 또는 네트워크 시간을 기반으로 정확한 타임코드 생성
- **마스터/슬레이브 구성**: 다중 장치 설정을 위한 유연한 구성 옵션
- **네트워크 동기화**: 패킷 손실 처리 기능이 있는 UDP 기반 네트워크 동기화

## 설치 방법

1. 릴리스 페이지 또는 언리얼 엔진 마켓플레이스에서 플러그인 다운로드
2. 프로젝트의 Plugins 폴더(`[프로젝트폴더]/Plugins/`)에 플러그인 압축 해제
3. 언리얼 엔진 재시작
4. Edit → Plugins → Timecode를 통해 플러그인 활성화
5. 플러그인 활성화 후 언리얼 엔진 다시 재시작

## 빠른 시작

### 액터에 타임코드 컴포넌트 추가하기

```cpp
// 새 블루프린트 액터 생성
// Components 패널에서 Timecode Component 추가
// 컴포넌트 설정 구성:

// 블루프린트에서:
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Timecode")
UTimecodeComponent* TimecodeComponent;

// C++에서:
#include "TimecodeComponent.h"

// 액터의 생성자에서:
TimecodeComponent = CreateDefaultSubobject<UTimecodeComponent>(TEXT("TimecodeComponent"));
```

### 기본 타임코드 변환

```cpp
#include "TimecodeUtils.h"

// 초를 타임코드로 변환
float TimeInSeconds = 3600.5f; // 1시간 0.5초
FString Timecode = UTimecodeUtils::SecondsToTimecode(TimeInSeconds, 29.97f, true); // "01:00:00;15" (드롭 프레임)

// 타임코드를 초로 변환
FString InputTimecode = "00:10:00;00";
float Seconds = UTimecodeUtils::TimecodeToSeconds(InputTimecode, 29.97f, true); // 600.0f
```

### 마스터/슬레이브 구성 설정

#### 마스터 장치 구성
```cpp
// 블루프린트에서:
TimecodeComponent->RoleMode = ETimecodeRoleMode::Manual;
TimecodeComponent->bIsManuallyMaster = true;

// C++에서:
TimecodeComponent->SetRoleMode(ETimecodeRoleMode::Manual);
TimecodeComponent->SetIsManuallyMaster(true);
```

#### 슬레이브 장치 구성
```cpp
// 블루프린트에서:
TimecodeComponent->RoleMode = ETimecodeRoleMode::Manual;
TimecodeComponent->bIsManuallyMaster = false;
TimecodeComponent->MasterIPAddress = "192.168.1.100"; // 마스터의 IP 주소

// C++에서:
TimecodeComponent->SetRoleMode(ETimecodeRoleMode::Manual);
TimecodeComponent->SetIsManuallyMaster(false);
TimecodeComponent->SetMasterIPAddress("192.168.1.100");
```

## 구성 옵션

### TimecodeComponent 속성

| 속성 | 설명 | 기본값 |
|----------|-------------|---------------|
| FrameRate | 타임코드 생성에 사용되는 프레임 레이트 | 29.97 |
| bUseDropFrame | 드롭 프레임 타임코드 형식 사용 여부 | true |
| RoleMode | 자동 또는 수동 마스터/슬레이브 할당 | Automatic |
| bIsManuallyMaster | 이 인스턴스가 수동으로 마스터로 설정되었는지 여부 | false |
| MasterIPAddress | 마스터 장치의 IP 주소 | "127.0.0.1" |
| SyncPort | 동기화에 사용되는 UDP 포트 | 12345 |
| SyncInterval | 동기화 패킷 사이의 시간 간격(초) | 1.0 |
| PLLBandwidth | PLL 알고리즘 대역폭 매개변수 | 0.5 |

## 고급 사용법

### 사용자 정의 타임코드 처리

```cpp
// 타임코드 변경 이벤트 구독
TimecodeComponent->OnTimecodeChanged.AddDynamic(this, &AMyActor::HandleTimecodeChanged);

// 타임코드 변경 처리
void AMyActor::HandleTimecodeChanged(const FString& NewTimecode)
{
    // 새 타임코드 처리
    UE_LOG(LogTemp, Log, TEXT("새 타임코드: %s"), *NewTimecode);
}
```

### 네트워크 역할 감지

```cpp
// 자동 역할 감지 설정
TimecodeComponent->RoleMode = ETimecodeRoleMode::Automatic;

// 역할 변경 이벤트 구독
TimecodeComponent->OnRoleChanged.AddDynamic(this, &AMyActor::HandleRoleChanged);

// 역할 변경 처리
void AMyActor::HandleRoleChanged(bool bIsMaster)
{
    if (bIsMaster)
    {
        UE_LOG(LogTemp, Log, TEXT("이 장치가 이제 마스터입니다"));
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("이 장치가 이제 슬레이브입니다"));
    }
}
```

## 테스트

플러그인에는 올바른 작동을 확인하기 위한 포괄적인 테스트 유틸리티가 포함되어 있습니다:

1. Window → Developer Tools → Session Frontend 열기
2. Automation 탭 선택
3. "TimecodeSync"로 필터링
4. 테스트를 실행하여 기능 확인

더 자세한 테스트 지침은 [테스트 가이드](docs/testing-guide_kr.md)를 참조하세요.

## 문제 해결

### 일반적인 문제

| 문제 | 해결책 |
|-------|----------|
| 장치가 동기화되지 않음 | 네트워크 연결 및 방화벽 설정 확인 |
| 잘못된 타임코드 변환 | 프레임 레이트 및 드롭 프레임 설정 확인 |
| 높은 동기화 오류 | PLL 대역폭 매개변수 조정 |
| 네트워크 타임아웃 | IP 주소 및 포트 설정 확인 |

### 로깅

문제 해결을 위해 상세 로깅 활성화:

```cpp
// 프로젝트의 DefaultEngine.ini에서:
[Core.Log]
LogTimecodeSync=Verbose
LogTimecodeComponent=Verbose
LogTimecodeNetwork=Verbose
```

## 기술 세부 정보

### 지원 플랫폼
- Windows
- Mac
- Linux
- iOS
- Android

### 의존성
- 언리얼 엔진 5.5+
- 네트워킹 모듈

## 라이센스

이 플러그인은 MIT 라이센스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE)를 참조하세요.

## 연락처 및 지원

문제, 기능 요청 또는 일반적인 질문:
- [GitHub 저장소](https://github.com/yourusername/timecode-sync)에 이슈 제출
- 이메일: support@yourcompany.com

## 감사의 글

- 타임코드 표준 문서를 제공한 SMPTE
- 테스트 및 피드백을 제공한 언리얼 엔진 커뮤니티
