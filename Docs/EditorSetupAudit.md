# IronField build checklist

Do these in order. Do not delete assets.

## 1. Repair UI

### `WBP_GameOver`

1. Open `WBP_GameOver` > **Designer**.
2. Add a `Canvas Panel` root if empty.
3. Add a `Text` named `ResultTitleText`; enable **Is Variable**.
4. Add a `Button` named `RestartButton`; enable **Is Variable**.
5. Add a `Button` named `MainMenuButton`; enable **Is Variable**.
6. Compile and save.
7. Open `BP_PlayerController` > **Class Defaults**:
   - `HUDWidgetClass` = `WBP_HUD`
   - `GameOverScreenWidgetClass` = `WBP_GameOver`
8. Compile and save.

### `BP_MainMenuGameMode`

1. Open it. If its parent is missing: **File > Reparent Blueprint**.
2. Select `GameModeBase`.
3. Set `PlayerControllerClass` = `BP_MenuPlayerController`.
4. Open `MainMenu` > **World Settings** > `GameMode Override` = `BP_MainMenuGameMode`.
5. Compile and save.

## 2. Equip weapons

### Player: `BP_Player`

1. `SwordMesh > Static Mesh` = `SM_TinyHero_Sword02_PBR`.
2. Attach `SwordMesh` to `Mesh`, then select the sword hand in the socket/bone picker. Align it in the viewport.
3. `ShieldMesh > Static Mesh` = `SM_TinyHero_Shield02_PBR`.
4. Attach `ShieldMesh` to `Mesh`, select the other hand, and align it.
5. Attach `WeaponCollision` to `SwordMesh`.
6. Resize `WeaponCollision` around the blade only.
7. Set **Generate Overlap Events** on; set `Pawn` collision response to **Overlap**.
8. Select inherited `Combat`: `WeaponCollisionBox` = `WeaponCollision`.
9. Compile and save.

If a component is absent: add a `Static Mesh` named `SwordMesh` or `ShieldMesh`, or a `Box Collision` named `WeaponCollision`. Use the same steps above.

### Melee: `BP_MeleeEnemyCharacter`

1. `SwordMesh > Static Mesh` = `SM_RPGHero_Sword01_PBR`; attach to sword hand; align it.
2. Do not add `ShieldMesh` (enemies do not use shields or blocking).
3. Attach `WeaponCollision` to `SwordMesh`; cover the blade; enable overlap events; `Pawn` = **Overlap**.
4. Inherited `Combat > WeaponCollisionBox` = `WeaponCollision`.
5. Compile and save.

### Mage: `BP_MageEnemyCharacter`

1. `StaffMesh > Static Mesh` = `Staff01SM`.
2. Attach `StaffMesh` to `Mesh`, select casting hand, then align it.
3. Do not add `WeaponCollision`.
4. Compile and save.

## 3. Combat

### Player: `BP_Player > Combat`

1. `ComboSteps`: add 3 entries:

| Entry | `AttackMontage` | `StaminaCost` | `Damage` |
| --- | --- | ---: | ---: |
| 0 | `AM_Player_Attack01` | `10.f` | `20.f` |
| 1 | `AM_Player_Attack02` | `10.f` | `20.f` |
| 2 | `AM_Player_Attack03` | `10.f` | `20.f` |

2. `HitReactionMontage` = `AM_Player_GetHit01`.
3. `BlockMontage` = `AM_Player_Defend`.
4. `BlockReactionMontage` = `AM_Player_DefendHit`.
5. `SpinAttackMontage` = `AM_Player_SpinAttack`.
6. In `AM_Player_SpinAttack`, create sections exactly named `Intro`, `Loop`, and `End`.
7. In all player attack montages and `AM_Player_SpinAttack`, add `UIFAnimNotifyStateAttackCollision` over blade-hit frames.
8. Compile and save.

### Melee: `BP_MeleeEnemyCharacter > Combat`

1. `ComboSteps`: add two entries:

