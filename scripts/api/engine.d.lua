---@meta
--- EmmyLua / LuaLS stubs for cpp-engine.
--- Editor-only: not loaded at runtime. Keep in sync with docs/lua-api.md and C++ bindings.

--------------------------------------------------------------------------------
-- Math
--------------------------------------------------------------------------------

---@class vec3
---@field x number
---@field y number
---@field z number
local vec3 = {}

---@return vec3
function vec3:normalize() end

---@return number
function vec3:length() end

---@param b vec3
---@return number
function vec3:dot(b) end

---@param b vec3
---@return vec3
function vec3:cross(b) end

---@param x number
---@param y number
---@param z number
---@return vec3
function vec3(x, y, z) end

---@class vec2
---@field x number
---@field y number

---@param x number
---@param y number
---@return vec2
function vec2(x, y) end

---@class quat
---@field w number
---@field x number
---@field y number
---@field z number

---@param w number
---@param x number
---@param y number
---@param z number
---@return quat
function quat(w, x, y, z) end

--------------------------------------------------------------------------------
-- Asset handles
--------------------------------------------------------------------------------

---@class AssetHandle
local AssetHandle = {}

---@return string
function AssetHandle:getGuid() end

---@return boolean
function AssetHandle:isValid() end

function AssetHandle:clear() end

---@class TextureHandle : AssetHandle
---@class ModelHandle : AssetHandle
---@class MaterialHandle : AssetHandle
---@class SceneHandle : AssetHandle
---@class TerrainTileHandle : AssetHandle
---@class ParticleHandle : AssetHandle
---@class SoundHandle : AssetHandle
---@class AnimationHandle : AssetHandle
---@class PrefabHandle : AssetHandle
---@class EntityHandle : AssetHandle

---@param parent? EntityHandle
---@return Entity
function PrefabHandle:instantiate(parent) end

---@return TextureHandle
function tex() end

---@return ModelHandle
function model() end

---@return MaterialHandle
function material() end

---@return SceneHandle
function scene() end

---@return TerrainTileHandle
function terrainTile() end

---@return ParticleHandle
function particle() end

---@return SoundHandle
function sound() end

---@return AnimationHandle
function animation() end

---@param guid? string
---@return PrefabHandle
function prefab(guid) end

---@param handle PrefabHandle
---@param parent? EntityHandle
---@return Entity
function instantiatePrefab(handle, parent) end

---@param guid? string
---@return EntityHandle
function ehandle(guid) end

---@class AssetVector
local AssetVector = {}

---@param item any
function AssetVector:push_back(item) end

---@return integer
function AssetVector:size() end

---@class EntityVector : AssetVector
---@class TextureVector : AssetVector
---@class ModelVector : AssetVector
---@class MaterialVector : AssetVector
---@class SceneVector : AssetVector
---@class TerrainTileVector : AssetVector
---@class ParticleVector : AssetVector
---@class SoundVector : AssetVector
---@class AnimationVector : AssetVector
---@class PrefabVector : AssetVector

--------------------------------------------------------------------------------
-- Physics shapes
--------------------------------------------------------------------------------

---@class ShapeSettings
local ShapeSettings = {}

---@return string
function ShapeSettings:getType() end

---@class SphereShape : ShapeSettings
---@class BoxShape : ShapeSettings
---@class CapsuleShape : ShapeSettings
---@class CylinderShape : ShapeSettings
---@class TriangleShape : ShapeSettings

---@param radius number
---@return SphereShape
function SphereShape(radius) end

---@param half_extent vec3
---@return BoxShape
function BoxShape(half_extent) end

---@param radius number
---@param height number
---@return CapsuleShape
function CapsuleShape(radius, height) end

---@param radius number
---@param height number
---@return CylinderShape
function CylinderShape(radius, height) end

---@param a vec3
---@param b vec3
---@param c vec3
---@return TriangleShape
function TriangleShape(a, b, c) end

--------------------------------------------------------------------------------
-- Raycast
--------------------------------------------------------------------------------

