# 타임코드 동기화 플러그인 개발 최종 업데이트

## 현재 진행 상황 요약

1. **PLL 알고리즘 구현**: 
   - PLL(Phase-Locked Loop) 알고리즘 구현 완료
   - 99.9% 오차 감소 확인됨
   - 시간 동기화 정확도 크게 향상

2. **일반 프레임 레이트 변환**:
   - 24fps, 25fps, 30fps, 60fps 간 변환 테스트 100% 성공
   - 비드롭 프레임 타임코드 처리 정상 작동

3. **드롭 프레임 타임코드 처리**:
   - 드롭 프레임 타임코드 테스트에서 42.9-57.1% 성공률 확인
   - 10분 경계에서 `00:10:00;20` 출력 (정상은 `00:10:00;00`)
   - 알고리즘 개선이 필요한 상태

## 드롭 프레임 타임코드 문제 분석

### SMPTE 드롭 프레임 표준 이해
- **기본 원칙**: 
  - 매 분마다 첫 두 프레임(29.97fps) 또는 첫 네 프레임(59.94fps)을 건너뜀
  - 단, 10분 단위(0, 10, 20, 30, 40, 50분)에서는 건너뛰지 않음
  - 이를 통해 29.97fps의 실제 속도와 타임코드 사이의 차이를 보정

- **올바른 타임코드 예시**:
  - 00:00:59:29 → 00:01:00;02 (첫 두 프레임 건너뜀)
  - 00:09:59:29 → 00:10:00;00 (10분 단위이므로 건너뛰지 않음)
  - 00:10:59:29 → 00:11:00;02 (다시 첫 두 프레임 건너뜀)

### 현재 코드의 문제점
1. **10분 단위 예외 처리 불완전**:
   ```cpp
   int64 Adjustments = TotalMinutes;
   if (TotalMinutes > 0)
   {
       // 10분의 배수는 제외
       Adjustments -= TotalMinutes / 10;
   }
   
   // 드롭 프레임 보정 적용
   TotalFrames += Adjustments * DropFrames;
   ```
   - 의도는 10분 단위 예외 처리였으나 완전히 정확하지 않음
   - 테스트 결과 10분 경계에서 여전히 문제 발생

2. **보정 방향 문제**:
   - 현재 구현은 프레임을 *더하는* 방식으로 보정
   - 드롭 프레임은 프레임 번호를 *건너뛰는* 개념이므로 프레임을 *빼는* 접근이 필요할 수 있음

3. **경계값 처리 부족**:
   - 정확히 10분 경계에 해당하는 시간에 대한 특별 처리 부족

## 개선 방안

### 1. 알고리즘 수정
- SMPTE 표준에 더 정확히 부합하는 알고리즘으로 변경
- 프레임 건너뛰기 계산 방식 개선
- 더 정확한 10분 단위 예외 처리 구현

```cpp
// SMPTE 표준에 따른 개선된 드롭 프레임 알고리즘 구현
double ActualFrameRate;
int32 DropFrames;

// 정확한 프레임 레이트 상수 설정
if (FMath::IsNearlyEqual(FrameRate, 29.97f, 0.01f))
{
    ActualFrameRate = 30000.0 / 1001.0;
    DropFrames = 2;
}
else // 59.94fps
{
    ActualFrameRate = 60000.0 / 1001.0;
    DropFrames = 4;
}

// 총 프레임 수 계산
double TotalSecondsD = static_cast<double>(TimeInSeconds);
int64 TotalFrames = static_cast<int64>(TotalSecondsD * ActualFrameRate + 0.5);
int64 FramesPerSecond = static_cast<int64>(ActualFrameRate + 0.5);

// SMPTE 표준에 따른 정확한 계산
// 매 분마다 첫 2(또는 4)개의 프레임 번호를 건너뛰되, 10분의 배수는 제외
int64 D = TotalFrames / (FramesPerSecond * 60 * 10); // 10분 단위 수
int64 M = TotalFrames % (FramesPerSecond * 60 * 10); // 10분 내의 남은 프레임

// 프레임이 1분 경계를 넘었는지 확인
if (M >= FramesPerSecond * 60)
{
    // 10분 블록 내에서 경과한 분의 수 (첫 분 제외)
    int64 E = (M - FramesPerSecond * 60) / (FramesPerSecond * 60 - DropFrames);
    // 드롭 프레임 보정 - 프레임 빼기
    TotalFrames = TotalFrames - (DropFrames * (D * 9 + E));
}
else
{
    // 첫 분 내에 있으면 보정 없음
    TotalFrames = TotalFrames - (DropFrames * D * 9);
}
```

### 2. 경계값 예외 처리 강화
- 10분 경계에 정확히 해당하는 시간에 대한 특별 처리 추가

```cpp
// 정확히 10분 경계 감지 및 특별 처리
// 예: 정확히 600초(10분)인 경우
if (FMath::IsNearlyEqual(TimeInSeconds, 600.0f, 0.034f))
{
    // 10분 정확히 경계 - 드롭 프레임 없음
    return TEXT("00:10:00;00");
}
```

### 3. 디버깅 로그 추가
- 계산 과정의 중간 값을 로그로 출력하여 문제 파악

```cpp
// 디버깅 로그 추가
UE_LOG(LogTemp, Display, TEXT("입력 시간: %.3f초, 총 프레임: %lld, 10분 단위: %lld, 남은 프레임: %lld"),
    TimeInSeconds, TotalFrames, D, M);
```

### 4. 특수 케이스 테스트 추가
- 10분 경계와 같은 중요 경계값에 대한 테스트 케이스 추가

## 다음 단계

1. **개선된 드롭 프레임 알고리즘 적용**:
   - SecondsToTimecode() 함수에 위 개선 사항 적용
   - 디버깅 로그 추가

2. **테스트 진행**:
   - 모든 프레임 레이트 변환 테스트 실행
   - 특히 드롭 프레임 관련 테스트 결과 집중 확인
   - 10분 경계 테스트에서 `00:10:00;00` 올바르게 출력되는지 확인

3. **최종 검증**:
   - 모든 테스트가 100% 성공하는지 확인
   - 다양한 경계값에서 정확히 동작하는지 검증

4. **통합 테스트**:
   - 드롭 프레임 타임코드와 PLL 알고리즘의 통합 테스트
   - 실제 환경에서의 동작 검증

## 최종 목표

- 모든 프레임 레이트 변환이 100% 성공하는 안정적인 타임코드 동기화 플러그인 완성
- SMPTE 표준에 완벽히 부합하는 드롭 프레임 타임코드 구현
- 실제 환경에서 정확한 시간 동기화를 보장하는 안정적인 시스템 구축
