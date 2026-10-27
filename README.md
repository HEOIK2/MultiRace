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

## 에디터 설정

C++ 빌드 후 아래 애셋 작업이 필요합니다.

1. **Input Action** (`Content/VehicleTemplate/Input/Actions`, Value Type: Digital(bool))
   - `IA_Fire`, `IA_Chat` 생성
   - `IMC_Default`에 매핑: `IA_Fire` = Left Mouse Button, `IA_Chat` = Enter
2. **차량 BP** (`SportsCar`, `OffroadCar` 폴더의 차량 BP 모두)
   - Class Defaults → Input: `Fire Action` = `IA_Fire`
   - 필요하면 `Fire Start Point` 컴포넌트 위치를 앞 범퍼 중앙에 맞춤
3. **위젯 BP**
   - `WBP_HUD` (부모 `CXHUDWidget`): TextBlock `CoinsText`, `BonusMaxSpeedText`, `MatchStateText`, `RemainingTimeText`, `ResultText`
   - `WBP_Chat` (부모 `CXChatWidget`): ScrollBox `ChatLogScrollBox`, EditableTextBox `ChatInputTextBox`
   - 이름이 다르면 BindWidget 때문에 위젯 BP 컴파일이 실패합니다.
4. **`BP_VehicleAdvPlayerController`**: `HUD Widget Class` = `WBP_HUD`, `Chat Widget Class` = `WBP_Chat`, `Chat Action` = `IA_Chat`
5. **`BP_VehicleAdvGameMode`**: Game State Class = `CXGameState`, Player State Class = `CXPlayerState`인지 확인
6. **레벨**: `CXItemSpawner`를 여러 개 배치하고 각 인스턴스의 `Item Class`를 `CXCoinItem` 또는 `CXSpeedBuffItem`으로 지정 (지면에서 약 100 정도 띄우기)
7. **테스트**: Number of Players 2, Net Mode `Play As Listen Server` (또는 Client), Editor Preferences → `Use Less CPU when in Background` 해제

아이템·피격 이펙트는 Starter Content(`P_Explosion`, `Collapse_Cue`, `Explosion_Cue`)를 기본값으로 사용하며, BP 서브클래스에서 교체할 수 있습니다.
