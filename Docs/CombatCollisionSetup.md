# Combat and collision setup

Combat code does not choose collision responses. Configure them in the Blueprint component **Details > Collision** panel. The code only turns a melee weapon box on for the attack window and turns it off afterward. Enemy hits are direct; enemies have no defensive collision mode.

Enemies do not have shields, blocking, parrying, or reactive-block mechanics. Any hit registered against an enemy directly inflicts damage unless that enemy is already dead. Blocking is exclusively a player mechanic.

Stamina is also player-only. Do not add a Stamina component or stamina settings to either enemy Blueprint.

## 1. Project collision channel

Project Settings > Engine > Collision already has the `Damageable` object channel. Use it for static world targets that can take combat damage, such as the stronghold.

## 2. Stronghold

Open `BP_Stronghold` and select `MeshComponent`.

1. Assign the stronghold static mesh.
2. Set **Collision Preset** to `Custom`.
3. Set **Collision Enabled** to `Query and Physics` and enable **Generate Overlap Events**.
4. Set **Object Type** to `Damageable`.
5. Set the response to `WorldDynamic` to **Overlap**. Leave `Pawn` as **Block** if enemies should not walk through the building.
6. Select `HealthComponent` and set **Max Health**.

The mesh asset must have collision geometry. Open the mesh asset and use Collision > Add Simple Collision (or Auto Convex Collision) if it has none.

## 3. Player and melee-enemy weapons

Each combat Blueprint needs exactly one `Box Collision` around the damaging part of its weapon, parented to the weapon mesh. There is nothing to assign: combat finds the single box on its own at BeginPlay. The mage needs none of this (ranged attacks, no weapon box).

For the weapon box: Custom preset, No Collision initially (combat enables it mid-swing), Generate Overlap Events on, WorldDynamic, Pawn + Damageable Overlap, rest Ignore.

Setup mistakes stay loud in the Output Log: no box, more than one box, or a box with no attach parent each log a warning naming the fix.

The attacked player and melee enemy use their capsule (`Pawn`) collision. Do not disable collision on their capsules. This is the setting that lets the player sword hit melee enemies.

For every attack montage, add `IFAnimNotifyStateAttackCollision` over the frames where the weapon actually crosses the target. It calls `BeginAttackCollision` at the start and `EndAttackCollision` at the end. Without this notify state, the weapon box is never enabled and cannot deal damage.

## 4. Mage projectile

Open `BP_MageProjectile` and select `CollisionSphere`.

1. Set **Collision Enabled** to `Query Only` and enable **Generate Overlap Events**.
2. Set **Object Type** to `WorldDynamic`.
3. Set responses to **Pawn**, **Damageable**, and `WorldStatic` to **Overlap**.
4. Set Camera to **Ignore**.

Keep the `AnimNotify_IFLaunchProjectile` in the mage cast montage at the frame where the projectile should appear.

## 5. Enemy attack range

Open each enemy Blueprint, select its class defaults, and set **Enemy > Combat > Combat Range**:

| Enemy | Starting value | Tune toward |
| --- | ---: | --- |
| Melee | 75–125 cm | The weapon's effective reach; lower it when the enemy swings short of the target. |
| Mage | 800–1400 cm | The desired casting distance. |

`Combat Range` is measured from collision surface to collision surface, not just actor pivots. Set it in the enemy Blueprint; do not change C++ for individual enemy types.

## Quick test

1. In Play In Editor, attack a melee enemy with the player sword.
2. Let a melee enemy approach the stronghold. Lower its `Combat Range` if the animation starts before the weapon reaches the mesh.
3. Hit the stronghold with the player weapon and a projectile; its health should decrease.
4. Verify the Output Log has neither `has no WeaponCollisionBox assigned` nor `has no attach parent` warning.