--- Result of PhysicsManager:raycast on hit (nil on miss).
---@class RaycastHit
---@field point vec3 World hit position
---@field normal vec3 Surface normal at hit (world space)
---@field distance number Distance from origin along the ray
---@field fraction number distance / maxDistance in [0, 1]

--------------------------------------------------------------------------------
-- Camera
--------------------------------------------------------------------------------

---@class Camera
---@field yaw number
---@field pitch number
---@field fov number
---@field movementSpeed number
---@field mouseSensitivity number
local Camera = {}

---@return vec3
function Camera:getPosition() end

---@param position vec3
function Camera:setPosition(position) end

---@return vec3
function Camera:getFront() end

---@return Camera
function getCamera() end

--------------------------------------------------------------------------------
-- Physics
--------------------------------------------------------------------------------

---@class PhysicsManager
local PhysicsManager = {}

---@return vec3
function PhysicsManager:getGravity() end

--- Closest-hit raycast. Direction is normalized; length is maxDistance.
---@param origin vec3
---@param direction vec3
---@param maxDistance number
---@return RaycastHit|nil
function PhysicsManager:raycast(origin, direction, maxDistance) end

---@return PhysicsManager
function getPhysics() end

--------------------------------------------------------------------------------
-- Input
--------------------------------------------------------------------------------

---@class Input
local Input = {}

---@param key integer
---@return boolean
function Input:isKeyPressed(key) end

---@param key integer
---@return boolean
function Input:isKeyReleased(key) end

---@param key integer
---@return boolean
function Input:isKeyPressedThisFrame(key) end

---@param button integer
---@return boolean
function Input:isMousePressed(button) end

---@param button integer
---@return boolean
function Input:isMouseClicked(button) end

---@return vec2
function Input:getMousePosition() end

---@return vec2
function Input:getMouseDelta() end

---@param pos vec2
function Input:setMousePosition(pos) end

---@param mode integer
function Input:setCursorMode(mode) end

---@return integer
function Input:getCursorMode() end

--- Smoothed virtual axis in [-1, 1] (Unity GetAxis). Built-ins: "Horizontal", "Vertical", "Mouse X", "Mouse Y", "Look Horizontal", "Look Vertical".
---@param name string
---@return number
function Input:getAxis(name) end

--- Instant virtual axis in [-1, 1] (Unity GetAxisRaw). Prefer for movement.
---@param name string
---@return number
function Input:getAxisRaw(name) end

--- True if a mapped gamepad is connected (optional GLFW joystick id).
---@param jid? integer
---@return boolean
function Input:isGamepadConnected(jid) end

--- Human-readable gamepad name, or empty string.
---@param jid? integer
---@return string
function Input:getGamepadName(jid) end

--- Gamepad axis after deadzone (GAMEPAD_AXIS_*).
---@param axis integer
---@param jid? integer
---@return number
function Input:getGamepadAxis(axis, jid) end

---@param button integer
---@param jid? integer
---@return boolean
function Input:isGamepadButtonPressed(button, jid) end

---@param button integer
---@param jid? integer
---@return boolean
function Input:isGamepadButtonPressedThisFrame(button, jid) end

---@param button integer
---@param jid? integer
---@return boolean
function Input:isGamepadButtonReleasedThisFrame(button, jid) end

---@param deadzone number
function Input:setGamepadDeadzone(deadzone) end

---@return number
function Input:getGamepadDeadzone() end

---@return Input
function getInput() end

