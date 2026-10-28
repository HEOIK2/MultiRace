# CX - 분노의 질주: 원화 채굴

2명의 플레이어가 제한 시간 동안 차량을 운전하며 원화(코인)를 더 많이 모으는 멀티플레이 게임 (UE 5.5.4, C++, Listen/Dedicated Server).

## 게임 규칙

| 요소 | 내용 |
| --- | --- |
| 코인 | 획득 시 +10원, 2초 뒤 같은 스포너에서 리스폰 |
| 스피드 버프 | 획득 시 추가 최대 속도 +10 km/h |
| 공격 (좌클릭) | 앞 범퍼에서 라인트레이스. 맞은 차량이 5원 이상이면 5원을 빼앗음 |
| 채팅 (Enter) | 입력창 포커스 → Enter로 전송 |
| 진행 | Waiting(첫 접속 후 10초) → Playing(90초) → Finished(최다 원화 우승, 동점은 공동 우승) |

## 데이터 흐름 설계

| 데이터 / 행동 | 소유 | 전달 방식 | 값을 바꾸는 쪽 |
| --- | --- | --- | --- |
| `Coins`, `BonusMaxSpeed` | `ACXPlayerState` | `DOREPLIFETIME_CONDITION(COND_OwnerOnly)` + `OnRep` → HUD | 서버 (`HasAuthority` 검사) |
| `MatchState`, `RemainingTime`, `Winners` | `ACXGameState` | `DOREPLIFETIME` (전체) + `OnRep` → HUD | 서버 (`ACXGameMode` 1초 타이머) |
| 채팅 | - | PC `Server_SendChatMessage` → GameMode `BroadcastChatMessage` → 각 PC `Client_ReceiveChatMessage` | 서버가 중계 (서버에서 재검증) |
| 공격 | - | `Server_Fire` → 라인트레이스 → `ApplyDamage` → 피해자 `TakeDamage`, 이펙트는 `Multicast_PlayFireEffect` | 서버 |
| 아이템 획득 | `ACXItemBase` (복제 액터, `NetUpdateFrequency = 1`) | 서버에서만 오버랩 처리 → `Destroy()` 복제 → 각 머신 `Destroyed()`에서 이펙트 | 서버 |
| 리스폰 | `ACXItemSpawner` (서버 전용) | 아이템 `OnDestroyed` 구독 → 2초 타이머 → 재스폰 | 서버 |
| 추가 최대 속도 적용 | `ACXPawn` | 로컬 클라이언트가 매 틱 `BaseMaxSpeed + BonusMaxSpeed`를 넘으면 가속 입력을 0으로 | 클라이언트 |

## 클래스 구조 (`Source/CX`)

| 클래스 | 역할 |
| --- | --- |
| `ACXGameMode` | 게임 규칙 (서버 전용): 상태 전환, 타이머, 우승자 판정, 채팅 중계 |
| `ACXGameState` | 모두가 공유하는 정보: 게임 상태, 남은 시간, 우승자 |
| `ACXPlayerState` | 플레이어별 정보: 원화, 추가 최대 속도 |
| `ACXPlayerController` | 위젯 생성, HUD 갱신, 채팅 RPC |
| `ACXPawn` | 최대 속도 제한, 공격, 피격 처리 |
| `UCXHUDWidget` / `UCXChatWidget` | HUD / 채팅 위젯 C++ 부모 클래스 |
| `ACXItemBase` → `ACXCoinItem`, `ACXSpeedBuffItem` | 오버랩 아이템 공통 로직 + 각자의 효과 |
| `ACXItemSpawner` | 아이템 스폰 및 리스폰 |

## 실행 및 에셋 연결

필수 입력/위젯 에셋과 BP 연결, 기본 맵의 아이템 배치가 저장되어 있습니다.

- 시작 맵: `VehicleAdvExampleMap` (기본 맵)
- 입력: `IA_Fire` = 좌클릭, `IA_Chat` = Enter (`IMC_Default`)
- HUD: `/Game/MultiRace/UI/WBP_HUD` — 코인, 추가 최대 속도, 상태, 남은 시간, 승패
- 채팅: `/Game/MultiRace/UI/WBP_Chat` — Enter로 입력 시작, Enter로 전송
- 차량 두 종류에 공격 액션 연결, 컨트롤러에 HUD/채팅 클래스 연결
- GameMode BP 두 종류에 `CXGameState`, `CXPlayerState` 연결
- 기본 맵: PlayerStart 3개, 코인 스포너 15개, 속도 버프 스포너 3개
- 아이템/피격 효과: Starter Content의 파티클과 사운드를 기본값으로 사용

에디터에서 플레이어 수 2, `Play As Listen Server`로 실행합니다.
필수 기능 범위이므로 게임 상태별 조작 잠금, 바퀴 피격, 지뢰는 포함하지 않습니다.

### 에셋 설정 재현

`CXSetupCommandlet`는 에디터 전용 설정 도구입니다. 에디터 하단 Cmd 입력란에서
`CX.SetupRequiredAssets`를 실행하면 입력 매핑, 위젯 컴파일, BP 기본값과 기본 맵 설정을 저장합니다.
이미 있는 위젯의 디자인과 기존 스포너 배치는 유지합니다. 플레이를 중지한 상태에서 실행하세요.
에셋이 저장되어 있으므로 일반 플레이 때 이 명령을 실행할 필요는 없습니다.

### 필수 기능 통합 검사

새로운 2인 리슨 서버 PIE를 시작한 뒤 에디터 하단 Cmd 입력란에서 실행합니다.

- `CX.VerifyRequired`: 코인/버프 오버랩, 소유자 전용 복제, 채팅 RPC, 2초 리스폰,
  클라이언트 공격 RPC/라인트레이스, 5원 미만 방어, 타이머 전환, 단독 승리/패배 UI
- PIE를 종료하고 새로 시작한 뒤 `CX.VerifyRequired Tie`: 동일 검사 + 공동 우승 UI

검사 도구는 PIE 월드에서 차량 위치와 코인, 남은 시간을 변경합니다. 편집 중인 맵은 변경하지 않습니다.
결과는 Output Log 및 `Saved/Logs/CX.log`의 `CXVerify PASS/FAIL/COMPLETE`에서 확인합니다.
에디터 전용 코드이므로 게임 빌드에서는 실행되지 않습니다.