| Entry | `AttackMontage` | `Damage` |
| --- | --- | ---: |
| 0 | `Anim_NormalAttack01_RPGHero_Montage` | `20.f` |
| 1 | `Anim_NormalAttack02_RPGHero_Montage` | `20.f` |

2. Add `UIFAnimNotifyStateAttackCollision` to both montages over blade-hit frames.
3. `ComboContinueChances` = `[0.f, 0.f]`.
4. (Note: Enemies do not have blocking or reactive-block mechanics; blocking is exclusively a player mechanic.)
5. Compile and save.

### Mage: `BP_MageEnemyCharacter > Combat`

1. `CastMontage` = `AM_MageAttack`.
2. `ProjectileClass` = `BP_MageProjectile`.
3. Open `AM_MageAttack`; add `UIFAnimNotifyLaunchProjectile` on the cast-release frame.
4. Verify `BP_MageProjectile` parent = `AIFProjectile`.
5. Compile and save.

## 4. Build AI from scratch

### `BB_Enemy`

1. Open `BB_Enemy`; remove old keys if starting clean.
2. Add key: `Object`, name `TargetActor`, base class `Actor`.
3. Save.

### `BT_MeleeEnemy`

1. Set Blackboard Asset = `BB_Enemy`; delete all nodes below Root.
2. Add a `Selector` under Root.
3. Add service `UBTService_IFFindTarget` to the Selector; `TargetActorKey` = `TargetActor`.
4. Add first Selector child:

```text
Sequence Attack
  Decorator UBTDecorator_IFInAttackRange: TargetActorKey = TargetActor
  Task UBTTask_IFAttack: TargetActorKey = TargetActor
```

5. Add second Selector child:

```text
Task UBTTask_IFMoveToTarget: TargetActorKey = TargetActor, RepathInterval = 0.25f
```

6. Save.

### `BT_MageEnemy`

1. Set Blackboard Asset = `BB_Enemy`; delete all nodes below Root.
2. Add a `Selector` under Root.
3. Add service `UBTService_IFFindTarget`; `TargetActorKey` = `TargetActor`.
4. Add these Selector children in this exact order:

```text
Sequence Retreat
  Decorator UBTDecorator_IFTooClose: TargetActorKey = TargetActor; MinRangeFraction = 0.5f
  Task UBTTask_IFMoveAwayFromTarget: TargetActorKey = TargetActor
    SuccessRangeTolerance = 0.95f; RepathInterval = 0.25f
    DestinationDriftThreshold = 120.f; AcceptanceRadius = 50.f

Sequence Attack
  Decorator UBTDecorator_IFInAttackRange: TargetActorKey = TargetActor
  Decorator UBTDecorator_IFHasLineOfSight: TargetActorKey = TargetActor
    TraceHeightOffset = 50.f; TraceChannel = Visibility
  Task UBTTask_IFAttack: TargetActorKey = TargetActor

Task UBTTask_IFMoveToTarget: TargetActorKey = TargetActor; RepathInterval = 0.25f
```

5. Save.

### Assign controllers

| Blueprint | `BehaviorTreeAsset` | `AIData` | `TargetActorKeyName` |
| --- | --- | --- | --- |
| `BP_MeleeEnemyController` | `BT_MeleeEnemy` | `DA_EnemyAI_Melee` | `TargetActor` |
| `BP_MageEnemyController` | `BT_MageEnemy` | `DA_EnemyAI_Mage` | `TargetActor` |

1. `BP_MeleeEnemyCharacter > AI Controller Class` = `BP_MeleeEnemyController`.
2. `BP_MageEnemyCharacter > AI Controller Class` = `BP_MageEnemyController`.
3. On both: Auto Possess AI = **Placed in World or Spawned**.
4. Compile and save.

### AI Data Assets

Each controller references a `UIFEnemyAIData` Primary Data Asset that tunes the shared AI behaviour.
If left unset the controller logs a warning and falls back to the C++ defaults shown below.

