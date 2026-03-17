# Project SZ (공개용 문서)

> UE5(C++) 기반 **4인 개인전 생존/탈출 PvPvE 게임**

<p align="center">
  <img src="https://github.com/user-attachments/assets/fb90d7ea-3718-42c5-bd08-df86ab6cea91" alt="Project SZ Concept" width="100%"/>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Engine-UE%205.5.4-black?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Branch-dev-blue?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Network-Listen%20Server-2ea44f?style=for-the-badge" />
  <img src="https://img.shields.io/badge/Language-C%2B%2B-orange?style=for-the-badge" />
</p>

---

## 개발 현황 (2026.03.08 기준)

<details>
<summary><b>상세 내역 확인 (클릭)</b></summary>
<br/>

### 완료 (Done)

- **캐릭터 및 액션 시스템**
  - `AQPCharacter`: 체력(HP) 및 스태미나 소비/회복 로직 구현 완료
  - `UQPAnimInstance`: 무기 타입별 로코모션 및 사격/근접 공격 애니메이션 연동 완료
- **전투 및 아이템 시스템**
  - **인벤토리**: 아이템 파밍, 습득 및 관리 시스템 구축 완료
  - **전투 컴포넌트**: 근접/사격 기본 판정 및 데미지 전달 로직 완료
- **AI (좀비)**
  - 기본 좀비 AI: 시야 기반 탐지, 추적, 공격 및 사망 처리 로직 구축 완료

### 진행 중 (In Progress)

- **소음 어그로 시스템 고도화**
  - 사운드 발생 지점 전파 및 좀비 군집(Swarm) 로직 구현 중
- **리스폰 및 게임 세션 관리**
  - 플레이어 무력화 시 리스폰 페널티 및 관전 시스템 구축 중
  </details>

---

## 1) 게임 소개 (Overview)

**Project SZ**는 좀비가 점령한 구역에서 4명의 플레이어가 생존 경쟁을 벌이는 **PvPvE 탈출 게임**입니다. 타 플레이어를 처치하는 것은 본인의 탈출을 위한 '지연 수단'이며, 소음으로 유발되는 좀비의 습격을 관리하며 가장 먼저 탈출하는 것이 최종 목표입니다.

---

## 2) 핵심 시스템 (Core Gameplay)

### 사운드 센싱 AI (Sound-Reactive AI)

- **소음 리스크**: 좀비들은 사운드에 매우 민감하며, 총성 발생 시 주변 좀비들이 해당 지점으로 집결하여 공격자에게도 치명적인 탈출 지연을 초래합니다.
- **전략적 침투**: 은밀한 이동(Crouch)과 소음이 적은 무기 활용을 통해 좀비의 주의를 끌지 않는 생존 전략이 필수적입니다.

### 경쟁적 저지 (PvP & Respawn)

- **무력화 및 리스폰**: 타 플레이어를 공격하여 사망 상태로 만들 수 있습니다.
- **지연 전략**: 사망한 플레이어는 리스폰 시간 동안 행동이 불가능해지며, 이를 이용하여 경쟁자의 미션 진행을 저지하고 탈출구를 선점할 수 있습니다.

---

## 3) 기술 스택 (Tech Stack)

- **Engine**: Unreal Engine 5.5.4 (C++)
- **Network**: Listen Server / Replication / RPC (Server Authority)
- **Collaboration**: GitHub (Git LFS) / Discord

---

## 4) 개발 로드맵 (Public Roadmap)

|     단계      |      기간       | 핵심 산출물                                                                   |
| :-----------: | :-------------: | :---------------------------------------------------------------------------- |
| **기능 완성** |  **~2026.02**   | 캐릭터(HP/스태미나), 애니메이션, 좀비 AI 기초, 인벤토리 시스템 완료           |
|   **MVP-4**   | **~2026.03.15** | **리스폰 시스템**: 사망 상태 전이, 리스폰 타이머 및 위치 랜덤화 로직 완성     |
|   **MVP-5**   | **~2026.03.31** | **소음 어그로 고도화**: 총성 기반 좀비 집결 및 구역별 웨이브 유입 시스템 구축 |
|   **Alpha**   | **~2026.04.15** | **탈출 미션**: 탈출구 활성화 오브젝트 상호작용 및 최종 탈출 판정 구현         |
|  **마무리**   | **~2026.04.30** | **최종 최적화**: 4인 멀티플레이 밸런싱 및 프로젝트 최종 데모 촬영             |

---

## 5) 플레이/빌드 (Getting Started)

> 추후 데모 배포 시 상세화 예정

- Unreal Engine 버전: `UE 5.5.4`
- 플랫폼: `Windows`
- 실행/빌드:
  1. 프로젝트 클론
  2. UE 에디터에서 `.uproject` 열기
  3. `Development Editor` 빌드 후 실행

---

## 6) 협업 규칙 (Contributing / Workflow)

- 브랜치 전략: `main`(안정) / `dev`(통합) / `feature/`(기능추가)
- 이슈 트래킹: 디스코드 (재현 절차/스크린샷/로그 포함)
- PR 규칙: 기능 단위 PR, 체크리스트(테스트/리플리케이션/리뷰) 통과 후 머지

---

## 7) 팀 (Team)

- 2인 개발
- 역할 분담 및 책임 범위는 Pre-Prod에서 확정 후 문서로 관리