-- Key / mouse / cursor constants (GLFW values)
KEY_SPACE = 32
KEY_APOSTROPHE = 39
KEY_COMMA = 44
KEY_MINUS = 45
KEY_PERIOD = 46
KEY_SLASH = 47
KEY_0 = 48
KEY_1 = 49
KEY_2 = 50
KEY_3 = 51
KEY_4 = 52
KEY_5 = 53
KEY_6 = 54
KEY_7 = 55
KEY_8 = 56
KEY_9 = 57
KEY_SEMICOLON = 59
KEY_EQUAL = 61
KEY_A = 65
KEY_B = 66
KEY_C = 67
KEY_D = 68
KEY_E = 69
KEY_F = 70
KEY_G = 71
KEY_H = 72
KEY_I = 73
KEY_J = 74
KEY_K = 75
KEY_L = 76
KEY_M = 77
KEY_N = 78
KEY_O = 79
KEY_P = 80
KEY_Q = 81
KEY_R = 82
KEY_S = 83
KEY_T = 84
KEY_U = 85
KEY_V = 86
KEY_W = 87
KEY_X = 88
KEY_Y = 89
KEY_Z = 90
KEY_LEFT_BRACKET = 91
KEY_BACKSLASH = 92
KEY_RIGHT_BRACKET = 93
KEY_GRAVE_ACCENT = 96
KEY_ESCAPE = 256
KEY_ENTER = 257
KEY_TAB = 258
KEY_BACKSPACE = 259
KEY_INSERT = 260
KEY_DELETE = 261
KEY_RIGHT = 262
KEY_LEFT = 263
KEY_DOWN = 264
KEY_UP = 265
KEY_PAGE_UP = 266
KEY_PAGE_DOWN = 267
KEY_HOME = 268
KEY_END = 269
KEY_CAPS_LOCK = 280
KEY_SCROLL_LOCK = 281
KEY_NUM_LOCK = 282
KEY_PRINT_SCREEN = 283
KEY_PAUSE = 284
KEY_F1 = 290
KEY_F2 = 291
KEY_F3 = 292
KEY_F4 = 293
KEY_F5 = 294
KEY_F6 = 295
KEY_F7 = 296
KEY_F8 = 297
KEY_F9 = 298
KEY_F10 = 299
KEY_F11 = 300
KEY_F12 = 301
KEY_KP_0 = 320
KEY_KP_1 = 321
KEY_KP_2 = 322
KEY_KP_3 = 323
KEY_KP_4 = 324
KEY_KP_5 = 325
KEY_KP_6 = 326
KEY_KP_7 = 327
KEY_KP_8 = 328
KEY_KP_9 = 329
KEY_KP_DECIMAL = 330
KEY_KP_DIVIDE = 331
KEY_KP_MULTIPLY = 332
KEY_KP_SUBTRACT = 333
KEY_KP_ADD = 334
KEY_KP_ENTER = 335
KEY_KP_EQUAL = 336
KEY_LEFT_SHIFT = 340
KEY_LEFT_CONTROL = 341
KEY_LEFT_ALT = 342
KEY_LEFT_SUPER = 343
KEY_RIGHT_SHIFT = 344
KEY_RIGHT_CONTROL = 345
KEY_RIGHT_ALT = 346
KEY_RIGHT_SUPER = 347
KEY_MENU = 348

MOUSE_LEFT = 0
MOUSE_RIGHT = 1

CURSOR_NORMAL = 212993
CURSOR_HIDDEN = 212994
CURSOR_DISABLED = 212995

-- Gamepad buttons (Xbox layout via GLFW)
GAMEPAD_A = 0
GAMEPAD_B = 1
GAMEPAD_X = 2
GAMEPAD_Y = 3
GAMEPAD_LEFT_BUMPER = 4
GAMEPAD_RIGHT_BUMPER = 5
GAMEPAD_BACK = 6
GAMEPAD_START = 7
GAMEPAD_GUIDE = 8
GAMEPAD_LEFT_THUMB = 9
GAMEPAD_RIGHT_THUMB = 10
GAMEPAD_DPAD_UP = 11
GAMEPAD_DPAD_RIGHT = 12
GAMEPAD_DPAD_DOWN = 13
GAMEPAD_DPAD_LEFT = 14
-- PlayStation aliases
GAMEPAD_CROSS = 0
GAMEPAD_CIRCLE = 1
GAMEPAD_SQUARE = 2
GAMEPAD_TRIANGLE = 3

