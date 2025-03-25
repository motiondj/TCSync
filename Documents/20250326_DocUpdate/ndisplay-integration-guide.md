# nDisplay 환경에서의 타임코드 플러그인 통합 가이드

## 개요

이 문서는 언리얼 엔진의 nDisplay 시스템과 타임코드 동기화 플러그인 간의 통합에 대한 정보를 제공합니다. 타임코드 플러그인을 nDisplay 환경에서 사용하기 위한 설정 방법, 고려사항 및 활용 사례를 설명합니다.

## nDisplay와 타임코드 플러그인의 관계

### 기능적 차이점

nDisplay와 타임코드 플러그인은 서로 다른 종류의 동기화를 제공합니다:

1. **nDisplay**:
   - 여러 디스플레이 노드 간의 렌더링 동기화를 관리
   - 프레임 출력 타이밍 동기화 (하드웨어 또는 네트워크 기반)
   - 카메라 시점 동기화 및 화면 간 콘텐츠 일관성 유지

2. **타임코드 플러그인**:
   - 여러 노드 간의 시간 정보 동기화를 관리
   - PLL(Phase-Locked Loop) 알고리즘을 통한 시간 보정
   - 시간 기반 이벤트의 동기화된 트리거 제공

### 독립적인 작동 방식

타임코드 플러그인은 nDisplay의 `render_sync_policy` 설정에 관계없이 독립적으로 작동합니다:

- **nDisplay 설정**: `"render_sync_policy": "ethernet"` 또는 `"render_sync_policy": "nvidia"`
- **영향**: 타임코드 플러그인은 두 설정 모두에서 정상적으로 작동

즉, NVIDIA Quadro Sync 하드웨어를 사용하는 환경에서도 타임코드 플러그인은 정상적으로 기능합니다.

### 상호 보완적 관계

nDisplay와 타임코드 플러그인은 서로 보완적인 관계를 가집니다:

- **nDisplay**: 프레임 렌더링의 시각적 동기화 담당
- **타임코드 플러그인**: 애플리케이션 로직과 시간 기반 이벤트의 동기화 담당

두 기술을 함께 사용하면 더 완벽한 동기화 시스템을 구축할 수 있습니다.

## nDisplay 역할 감지 기능

타임코드 플러그인은 nDisplay 환경에서 자동으로 역할(마스터/슬레이브)을 감지할 수 있습니다:

```cpp
bool UTimecodeComponent::CheckNDisplayRole()
{
#if WITH_DISPLAYCLUSTER
    // If nDisplay module is available
    if (IDisplayCluster::IsAvailable())
    {
        // Get nDisplay cluster manager
        IDisplayClusterClusterManager* ClusterManager = IDisplayCluster::Get().GetClusterMgr();
        if (ClusterManager)
        {
            // Check if current node is master
            bool bIsNDisplayMaster = ClusterManager->IsMaster();
            return bIsNDisplayMaster;
        }
    }
#endif
    return false; // Default to slave if nDisplay is not available
}
```

이 기능으로 nDisplay 클러스터 설정에 맞게 타임코드 플러그인의 역할이 자동으로 구성됩니다.

## nDisplay 환경에서의 설정 방법

### 1. 프로젝트 설정

1. nDisplay 플러그인 활성화:
   - 편집 → 플러그인 → nDisplay

2. 타임코드 플러그인 설치:
   - 플러그인 폴더에 타임코드 플러그인 복사
   - 플러그인 활성화

3. nDisplay 구성 파일 설정:
   - 편집 → 프로젝트 설정 → nDisplay → Config File: 구성 파일 경로 지정

### 2. 타임코드 컴포넌트 설정

블루프린트에서 다음과 같이 설정:

1. 액터에 타임코드 컴포넌트 추가
2. 컴포넌트 세부 설정:
   - Role Mode: `Automatic Detection` (nDisplay 역할 자동 감지)
   - Use nDisplay: `체크` (활성화)
   - 필요에 따라 프레임 레이트 및 기타 옵션 설정

C++에서는 다음과 같이 설정:

