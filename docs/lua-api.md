# Lua Scripting API

Reference for gameplay scripts in **cpp-engine**. Bindings are registered via sol2 from C++ (`setLuaBindings` / `AddBindings`). EmmyLua stubs for editor autocomplete live in [`scripts/api/engine.d.lua`](../scripts/api/engine.d.lua).

When you add or change a Lua binding, update **this file** and **the stubs**.

---

## Table of contents

1. [Script lifecycle](#script-lifecycle)
2. [Math types](#math-types)
3. [Globals](#globals)
4. [Events](#events)
5. [Entity](#entity)
6. [Components](#components)
7. [Camera](#camera)
8. [Physics](#physics)
9. [Input](#input)
10. [Window & UI](#window--ui)
11. [Animation manager](#animation-manager)
12. [Particles](#particles)
13. [Asset handles](#asset-handles)
14. [ImGui (editor)](#imgui-editor)
15. [Examples](#examples)

---

## Script lifecycle

Entity scripts are `.lua` files assigned to a `LuaScript` component. Each script runs in its own environment.

### Per-script environment

| Name | Type | Description |
|------|------|-------------|
| `gameObject` | `Entity` | The entity this script is attached to |
| `deltaTime` | `number` | Frame delta time (seconds); set before `Update` / `LateUpdate` |
| `variables` | `table` | Optional inspector-editable table (see below) |
| `getEntityFromHandle` | `function` | Resolve an `EntityHandle` to an `Entity` (script-local) |

### Callbacks

| Function | When |
|----------|------|
| `Start()` | Once, first frame the script runs in play mode |
| `Update()` | Every frame while playing (after `Start`) |
| `LateUpdate()` | Every frame after all `Update` calls (camera follow, etc.) |
| `OnCollisionEnter(other)` | Rigid-body collision with `other` (`Entity`) |
| `OnPlayerCollisionEnter()` | Character controller touched this entity |

Collision also publishes bus events: `OnCollisionEnter` (with entity data) and `OnPlayerCollisionEnter`.

### `variables` table

Optional table at the top of a script. Values are editable in the inspector and serialized with the scene.

```lua
variables = {
    WALK_SPEED = 7.0,
    IDLE_ANIM  = "resources/animations/idle.ozz",
    BULLET_PARENT = ehandle(),
    shootSound    = sound(),
}
```

Supported default factories: `tex()`, `model()`, `material()`, `scene()`, `terrainTile()`, `particle()`, `sound()`, `animation()`, `prefab()`, `ehandle()`.

---

## Math types

### `vec3`

```lua
local v = vec3(1, 2, 3)
```

| Member / method | Description |
|-----------------|-------------|
| `x`, `y`, `z` | Components |
| `v:normalize()` | Unit vector (unchanged if zero length) |
| `v:length()` | Magnitude |
| `a:dot(b)` | Dot product |
| `a:cross(b)` | Cross product |

### `vec2`

```lua
local v = vec2(1, 2)  -- .x, .y
```

### `quat`

```lua
local q = quat(w, x, y, z)  -- .w, .x, .y, .z
```

---

## Globals

### Logging

| Function | Log level |
|----------|-----------|
| `print(...)` | debug (`[LuaPrint]`) |
| `debug(...)` | debug (`[LuaDebug]`) |
| `info(...)` | info (`[LuaInfo]`) |

Arguments are converted to strings and space-joined.

### Entities

#### `createEntity(name) → Entity`

Creates a new entity in the current scene.

```lua
local ball = createEntity("Ball0")
```

#### `getPlayerEntity() → Entity|nil`

First entity with `PlayerControllerComponent`, or `nil`.

#### `getEntityFromHandle(handle) → Entity`

**Script-local** (available inside entity scripts). Resolves an `EntityHandle` to a live `Entity`.

```lua
local kids = gameObject:getChildren()
for _, h in pairs(kids) do
    local e = getEntityFromHandle(h)
    if e and e:isValid() then
        -- ...
    end
end
```

### System accessors

| Function | Returns |
|----------|---------|
| `getCamera()` | `Camera` |
| `getPhysics()` | `PhysicsManager` |
| `getInput()` | `Input` |
| `getWindow()` | `Window` |
| `getUI()` | `UI` |
| `getAnimationManager()` | `AnimationManager` |
| `getParticleManager()` | `ParticleManager` |

### `loadAnimation(path) → AnimationHandle`

Loads an animation asset by path.

```lua
local clip = loadAnimation("resources/animations/walk_inplace.anim")
```

---

## Events

### `subscribe(eventName, callback) → subscription id`

```lua
subscribe("TargetHit", function()
    info("target hit")
end)

-- Entity events may pass an entity
subscribe("OnCollisionEnter", function(entity)
    -- ...
end)
```

### `publish(eventName [, data])`

Overloads:

| Call | Data type |
|------|-----------|
| `publish(name)` | none |
| `publish(name, number)` | `float` or `int` |
| `publish(name, bool)` | bool |
| `publish(name, string)` | string |
| `publish(name, vec3)` | `vec3` |
| `publish(name, entity)` | `Entity` (entity event) |

Built-in names used by the engine include `OnCollisionEnter` and `OnPlayerCollisionEnter`. Game scripts may define their own (e.g. `TargetHit`, `AllTargetsDestroyed`).

---

## Entity

```lua
---@class Entity
```

| Method | Returns | Description |
|--------|---------|-------------|
| `isValid()` | `bool` | Entity still exists |
| `getName()` | `string` | Display name |
| `setName(name)` | | |
| `getTag()` | `string` | Tag |
| `setParent(handle)` | | Parent by `EntityHandle` |
| `getHandle()` | `EntityHandle` | This entity's handle |
| `instantiatePrefab(handle)` | `Entity` | Spawn a prefab parented to this entity |
| `getChildren()` | `EntityHandle[]` | Child handles |
| `destroy()` | | Mark for destruction next update |

### Component helpers

For each component type `Name`, Entity has:

| Method | Description |
|--------|-------------|
| `AddName()` | Add component; returns reference |
| `GetName()` | Get component (must exist) |
| `HasName()` | `bool` |
| `RemoveName()` | Remove component |

**Component `Name` values** (Lua method suffix):

| Lua name | C++ type |
|----------|----------|
| `LuaScript` | Script component |
| `ShadowCaster` | Shadow caster |
| `Transform` | Transform |
| `TerrainRenderer` | Terrain |
| `ModelRenderer` | Static model |
| `RigidBodyComponent` | Rigid body |
| `AudioSource` | Audio |
| `AnimationComponent` | Skeletal animation |
| `SkinnedMeshComponent` | Skinned mesh |
| `ParticleSystem` | Particles |
| `PlayerControllerComponent` | Character controller |
| `RmlUIComponent` | RmlUi document |
| `GizmoComponent` | Gizmo |
| `Text3DComponent` | 3D text |
| `PrefabInstance` | Prefab link (`prefab` handle) |
| `EntityMetadata` | Metadata |

```lua
local tr = gameObject:AddTransform()
local rb = gameObject:AddRigidBodyComponent()
if gameObject:HasAnimationComponent() then
    local ac = gameObject:GetAnimationComponent()
end
```

---

## Components

### Transform

World-space fields (as bound):

| Field / method | Type | Description |
|----------------|------|-------------|
| `position` | `vec3` | World position |
| `rotation` | `quat` | World rotation |
| `scale` | `vec3` | World scale |
| `setRotation(quat)` | | |
| `GetEulerAngles()` | `vec3` | Euler angles |

```lua
local tr = gameObject:GetTransform()
tr.scale = vec3(0.5, 0.5, 0.5)
```

### ModelRenderer

| Member / method | Description |
|-----------------|-------------|
| `model` | `ModelHandle` |
| `setModel(path)` | Load model from path string |
| `setMaterial(handle)` | `MaterialHandle` |

```lua
local mr = entity:AddModelRenderer()
mr:setModel("resources/models/sphere.obj")
mr:setMaterial(variables.BULLET_MATERIAL)
```

### RigidBodyComponent

| Method | Description |
|--------|-------------|
| `getPosition()` / `setPosition(vec3)` | Body position |
| `setRotation(quat)` / `setRotationEuler(vec3)` | Orientation |
| `getLinearVelocity()` / `setLinearVelocity(vec3)` / `addLinearVelocity(vec3)` | Linear velocity |
| `getAngularVelocity()` / `setAngularVelocity(vec3)` | Angular velocity |
| `applyForce(vec3)` / `applyImpulse(vec3)` | Forces |
| `applyTorque(vec3)` / `applyAngularImpulse(vec3)` | Torques |
| `setGravityFactor(n)` / `getGravityFactor()` | Gravity scale |
| `activate()` / `deactivate()` / `isActive()` | Sleep state |
| `setKinematic(bool)` / `isKinematic()` | Kinematic mode |
| `moveKinematic(pos, rot, dt)` | Kinematic move |
| `setBoxShape(shape)` | From `BoxShape(...)` |
| `setSphereShape(shape)` | From `SphereShape(...)` |
| `setCapsuleShape(shape)` | From `CapsuleShape(...)` |
| `setCylinderShape(shape)` | From `CylinderShape(...)` |
| `setMeshShape(...)` / `setConvexMeshShape(...)` | Mesh shapes |
| `setCollisionShape` / `setCollisionShapeRef` | Low-level shape set |

```lua
local rb = ball:AddRigidBodyComponent()
rb:setPosition(spawnPos)
rb:setSphereShape(SphereShape(0.25))
rb:addLinearVelocity(vec3(aim.x * speed, aim.y * speed, aim.z * speed))
```

### PlayerControllerComponent

Jolt `CharacterVirtual` wrapper (one player character).

| Method | Description |
|--------|-------------|
| `isOnGround()` | On ground |
| `getPosition()` / `setPosition(vec3)` | Capsule position |
| `getLinearVelocity()` / `setLinearVelocity(vec3)` | Velocity |
| `getGroundVelocity()` | Velocity of supporting ground |
| `setRotation(quat)` / `setRotationEuler(vec3)` | Orientation (euler `.y` = yaw degrees) |
| `setFacingDirection(vec3)` | Yaw so local **+Z** faces `worldDir` on XZ |

```lua
local cr = gameObject:GetPlayerControllerComponent()
cr:setLinearVelocity(new_velocity)
cr:setFacingDirection(vec3(math.sin(yaw), 0, math.cos(yaw)))
```

### AnimationComponent

| Field / property | Description |
|------------------|-------------|
| `skeletonPath` | Skeleton asset path |
| `defaultFadeDuration` | Default crossfade seconds |
| `playbackSpeed` | Global playback scale |
| `speed` | Current clip speed (property) |
| `looping` | Current clip loop (property) |

| Method | Description |
|--------|-------------|
| `setSkeleton(path)` | Load skeleton |
| `play(path\|handle [, loop [, fade]])` | Play clip (default loop=`true`, fade=`-1` → default) |
| `crossfadeTo(path\|handle [, fade])` | Play with loop + fade (default fade = `defaultFadeDuration`) |
| `stop()` | Stop |
| `seek(time)` | Seek (seconds) |
| `isPlaying()` / `isFading()` | State |
| `getTime()` / `getLength()` | Clip time / length |
| `getBlendWeight()` | Current blend weight |
| `getCurrentClip()` | Current clip handle |
| `hasSkeleton()` | Skeleton loaded |
| `jointCount()` | Number of joints |

```lua
local ac = visual:GetAnimationComponent()
ac:setSkeleton("resources/animations/skeleton.ozz")
ac:play("resources/animations/idle.ozz", true, 0.2)
```

### AudioSource

| Field | Description |
|-------|-------------|
| `autoPlay` | Play on start |
| `looping` | Loop |
| `volume` / `pitch` | Playback |
| `isPlaying` | Playing flag |
| `referenceDistance` / `maxDistance` / `rolloffFactor` | 3D attenuation |

| Method | Description |
|--------|-------------|
| `play()` / `stop()` | Control |
| `setSound(handle)` | `SoundHandle` |

```lua
local src = gameObject:GetAudioSource()
src:setSound(variables.shootSound)
src:play()
```

### LuaScript

| Member / method | Description |
|-----------------|-------------|
| `scriptPath` | Path to `.lua` |
| `setScript(entity, path)` | Load script onto entity’s `LuaScript` |
| `setVariable(name, value)` | Set existing `variables` entry only |
| `getVariable(name)` | Get variable, or `nil` |

```lua
local sc = newBall:AddLuaScript()
sc:setScript(newBall, "scripts/bullet.lua")
```

### RmlUIComponent

| Member / method | Description |
|-----------------|-------------|
| `documentPath` | Rml document path |
| `LoadDocument` / `UnloadDocument` | Load / unload |
| `SetVisible(bool)` / `IsVisible()` / `visible` | Visibility |

### ParticleSystem

Bound as a usertype with no extra methods yet; use with `getParticleManager():playEffect(entity)`.

### ShadowCaster / SkinnedMesh / Terrain / Gizmo / EntityMetadata

Add/Get/Has/Remove only unless extended later.

---

## Camera

```lua
local cam = getCamera()
```

| Member / method | Type | Description |
|-----------------|------|-------------|
| `getPosition()` | `vec3` | World position |
| `setPosition(vec3)` | | Set position |
| `getFront()` | `vec3` | Forward direction |
| `yaw` / `pitch` | `number` | Euler degrees |
| `fov` | `number` | Field of view |
| `movementSpeed` | `number` | Editor fly speed |
| `mouseSensitivity` | `number` | Editor look sensitivity |

**Look basis** (from `UpdateCameraVectors`):

- `front.x = cos(yaw) * cos(pitch)`
- `front.y = sin(pitch)`
- `front.z = sin(yaw) * cos(pitch)`

Third-person orbit example: place camera at `pivot - front * distance`.

---

## Physics

```lua
local physics = getPhysics()
```

### `PhysicsManager:getGravity() → vec3`

World gravity.

### `PhysicsManager:raycast(origin, direction, maxDistance) → RaycastHit|nil`

Closest-hit raycast. Direction is normalized internally; length is `maxDistance`.

**Parameters**

| Name | Type | Description |
|------|------|-------------|
| `origin` | `vec3` | World-space start |
| `direction` | `vec3` | Direction (need not be unit) |
| `maxDistance` | `number` | Ray length |

**Returns**

- `nil` if nothing is hit
- On hit, a table:

| Field | Type | Description |
|-------|------|-------------|
| `point` | `vec3` | World hit position |
| `distance` | `number` | Distance from origin along the ray |
| `fraction` | `number` | `distance / maxDistance` in `[0, 1]` |

**Notes**

- Uses the same layer filters as character movement (`MOVING`).
- `CharacterVirtual` is not a rigid body; the player capsule is not hit.
- See `scripts/player_thirdperson.lua` (`LateUpdate`) for orbit camera collision.

```lua
local hit = getPhysics():raycast(pivot, toCamera, desiredDist)
if hit then
    local dist = math.max(hit.distance - 0.2, 0.25)
end
```

### Collision shapes

Factories return shape settings used with `RigidBodyComponent:set*Shape`.

| Factory | Arguments | `getType()` |
|---------|-----------|-------------|
| `SphereShape(radius)` | radius | `"SphereShape"` |
| `BoxShape(halfExtents)` | `vec3` half extents | `"BoxShape"` |
| `CapsuleShape(radius, height)` | radius, height | `"CapsuleShape"` |
| `CylinderShape(radius, height)` | radius, height | `"CylinderShape"` |
| `TriangleShape(a, b, c)` | three `vec3` | `"TriangleShape"` |

```lua
rb:setBoxShape(BoxShape(vec3(30, 1, 30)))
rb:setSphereShape(SphereShape(0.25))
```

---

## Input

```lua
local input = getInput()
```

### Keyboard & mouse

| Method | Description |
|--------|-------------|
| `isKeyPressed(key)` | Held |
| `isKeyReleased(key)` | Not held |
| `isKeyPressedThisFrame(key)` | Edge press |
| `isMousePressed(button)` | Mouse held |
| `isMouseClicked(button)` | Mouse click |
| `getMousePosition()` | `vec2` |
| `getMouseDelta()` | `vec2` frame delta |
| `setMousePosition(vec2)` | Warp cursor |
| `setCursorMode(mode)` | Cursor mode |
| `getCursorMode()` | Current mode |

### Virtual axes (Unity-style)

Named axes combine keyboard, d-pad, and analog sticks. Values are in **[-1, 1]** (except mouse axes, which are pixel deltas).

| Method | Description |
|--------|-------------|
| `getAxis(name)` | Smoothed axis (sensitivity / gravity, like Unity `Input.GetAxis`) |
| `getAxisRaw(name)` | Instant value, no smoothing (like Unity `Input.GetAxisRaw`) — prefer for movement |

#### Built-in axis names

| Name | Positive | Negative | Also from gamepad |
|------|----------|----------|-------------------|
| `"Horizontal"` | `D` / Right arrow | `A` / Left arrow | Left stick X, d-pad L/R |
| `"Vertical"` | `W` / Up arrow | `S` / Down arrow | Left stick Y (up = +1), d-pad U/D |
| `"Mouse X"` | | | Mouse delta X (pixels this frame) |
| `"Mouse Y"` | | | Mouse delta Y (pixels this frame) |
| `"Look Horizontal"` | | | Right stick X |
| `"Look Vertical"` | | | Right stick Y (up = +1) |

Stick axes use a default **deadzone** of `0.2` (tunable via `setGamepadDeadzone`).

```lua
-- Movement (keyboard + left stick + d-pad)
local h = input:getAxisRaw("Horizontal")
local v = input:getAxisRaw("Vertical")

-- Camera look: mouse + right stick
local lookX = input:getMouseDelta().x * mouseSens
          + input:getAxisRaw("Look Horizontal") * padSens
local lookY = input:getMouseDelta().y * mouseSens
          + input:getAxisRaw("Look Vertical") * padSens
```

Unknown axis names return `0`.

### Gamepad

Uses GLFW’s Xbox-layout gamepad mapping. The first connected mapped pad is the default device.

| Method | Description |
|--------|-------------|
| `isGamepadConnected([jid])` | Any / specific joystick present as a gamepad |
| `getGamepadName([jid])` | Human-readable name, or `""` |
| `getGamepadAxis(axis [, jid])` | Raw axis after deadzone (triggers: leave as reported) |
| `isGamepadButtonPressed(button [, jid])` | Held |
| `isGamepadButtonPressedThisFrame(button [, jid])` | Edge press (primary pad) |
| `isGamepadButtonReleasedThisFrame(button [, jid])` | Edge release (primary pad) |
| `setGamepadDeadzone(n)` / `getGamepadDeadzone()` | Stick deadzone in `[0, 0.95]` |

`jid` is a GLFW joystick id (`0` = first). Omit for the active pad.

#### Button constants

| Constant | Xbox / typical |
|----------|----------------|
| `GAMEPAD_A` | A / Cross |
| `GAMEPAD_B` | B / Circle |
| `GAMEPAD_X` | X / Square |
| `GAMEPAD_Y` | Y / Triangle |
| `GAMEPAD_LEFT_BUMPER` / `GAMEPAD_RIGHT_BUMPER` | LB / RB |
| `GAMEPAD_BACK` / `GAMEPAD_START` / `GAMEPAD_GUIDE` | Select / Start / Guide |
| `GAMEPAD_LEFT_THUMB` / `GAMEPAD_RIGHT_THUMB` | Stick click |
| `GAMEPAD_DPAD_UP` / `DOWN` / `LEFT` / `RIGHT` | D-pad |
| `GAMEPAD_CROSS` / `CIRCLE` / `SQUARE` / `TRIANGLE` | PlayStation aliases |

#### Axis constants

| Constant | Description |
|----------|-------------|
| `GAMEPAD_AXIS_LEFT_X` / `LEFT_Y` | Left stick |
| `GAMEPAD_AXIS_RIGHT_X` / `RIGHT_Y` | Right stick |
| `GAMEPAD_AXIS_LEFT_TRIGGER` / `RIGHT_TRIGGER` | Triggers |

```lua
if input:isGamepadConnected() then
    info("Pad: " .. input:getGamepadName())
end

if input:isKeyPressed(KEY_SPACE) or input:isGamepadButtonPressed(GAMEPAD_A) then
    -- jump
end

if input:isGamepadButtonPressedThisFrame(GAMEPAD_X) then
    -- shoot
end
```

### Key / mouse / cursor constants

Keys mirror GLFW: `KEY_W`, `KEY_A`, `KEY_S`, `KEY_D`, `KEY_SPACE`, `KEY_LEFT_SHIFT`, `KEY_E`, `KEY_ESCAPE`, `KEY_F1`…`KEY_F12`, keypad `KEY_KP_*`, etc.

| Constant | Meaning |
|----------|---------|
| `MOUSE_LEFT` / `MOUSE_RIGHT` | Mouse buttons |
| `CURSOR_NORMAL` | Visible cursor |
| `CURSOR_DISABLED` | Captured / FPS style |
| `CURSOR_HIDDEN` | Hidden |

```lua
input:setCursorMode(CURSOR_DISABLED)
local h = input:getAxisRaw("Horizontal")
local v = input:getAxisRaw("Vertical")
if input:isKeyPressedThisFrame(KEY_E) or input:isGamepadButtonPressedThisFrame(GAMEPAD_X) then
    -- shoot
end
```

See `scripts/player_thirdperson.lua` and `scripts/player.lua` for full movement + look wiring.

---

## Window & UI

### Window (`getWindow()`)

| Member / method | Description |
|-----------------|-------------|
| `getWidth()` / `getHeight()` | Window size |
| `getAspectRatio()` / `getTargetAspectRatio()` | Aspect |
| `updateViewportSize()` | Refresh viewport |
| `targetWidth` / `targetHeight` / `targetX` / `targetY` | Target viewport |

### UI (`getUI()`)

| Method | Description |
|--------|-------------|
| `getSelectedEntity()` | Editor-selected `Entity` |

---

## Animation manager

```lua
local am = getAnimationManager()
```

| Member / method | Description |
|-----------------|-------------|
| `drawSkeleton` / `getDrawSkeleton` / `setDrawSkeleton` | Debug skeleton |
| `drawMesh` / `getDrawMesh` / `setDrawMesh` | Draw skinned mesh |

---

## Particles

```lua
getParticleManager():playEffect(entity)
```

Plays the `ParticleSystem` effect on the given entity.

---

## Asset handles

Empty handles for `variables` and assignment. Assigned in the editor or by loading.

| Factory | Type | Methods |
|---------|------|---------|
| `tex()` | `TextureHandle` | `getGuid()`, `isValid()`, `clear()` |
| `model()` | `ModelHandle` | same |
| `material()` | `MaterialHandle` | same |
| `scene()` | `SceneHandle` | same |
| `terrainTile()` | `TerrainTileHandle` | same |
| `particle()` | `ParticleHandle` | same |
| `sound()` | `SoundHandle` | same |
| `animation()` | `AnimationHandle` | same |
| `prefab()` / `prefab(guid)` | `PrefabHandle` | same + `instantiate([parentHandle])` |
| `ehandle()` / `ehandle(guid)` | `EntityHandle` | same |

`instantiatePrefab(handle [, parentHandle])` spawns a prefab into the active scene and returns the root `Entity`. Inner entity GUIDs (parent/child links and script `ehandle` fields that pointed at entities inside the prefab) are remapped; handles that pointed outside the prefab are left unchanged. Asset handles and other script variables are copied as authored.

`entity:instantiatePrefab(handle)` is the same, parenting the instance under `entity`.

### Vectors (1-based index)

`EntityVector`, `TextureVector`, `ModelVector`, `MaterialVector`, `SceneVector`, `TerrainTileVector`, `ParticleVector`, `SoundVector`, `AnimationVector`, `PrefabVector`:

- `push_back(item)`
- `size()`
- `[i]` get/set (Lua **1-based**)

---

## ImGui (editor)

Global table `imgui` (editor helpers; used from `scripts/init.lua`).

| Function | Description |
|----------|-------------|
| `imgui.Begin(name [, flags])` | Begin window → `bool` |
| `imgui.End()` | End window |
| `imgui.Text(text)` | Label |
| `imgui.Button(label)` | Button → `bool` clicked |
| `imgui.SameLine()` | Layout |
| `imgui.Separator()` | Separator |
| `imgui.Checkbox(label, { v = bool })` | Checkbox; mutates `value.v` |
| `imgui.SliderFloat(label, { v = n }, min, max)` | Float slider |
| `imgui.SliderInt(label, { v = n }, min, max)` | Int slider |

`imgui.WindowFlags`: `None`, `NoTitleBar`, `NoResize`, `NoMove`, `NoCollapse`.

Checkbox/slider widgets take a table with field `v` so Lua can pass by reference.

---

## Examples

| Script | Topics |
|--------|--------|
| [`scripts/player_thirdperson.lua`](../scripts/player_thirdperson.lua) | Player controller, orbit camera, raycast pull-in, animation, shooting |
| [`scripts/player.lua`](../scripts/player.lua) | First-person style movement |
| [`scripts/bullet.lua`](../scripts/bullet.lua) | Projectile lifetime |
| [`scripts/animation_example.lua`](../scripts/animation_example.lua) | Clip playback |
| [`scripts/init.lua`](../scripts/init.lua) | Editor init / ImGui |

### Camera collision snippet

```lua
-- From orbit pivot toward camera (-look direction)
local toCamera = vec3(-front.x, -front.y, -front.z)
local hit = getPhysics():raycast(pivot, toCamera, desiredDist)
local maxAllowed = desiredDist
if hit then
    maxAllowed = math.max(hit.distance - 0.2, 0.25)
end
-- Snap in if blocked; ease out when free (see player_thirdperson LateUpdate)
```

---

## Maintaining this doc

1. Add/change binding in C++ (`set_function` / `new_usertype` / `AddBindings`).
2. Update the matching section here (especially **return table fields**).
3. Update [`scripts/api/engine.d.lua`](../scripts/api/engine.d.lua).
4. Prefer a short example when the return shape is non-obvious.

### EmmyLua / LuaLS

Point the language server at the stubs (repo root [`.luarc.json`](../.luarc.json) if present):

```json
{
  "workspace.library": ["scripts/api"]
}
```

Stubs are for the editor only; they are not executed at runtime.