-- Gamepad axes
GAMEPAD_AXIS_LEFT_X = 0
GAMEPAD_AXIS_LEFT_Y = 1
GAMEPAD_AXIS_RIGHT_X = 2
GAMEPAD_AXIS_RIGHT_Y = 3
GAMEPAD_AXIS_LEFT_TRIGGER = 4
GAMEPAD_AXIS_RIGHT_TRIGGER = 5

--------------------------------------------------------------------------------
-- Window / UI / animation / particles
--------------------------------------------------------------------------------

---@class Window
---@field targetWidth number
---@field targetHeight number
---@field targetX number
---@field targetY number
local Window = {}

---@return integer
function Window:getWidth() end

---@return integer
function Window:getHeight() end

---@return number
function Window:getAspectRatio() end

---@return number
function Window:getTargetAspectRatio() end

function Window:updateViewportSize() end

---@return Window
function getWindow() end

---@class UI
local UI = {}

---@return Entity
function UI:getSelectedEntity() end

---@return UI
function getUI() end

---@class AnimationManager
---@field drawSkeleton boolean
---@field drawMesh boolean
local AnimationManager = {}

---@return boolean
function AnimationManager:getDrawSkeleton() end

---@param v boolean
function AnimationManager:setDrawSkeleton(v) end

---@return boolean
function AnimationManager:getDrawMesh() end

---@param v boolean
function AnimationManager:setDrawMesh(v) end

---@return AnimationManager
function getAnimationManager() end

---@param path string
---@return AnimationHandle
function loadAnimation(path) end

---@param path string
---@return SkeletonReference
function loadSkeleton(path) end

---@class SkeletonReference
local SkeletonReference = {}

---@return string
function SkeletonReference:getGuid() end

---@return boolean
function SkeletonReference:isValid() end

function SkeletonReference:clear() end

--- Empty handle, or from GUID string.
---@param guid? string
---@return SkeletonReference
function skeleton(guid) end

---@class ParticleManager
local ParticleManager = {}

---@param entity Entity
function ParticleManager:playEffect(entity) end

---@return ParticleManager
function getParticleManager() end

--------------------------------------------------------------------------------
-- Components
--------------------------------------------------------------------------------

---@class Transform
---@field position vec3
---@field rotation quat
---@field scale vec3
local Transform = {}

---@param rotation quat
function Transform:setRotation(rotation) end

---@return vec3
function Transform:GetEulerAngles() end

---@class ModelRenderer
---@field model ModelHandle
local ModelRenderer = {}

---@param path string
function ModelRenderer:setModel(path) end

---@param handle MaterialHandle
function ModelRenderer:setMaterial(handle) end

---@class RigidBodyComponent
local RigidBodyComponent = {}

---@return vec3
function RigidBodyComponent:getPosition() end

---@param pos vec3
function RigidBodyComponent:setPosition(pos) end

---@param rot quat
function RigidBodyComponent:setRotation(rot) end

---@param euler vec3
function RigidBodyComponent:setRotationEuler(euler) end

---@return vec3
function RigidBodyComponent:getLinearVelocity() end

---@param vel vec3
function RigidBodyComponent:setLinearVelocity(vel) end

---@param vel vec3
function RigidBodyComponent:addLinearVelocity(vel) end

---@return vec3
function RigidBodyComponent:getAngularVelocity() end

---@param vel vec3
function RigidBodyComponent:setAngularVelocity(vel) end

---@param force vec3
function RigidBodyComponent:applyForce(force) end

---@param impulse vec3
function RigidBodyComponent:applyImpulse(impulse) end

---@param torque vec3
function RigidBodyComponent:applyTorque(torque) end

---@param impulse vec3
function RigidBodyComponent:applyAngularImpulse(impulse) end

---@param factor number
function RigidBodyComponent:setGravityFactor(factor) end

---@return number
function RigidBodyComponent:getGravityFactor() end

function RigidBodyComponent:activate() end
function RigidBodyComponent:deactivate() end

---@return boolean
function RigidBodyComponent:isActive() end