**How to create one**
1. Content Browser → **right-click → Miscellaneous → Data Asset**.
2. Pick `IFEnemyAIData` as the class.
3. Name it `DA_EnemyAI_Melee` or `DA_EnemyAI_Mage`.

**Fields and what they do**

| Field | Category | What it controls | Used by |
| --- | --- | --- | --- |
| `MinReattackCooldownSeconds` | `IronField\|AI\|Combat` | Lower bound of the random cooldown (seconds) the controller waits after finishing an attack before allowing the next one. | Both |
| `MaxReattackCooldownSeconds` | `IronField\|AI\|Combat` | Upper bound of that cooldown. Each attack end picks a fresh random value in `[Min, Max]`. | Both |
| `PlayerDetectionRange` | `IronField\|AI\|Targeting` | Distance (cm) within which the enemy can detect and target the player. Beyond this it targets the Stronghold instead. | Both |
| `TargetSwitchMargin` | `IronField\|AI\|Targeting` | Hysteresis (cm) that the other target must be **closer by** before the enemy switches from its current target. Prevents rapid flip-flopping. | Both |

**Recommended values**

| Asset | `MinReattack` | `MaxReattack` | `PlayerDetectionRange` | `TargetSwitchMargin` |
| --- | ---: | ---: | ---: | ---: |
| `DA_EnemyAI_Melee` | `0.6` | `1.2` | `2000` | `250` |
| `DA_EnemyAI_Mage` | `1.5` | `2.5` | `2500` | `250` |

> The mage uses a longer cooldown because its cast montage is slower and it needs time to kite back into range before firing again.

## 5. Player input

In `BP_Player > Class Defaults`:

| Property | Asset |
| --- | --- |
| `DefaultInputMappingContext` | `IMC_Default` |
| `MoveInputAction` | `IA_Move` |
| `LookInputAction` | `IA_Look` |
| `JumpInputAction` | `IA_Jump` |
| `SprintInputAction` | `IA_Sprint` |
| `BlockInputAction` | `IA_Defend` |
| `AttackInputAction` | `IA_Attack` |
| `SpinAttackInputAction` | `IA_Spin` |

Set `DefaultInputMappingPriority = 0`. `IA_Move` and `IA_Look` must be Axis2D.

## 6. MainLevel

1. `MainLevel > World Settings > GameMode Override` = `BP_GameMode`.
2. `BP_GameMode > DefaultPawnClass` = `BP_Player`.
3. `BP_GameMode > PlayerControllerClass` = `BP_PlayerController`.
4. Place one `BP_WaveManager` and one `BP_Stronghold`.
5. Place one or more `BP_EnemySpawnPoint` actors.
6. Add a `NavMeshBoundsVolume`; press `P`; playable floor must become green.
7. `BP_WaveManager > Waves`: add enemy groups with `EnemyClass` = `BP_MeleeEnemyCharacter` or `BP_MageEnemyCharacter`, and `EnemyCount` ≥ `1`.
8. Leave `bAutoStartOnBeginPlay = true`.

## 7. HUD and menu

1. `WBP_StatBar`: `ProgressBar` widget, **Is Variable** on.
  2. `WBP_HUD`: `WBP_StatBar` children named `PlayerHealthBar`, `PlayerStaminaBar`, `StrongholdHealthBar`; **Is Variable** on.
3. `WBP_MainMenu`: buttons named `NormalModeButton`, `UnlimitedModeButton`; **Is Variable** on.
4. `BP_MenuPlayerController > MainMenuWidgetClass` = `WBP_MainMenu`.
5. Project Settings > Maps & Modes > `GameInstanceClass` = `GI_IronField`.
6. `GI_IronField`: `MainMenuLevelName = MainMenu`; `GameplayLevelName = MainLevel`.

## 8. Test

1. Test `MainMenu` buttons.
2. Test player movement, sword, player block, and spin.
3. Test melee chase/attack.
4. Test mage retreat/cast/projectile.
5. Test waves, stronghold loss, and all-waves victory.