```cpp
UTimecodeComponent* TimecodeComp = CreateDefaultSubobject<UTimecodeComponent>(TEXT("TimecodeComponent"));
TimecodeComp->RoleMode = ETimecodeRoleMode::Automatic;
TimecodeComp->bUseNDisplay = true;
```

## nDisplay 렌더링 동기화 정책 비교

타임코드 플러그인은 다음 nDisplay 설정과 모두 호환됩니다:

### 1. 이더넷 기반 동기화 (`"render_sync_policy": "ethernet"`)

- **원리**: 네트워크 메시지를 통해 프레임 렌더링 시간 동기화
- **장점**: 특별한 하드웨어 필요 없음
- **단점**: 네트워크 지연으로 인한 동기화 정확도 제한
- **타임코드 플러그인**: 정상 작동, PLL 알고리즘으로 네트워크 지연 보상

### 2. NVIDIA Quadro Sync (`"render_sync_policy": "nvidia"`)

- **원리**: 전용 Quadro Sync 하드웨어를 통한 GPU 수준 동기화
- **장점**: 프레임 단위의 정확한 동기화, 최소한의 지연
- **단점**: 전용 하드웨어 필요 (NVIDIA Quadro 카드 및 Sync 보드)
- **타임코드 플러그인**: 정상 작동, 하드웨어 동기화와 함께 더 정확한 시간적 일관성 제공

## 활용 사례

### 1. LED 월 콘텐츠 동기화

다수의 nDisplay 노드로 구성된 LED 월에서:
- nDisplay: 화면 간 렌더링 동기화
- 타임코드 플러그인: 애니메이션, 효과, 이벤트의 시간적 동기화

### 2. 카메라 트래킹 환경

가상 제작 환경에서:
- nDisplay: 다중 화면 콘텐츠 일관성 유지
- 타임코드 플러그인: 카메라 이동에 따른 환경 변화 시간 동기화

### 3. 실시간 퍼포먼스

복잡한 실시간 쇼에서:
- nDisplay: 여러 화면의 출력 동기화
- 타임코드 플러그인: 조명, 음향, 특수효과 큐 동기화

## 고급 설정 및 최적화

### PLL 매개변수 최적화

네트워크 환경에 따라 PLL 매개변수를 조정하여 동기화 품질 개선:

- 대역폭(Bandwidth): 0.01-1.0 사이 값 (작을수록 안정적, 클수록 반응성 높음)
- 감쇠 계수(Damping): 0.1-2.0 사이 값 (1.0이 일반적으로 적정)

```cpp
// C++에서 설정
TimecodeComp->SetPLLParameters(0.1f, 1.0f);
```

또는 블루프린트에서 "Set PLL Parameters" 노드 사용

### 네트워크 설정

nDisplay 클러스터 환경에서 네트워크 설정 최적화:

- 멀티캐스트 그룹을 사용하여 효율적인 메시지 배포
- 동기화 간격을 네트워크 상태에 맞게 조정
- 네트워크 버퍼 크기 최적화

## 주의사항 및 제한

1. **nDisplay와 타임코드 플러그인의 역할 동기화**:
   - 항상 nDisplay의 마스터 노드가 타임코드의 마스터 역할을 하도록 구성
   - 자동 감지 모드가 최적의 구성 보장

2. **네트워크 지연 고려**:
   - 네트워크 품질에 따라 동기화 정확도 영향 받음
   - 고품질 네트워크 환경 권장

3. **초기화 순서**:
   - nDisplay 시스템이 먼저 초기화된 후 타임코드 시스템 초기화
   - 마스터 노드를 먼저 시작한 후 슬레이브 노드 시작

## 결론

nDisplay와 타임코드 플러그인은 서로 독립적으로 작동하지만, 함께 사용할 때 더 강력한 동기화 솔루션을 제공합니다. nDisplay의 렌더링 동기화 방식(`ethernet` 또는 `nvidia`)에 관계없이 타임코드 플러그인은 정상적으로 기능하며, 두 기술을 조합하여 프레임 렌더링과 콘텐츠 타이밍 모두 동기화된 고품질 멀티 디스플레이 환경을 구축할 수 있습니다.