---@param kinematic boolean
function RigidBodyComponent:setKinematic(kinematic) end

---@return boolean
function RigidBodyComponent:isKinematic() end

---@param pos vec3
---@param rot quat
---@param dt number
function RigidBodyComponent:moveKinematic(pos, rot, dt) end

---@param shape BoxShape
function RigidBodyComponent:setBoxShape(shape) end

---@param shape SphereShape
function RigidBodyComponent:setSphereShape(shape) end

---@param shape CapsuleShape
function RigidBodyComponent:setCapsuleShape(shape) end

---@param shape CylinderShape
function RigidBodyComponent:setCylinderShape(shape) end

function RigidBodyComponent:setMeshShape(...) end
function RigidBodyComponent:setConvexMeshShape(...) end
function RigidBodyComponent:setCollisionShape(...) end
function RigidBodyComponent:setCollisionShapeRef(...) end

---@class PlayerControllerComponent
---@field setClimbing fun(self: PlayerControllerComponent, climbing: boolean)
---@field isClimbing fun(self: PlayerControllerComponent): boolean
---@field getCapsuleRadius fun(self: PlayerControllerComponent): number
---@field getCapsuleHalfHeight fun(self: PlayerControllerComponent): number
---@field getClimbNormal fun(self: PlayerControllerComponent): vec3
---@field getClimbPoint fun(self: PlayerControllerComponent): vec3
---@field hasClimbSurface fun(self: PlayerControllerComponent): boolean
---@field probeClimbSurface fun(self: PlayerControllerComponent, dir: vec3, maxDist: number, minNormalY?: number, maxNormalY?: number): boolean
local PlayerControllerComponent = {}

---@return boolean
function PlayerControllerComponent:isOnGround() end

---@return vec3
function PlayerControllerComponent:getPosition() end

---@param pos vec3
function PlayerControllerComponent:setPosition(pos) end

---@return vec3
function PlayerControllerComponent:getLinearVelocity() end

---@param vel vec3
function PlayerControllerComponent:setLinearVelocity(vel) end

---@return vec3
function PlayerControllerComponent:getGroundVelocity() end

---@param rot quat
function PlayerControllerComponent:setRotation(rot) end

---@param euler vec3
function PlayerControllerComponent:setRotationEuler(euler) end

--- Face so local +Z points along worldDir on the XZ plane.
---@param worldDir vec3
function PlayerControllerComponent:setFacingDirection(worldDir) end

--- BOTW-style climb: zero gravity / no stick-to-floor while true.
---@param climbing boolean
function PlayerControllerComponent:setClimbing(climbing) end

---@return boolean
function PlayerControllerComponent:isClimbing() end

---@return number
function PlayerControllerComponent:getCapsuleRadius() end

---@return number
function PlayerControllerComponent:getCapsuleHalfHeight() end

---@return vec3
function PlayerControllerComponent:getClimbNormal() end

---@return vec3
function PlayerControllerComponent:getClimbPoint() end

---@return boolean
function PlayerControllerComponent:hasClimbSurface() end

--- Raycast for a steep wall along dir. Optional normal.y band (default mild overhang..not floor).
---@param dir vec3
---@param maxDist number
---@param minNormalY? number
---@param maxNormalY? number
---@return boolean
function PlayerControllerComponent:probeClimbSurface(dir, maxDist, minNormalY, maxNormalY) end

---@class AnimationComponent
---@field skeleton SkeletonReference
---@field skeletonRef SkeletonReference
---@field defaultFadeDuration number
---@field playbackSpeed number
---@field speed number
---@field looping boolean
--- When true, hips/root model translation is forced to skeleton rest (cancels Mixamo float).
---@field counteractRootOffset boolean
local AnimationComponent = {}

---@param handle_or_path SkeletonReference|string
function AnimationComponent:setSkeleton(handle_or_path) end

---@return boolean
function AnimationComponent:hasSkeleton() end

---@param handle AnimationHandle
---@param loop? boolean
---@param fade? number
function AnimationComponent:play(handle, loop, fade) end

---@param handle AnimationHandle
---@param fade? number
function AnimationComponent:crossfadeTo(handle, fade) end

function AnimationComponent:stop() end

---@param time number
function AnimationComponent:seek(time) end

---@return boolean
function AnimationComponent:isPlaying() end

---@return boolean
function AnimationComponent:isFading() end

---@return number
function AnimationComponent:getTime() end

---@return number
function AnimationComponent:getLength() end

---@return number
function AnimationComponent:getBlendWeight() end

---@return AnimationHandle
function AnimationComponent:getCurrentClip() end

---@return boolean
function AnimationComponent:hasSkeleton() end

---@return integer
function AnimationComponent:jointCount() end

---@class AudioSource
---@field autoPlay boolean
---@field looping boolean
---@field volume number
---@field pitch number
---@field isPlaying boolean
---@field referenceDistance number
---@field maxDistance number
---@field rolloffFactor number
local AudioSource = {}

function AudioSource:play() end
function AudioSource:stop() end

---@param handle SoundHandle
function AudioSource:setSound(handle) end

---@class LuaScript
---@field scriptPath string
local LuaScript = {}

---@param entity Entity
---@param path string
function LuaScript:setScript(entity, path) end

---@param name string
---@param value any
function LuaScript:setVariable(name, value) end

---@param name string
---@return any
function LuaScript:getVariable(name) end

---@class RmlUIComponent
---@field documentPath string
---@field visible boolean
local RmlUIComponent = {}

function RmlUIComponent:LoadDocument() end
function RmlUIComponent:UnloadDocument() end

---@param visible boolean
function RmlUIComponent:SetVisible(visible) end

---@return boolean
function RmlUIComponent:IsVisible() end

---@class ParticleSystem
---@class ShadowCaster
---@class SkinnedMeshComponent
---@class TerrainRenderer
---@class GizmoComponent
---@class EntityMetadata

--------------------------------------------------------------------------------
-- Entity
--------------------------------------------------------------------------------

---@class Entity
local Entity = {}

---@return boolean
function Entity:isValid() end

---@return string
function Entity:getName() end

---@param name string
function Entity:setName(name) end

---@return string
function Entity:getTag() end

---@param handle EntityHandle
function Entity:setParent(handle) end

---@return EntityHandle
function Entity:getHandle() end

---@param handle PrefabHandle
---@return Entity
function Entity:instantiatePrefab(handle) end

---@return EntityHandle[]
function Entity:getChildren() end

function Entity:destroy() end

---@return Transform
function Entity:AddTransform() end
---@return Transform
function Entity:GetTransform() end
---@return boolean
function Entity:HasTransform() end
function Entity:RemoveTransform() end

---@return ModelRenderer
function Entity:AddModelRenderer() end
---@return ModelRenderer
function Entity:GetModelRenderer() end
---@return boolean
function Entity:HasModelRenderer() end
function Entity:RemoveModelRenderer() end

---@return RigidBodyComponent
function Entity:AddRigidBodyComponent() end
---@return RigidBodyComponent
function Entity:GetRigidBodyComponent() end
---@return boolean
function Entity:HasRigidBodyComponent() end
function Entity:RemoveRigidBodyComponent() end

---@return PlayerControllerComponent
function Entity:AddPlayerControllerComponent() end
---@return PlayerControllerComponent
function Entity:GetPlayerControllerComponent() end
---@return boolean
function Entity:HasPlayerControllerComponent() end
function Entity:RemovePlayerControllerComponent() end

---@return AnimationComponent
function Entity:AddAnimationComponent() end
---@return AnimationComponent
function Entity:GetAnimationComponent() end
---@return boolean
function Entity:HasAnimationComponent() end
function Entity:RemoveAnimationComponent() end

---@return AudioSource
function Entity:AddAudioSource() end
---@return AudioSource
function Entity:GetAudioSource() end
---@return boolean
function Entity:HasAudioSource() end
function Entity:RemoveAudioSource() end

---@return LuaScript
function Entity:AddLuaScript() end
---@return LuaScript
function Entity:GetLuaScript() end
---@return boolean
function Entity:HasLuaScript() end
function Entity:RemoveLuaScript() end

---@return ShadowCaster
function Entity:AddShadowCaster() end
---@return ShadowCaster
function Entity:GetShadowCaster() end
---@return boolean
function Entity:HasShadowCaster() end
function Entity:RemoveShadowCaster() end

---@return SkinnedMeshComponent
function Entity:AddSkinnedMeshComponent() end
---@return SkinnedMeshComponent
function Entity:GetSkinnedMeshComponent() end
---@return boolean
function Entity:HasSkinnedMeshComponent() end
function Entity:RemoveSkinnedMeshComponent() end

---@return ParticleSystem
function Entity:AddParticleSystem() end
---@return ParticleSystem
function Entity:GetParticleSystem() end
---@return boolean
function Entity:HasParticleSystem() end
function Entity:RemoveParticleSystem() end

---@return TerrainRenderer
function Entity:AddTerrainRenderer() end
---@return TerrainRenderer
function Entity:GetTerrainRenderer() end
---@return boolean
function Entity:HasTerrainRenderer() end
function Entity:RemoveTerrainRenderer() end

---@return RmlUIComponent
function Entity:AddRmlUIComponent() end
---@return RmlUIComponent
function Entity:GetRmlUIComponent() end
---@return boolean
function Entity:HasRmlUIComponent() end
function Entity:RemoveRmlUIComponent() end

---@return GizmoComponent
function Entity:AddGizmoComponent() end
---@return GizmoComponent
function Entity:GetGizmoComponent() end
---@return boolean
function Entity:HasGizmoComponent() end
function Entity:RemoveGizmoComponent() end

---@return EntityMetadata
function Entity:AddEntityMetadata() end
---@return EntityMetadata
function Entity:GetEntityMetadata() end
---@return boolean
function Entity:HasEntityMetadata() end
function Entity:RemoveEntityMetadata() end

---@param name string
---@return Entity
function createEntity(name) end

---@return Entity|nil
function getPlayerEntity() end

--- Resolve handle to entity (available in entity script environments).
---@param handle EntityHandle
---@return Entity
function getEntityFromHandle(handle) end

--------------------------------------------------------------------------------
-- Globals: logging & events
--------------------------------------------------------------------------------

function print(...) end
function debug(...) end
function info(...) end

---@param eventName string
---@param callback fun(...: any)
---@return any subscription id
function subscribe(eventName, callback) end

---@param eventName string
---@param data? number|boolean|string|vec3|Entity
function publish(eventName, data) end

--------------------------------------------------------------------------------
-- Script environment (per-entity)
--------------------------------------------------------------------------------

--- Entity this script is attached to.
---@type Entity
gameObject = nil

--- Frame delta time in seconds (set before Update / LateUpdate).
---@type number
deltaTime = 0

--- Optional inspector-serialized variables table.
---@type table
variables = {}

--------------------------------------------------------------------------------
-- ImGui (editor)
--------------------------------------------------------------------------------

---@class imgui
imgui = {}

---@param name string
---@param flags? integer
---@return boolean
function imgui.Begin(name, flags) end

function imgui.End() end

---@param text string
function imgui.Text(text) end

---@param label string
---@return boolean
function imgui.Button(label) end

function imgui.SameLine() end
function imgui.Separator() end

---@param label string
---@param value { v: boolean }
---@return boolean
function imgui.Checkbox(label, value) end

---@param label string
---@param value { v: number }
---@param min number
---@param max number
---@return boolean
function imgui.SliderFloat(label, value, min, max) end

---@param label string
---@param value { v: integer }
---@param min integer
---@param max integer
---@return boolean
function imgui.SliderInt(label, value, min, max) end

---@class imgui.WindowFlags
---@field None integer
---@field NoTitleBar integer
---@field NoResize integer
---@field NoMove integer
---@field NoCollapse integer
imgui.WindowFlags = {}
