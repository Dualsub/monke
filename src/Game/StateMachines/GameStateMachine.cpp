#include "Game/StateMachines/GameStateMachine.h"

#include "Core/Core.h"
#include "Application.h"
#include "UI/UIHelper.h"
#include "Game/Components.h"
#include "Game/Helpers/ParticleHelper.h"
#include "Game/Helpers/PerlinNoiseHelper.h"
#include "Game/Helpers/PhysicsRenderingHelper.h"

#include <glm/glm.hpp>

#include <ranges>

// Undefine windows.h macro for CreateEvent
#ifdef CreateEvent
#undef CreateEvent
#endif

#ifdef DEBUG
#define assert_transform_valid(transform)      \
    assert(!glm::isnan(transform.position.x)); \
    assert(!glm::isnan(transform.position.y)); \
    assert(!glm::isnan(transform.position.z)); \
    assert(!glm::isnan(transform.rotation.x)); \
    assert(!glm::isnan(transform.rotation.y)); \
    assert(!glm::isnan(transform.rotation.z)); \
    assert(!glm::isnan(transform.rotation.w)); \
    assert(!glm::isnan(transform.scale.x));    \
    assert(!glm::isnan(transform.scale.y));    \
    assert(!glm::isnan(transform.scale.z));
#else
// Print out a warning if the transform is invalid using std::cout
#define assert_transform_valid(transform)                                                                                                                                                                                                                                                                                                                            \
    if (glm::isnan(transform.position.x) || glm::isnan(transform.position.y) || glm::isnan(transform.position.z) || glm::isnan(transform.rotation.x) || glm::isnan(transform.rotation.y) || glm::isnan(transform.rotation.z) || glm::isnan(transform.rotation.w) || glm::isnan(transform.scale.x) || glm::isnan(transform.scale.y) || glm::isnan(transform.scale.z)) \
    {                                                                                                                                                                                                                                                                                                                                                                \
        std::cout << "Warning: #transform is invalid" << std::endl;                                                                                                                                                                                                                                                                                                  \
    }
#endif

namespace mk
{
    template <typename... Components>
    struct Entity
    {
        std::tuple<Components...> components;

        Entity(Components &&...components)
            : components(std::forward<Components>(components)...)
        {
        }
        Entity() = default;
        ~Entity() = default;

        template <typename Component>
        Component &GetComponent()
        {
            return std::get<Component>(components);
        }
    };

    template <typename... Components>
    using EntityList = std::vector<Entity<Components...>>;

    template <typename... Components>
    Entity<Components...> CreateEntity(Components &&...components)
    {
        return Entity<Components...>(std::forward<Components>(components)...);
    }

    template <typename... Components, typename... Archetypes>
    void ForEach(std::function<void(Components &...)> func, Archetypes &...archetypes)
    {
        // For each archetype, check if it is a collection of entities, or a single entity
        // If it is a collection, iterate over each entity and call the function with the entity
        // If it is a single entity, call the function with the components of the entity that are requested

        // For each archetype, call the function with the entity
        ([&](auto &&archetype)
         {
             if constexpr (std::ranges::range<decltype(archetype)>)
             {
                for (auto&& entity : archetype)
                {
                    func(std::get<Components>(entity.components)...);
                }
             }
             else
             {
                func(std::get<Components>(archetype.components)...);
             } }(archetypes), ...);
    }

    // For each, but with a return bool value to indicate if the entity should be removed or not
    template <typename... Components, typename... Archetypes>
    void Filter(std::function<bool(Components &...)> func, Archetypes &...archetypes)
    {
        ([&](auto &&archetype)
         {
             static_assert(std::ranges::range<decltype(archetype)>);
                // Using iterator to be able to delete entities 
             for (auto it = std::begin(archetype); it != std::end(archetype);)
                {
                    if (func(std::get<Components>(it->components)...))
                    {
                        it = archetype.erase(it);
                    }
                    else
                    {
                        ++it;
                    }
                } }(archetypes), ...);
    }

    void UpdateAnimation(Animation &animation, float dt)
    {
        animation.time += dt;
        if (animation.time > animation.duration)
        {
            animation.time = animation.loop ? animation.time - animation.duration : animation.duration;
        }
    }

    Transform GetAnimationTransform(Animation &animation)
    {
        // Return identity matrix if there are not enough keyframes to interpolate
        if (animation.keyframes.size() < 2)
            return Transform{};

        int32_t keyFrame1Index = -1;
        int32_t keyFrame2Index = -1;
        float interpolationFactor = 0.0f;

        // Handle looping
        float animTime = animation.time;
        if (animation.loop)
        {
            animTime = fmod(animation.time, animation.duration);
        }

        // Find the current keyframes
        for (size_t i = 0; i < animation.keyframes.size() - 1; ++i)
        {
            const auto &currentKeyFrame = animation.keyframes[i];
            const auto &nextKeyFrame = animation.keyframes[i + 1];

            if (animTime >= currentKeyFrame.time && animTime <= nextKeyFrame.time)
            {
                keyFrame1Index = i;
                keyFrame2Index = i + 1;
                float timeDelta = nextKeyFrame.time - currentKeyFrame.time;
                interpolationFactor = (animTime - currentKeyFrame.time) / timeDelta;
                break;
            }
        }

        // If no valid keyframe was found and not looping, return the last keyframe
        if (keyFrame1Index == -1)
        {
            if (!animation.loop)
            {
                // Set to last keyframe if non-looping and past the last keyframe
                keyFrame1Index = animation.keyframes.size() - 2;
                keyFrame2Index = animation.keyframes.size() - 1;
                interpolationFactor = 1.0f; // Fully at the last keyframe
            }
            else
            {
                // Handle wrapping for looping case if no valid keyframes found
                keyFrame1Index = animation.keyframes.size() - 1;
                keyFrame2Index = 0;
                interpolationFactor = animTime / animation.duration;
            }
        }

        interpolationFactor = glm::clamp(interpolationFactor, 0.0f, 1.0f);

        const auto &keyFrame1 = animation.keyframes[keyFrame1Index];
        const auto &keyFrame2 = animation.keyframes[keyFrame2Index];

        // Interpolate position and rotation between the two keyframes
        glm::vec3 position = glm::mix(keyFrame1.position, keyFrame2.position, interpolationFactor);
        glm::quat rotation = glm::slerp(keyFrame1.rotation, keyFrame2.rotation, interpolationFactor);

        // Return the transformation matrix combining translation and rotation
        return Transform{position, rotation};
    }

    struct DebugCamera
    {
        glm::vec3 position = glm::vec3(0.0f);
        glm::quat rotation = glm::identity<glm::quat>();
        float yaw = 0.0f;
        float pitch = 0.0f;
        float fov = 45.0f;
        bool active = false;
    } g_debugCamera = {};

    const RenderHandle c_fontMaterialHandle = GetHandle("FontMaterial");
    const RenderHandle c_fontAtlasHandle = GetHandle(MK_ASSET_PATH("ui/font_msdf.dat"));

    GameStateMachine::OptionalState GameStateImpl::TransitionAnyTo(const GameStateMachine::State &state)
    {
        return std::nullopt;
    }

#pragma region InitialLoadState

    void GameStateImpl::OnEnter(GameStates::InitialLoadState &state)
    {
        state.future = std::async(
            std::launch::async,
            []()
            {
                Application::ReadPersistentData();

                auto &audioSystem = Application::GetAudioSystem();
                audioSystem.LoadBank(MK_ASSET_PATH("/audio/monke/Build/Desktop/Master.bank"), BankType::Master);
                audioSystem.LoadBank(MK_ASSET_PATH("/audio/monke/Build/Desktop/Master.strings.bank"), BankType::Strings);

                auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());

                // Load assets
                renderer.CreateMaterial<SpriteMaterial>(
                    "WhiteSpriteMaterial",
                    {
                        .texture = GetHandle("white_sprite"),
                    });

                renderer.CreateMaterial<SpriteMaterial>(
                    "BloodHudMaterial",
                    {
                        .texture = renderer.LoadImage(MK_ASSET_PATH("ui/blood_hud.dat")),
                    });

                auto normal = renderer.LoadImage(MK_ASSET_PATH("textures/normal.dat"), ImageType::Texture2DArray);

                renderer.CreateMaterial<PBRMaterial>(
                    "WhiteMaterial",
                    {
                        .albedo = GetHandle("white"),
                        .normal = normal,
                        .metallicRoughnessAO = GetHandle("white"),
                        .metallicMin = 0.0f,
                        .metallicMax = 0.0f,
                        .roughnessMin = 1.0f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.0f,
                        .aoMax = 1.0f,
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "TestMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("textures/grid.dat"), ImageType::Texture2DArray),
                        .normal = normal,
                        .metallicRoughnessAO = GetHandle("white"),
                        .metallicMin = 0.0f,
                        .metallicMax = 0.0f,
                        .roughnessMin = 1.0f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.0f,
                        .aoMax = 1.0f,
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "WeaponMaterial",
                    {
                        .albedo = GetHandle("white"),
                        .albedoColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("textures/normal.dat"), ImageType::Texture2DArray),
                        .metallicRoughnessAO = GetHandle("white"),
                        .metallicMin = 0.0f,
                        .metallicMax = 0.0f,
                        .roughnessMin = 1.0f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.0f,
                        .aoMax = 1.0f,
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "FloorMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_baseColor.dat"), ImageType::Texture2DArray, true),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_normal.dat"), ImageType::Texture2DArray, true),
                        .metallicRoughnessAO = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_occlusionRoughnessMetallic.dat"), ImageType::Texture2DArray, true),
                        .emissive = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_emissive.dat"), ImageType::Texture2DArray, true),
                        .emissiveColor = glm::vec4(glm::vec3(0.0f), 0.0f),
                        .metallicMin = 0.0f,
                        .metallicMax = 1.0f,
                        .roughnessMin = 0.15f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.75f,
                        .aoMax = 1.0f,
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "FloorCorruptedMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_corrupted_baseColor.dat"), ImageType::Texture2DArray, true),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_corrupted_normal.dat"), ImageType::Texture2DArray, true),
                        .metallicRoughnessAO = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_corrupted_occlusionRoughnessMetallic.dat"), ImageType::Texture2DArray, true),
                        .emissive = renderer.LoadImage(MK_ASSET_PATH("models/floor/textures/floor_corrupted_emissive.dat"), ImageType::Texture2DArray, true),
                        .emissiveColor = glm::vec4(glm::vec3(5.0f), 1.0f),
                        .metallicMin = 0.0f,
                        .metallicMax = 1.0f,
                        .roughnessMin = 0.0f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.0f,
                        .aoMax = 1.0f,
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "DroneMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_baseColor.dat"), ImageType::Texture2DArray, true),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_normal.dat"), ImageType::Texture2DArray, true),
                        .metallicRoughnessAO = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_occlusionRoughnessMetallic.dat"), ImageType::Texture2DArray, true),
                        .emissive = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_emissive.dat"), ImageType::Texture2DArray, true),
                        .emissiveColor = glm::vec4(glm::vec3(20.0f, 0.0f, 0.0f), 1.0f),
                        .metallicMin = 0.0f,
                        .metallicMax = 1.0f,
                        .roughnessMin = 0.0f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.0f,
                        .aoMax = 1.0f,
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "FastMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_baseColor.dat"), ImageType::Texture2DArray, true),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_normal.dat"), ImageType::Texture2DArray, true),
                        .metallicRoughnessAO = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_occlusionRoughnessMetallic.dat"), ImageType::Texture2DArray, true),
                        .emissive = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_emissive.dat"), ImageType::Texture2DArray, true),
                        .emissiveColor = glm::vec4(glm::vec3(0.0f, 20.0f, 0.0f), 1.0f),
                        .metallicMin = 0.0f,
                        .metallicMax = 1.0f,
                        .roughnessMin = 0.0f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.0f,
                        .aoMax = 1.0f,
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "HeavyMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_baseColor.dat"), ImageType::Texture2DArray, true),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_normal.dat"), ImageType::Texture2DArray, true),
                        .metallicRoughnessAO = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_occlusionRoughnessMetallic.dat"), ImageType::Texture2DArray, true),
                        .emissive = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures_2/DefaultMaterial_emissive.dat"), ImageType::Texture2DArray, true),
                        .emissiveColor = glm::vec4(glm::vec3(0.0f, 0.0f, 20.0f), 1.0f),
                        .metallicMin = 0.0f,
                        .metallicMax = 1.0f,
                        .roughnessMin = 0.0f,
                        .roughnessMax = 1.0f,
                        .aoMin = 0.0f,
                        .aoMax = 1.0f,
                    });

                renderer.LoadMesh(MK_ASSET_PATH("models/sphere.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/plane.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/gun.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/launcher/launcher.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/floor/floor.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/drone/drone.dat"), true);
                renderer.LoadMesh(MK_ASSET_PATH("models/tree/tree.dat"), true);

                renderer.CreateMaterial<PBRMaterial>(
                    "ParticleAtlasMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("textures/particle_atlas_BC.dat"), ImageType::Texture2DArray),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("textures/particle_atlas_N.dat"), ImageType::Texture2DArray),
                        .metallicRoughnessAO = renderer.LoadImage(MK_ASSET_PATH("textures/particle_atlas_ARM.dat"), ImageType::Texture2DArray),
                    });

                renderer.CreateMaterial<SkyboxMaterial>(
                    "skybox",
                    {
                        .cubemap = renderer.LoadImage(MK_ASSET_PATH("skybox/skybox.dat"), ImageType::Cubemap),
                    });
                RenderHandle environmentMap = renderer.LoadEnvironmentMap(
                    "environment",
                    MK_ASSET_PATH("skybox/skybox_irradiance.dat"),
                    MK_ASSET_PATH("skybox/skybox_prefiltered.dat"),
                    {
                        .min = glm::vec4(-1.0f),
                        .max = glm::vec4(1.0f),
                        .numCells = glm::uvec4(1, 1, 1, 0),
                    },
                    {
                        glm::vec3(0.0f, 0.0f, 0.0f),
                    });
            });
    }

    void GameStateImpl::OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::InitialLoadState &state)
    {
    }

    void GameStateImpl::OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::InitialLoadState &state)
    {
    }

    void GameStateImpl::OnRender(Vultron::SceneRenderer &renderer, GameStates::InitialLoadState &state)
    {
        UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "Loading...", glm::vec2(0.0f, 0.0f), 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    void GameStateImpl::OnExit(GameStates::InitialLoadState &state)
    {
        state.future.get();
    }

    GameStateMachine::OptionalState GameStateImpl::TransitionTo(const GameStates::InitialLoadState &state)
    {
        if (state.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            return GameStates::MainMenuState{};
        }

        return std::nullopt;
    }

#pragma endregion

#pragma region MainMenuState

    void GameStateImpl::OnEnter(GameStates::MainMenuState &state)
    {
    }

    void GameStateImpl::OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::MainMenuState &state)
    {
        if (inputState.Pressed(InputActionType::Jump))
        {
            state.shouldEnterGame = true;
        }

        if (inputState.Pressed(InputActionType::Escape))
        {
            Application::Quit();
        }
    }

    void GameStateImpl::OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::MainMenuState &state)
    {
    }

    void GameStateImpl::OnRender(Vultron::SceneRenderer &renderer, GameStates::MainMenuState &state)
    {
        float time = Application::GetTimeSinceStart();
        float blink = glm::mix(glm::sin(time * 3.0f) * 0.5f + 0.5f, 1.0f, 0.25f);
        UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "The Last Garden.", glm::vec2(-0.65f, -0.125f), 4.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), TextAlignment::Left);
        UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "Press spacebar to start.", glm::vec2(-0.65f, 0.0f), 1.0f, glm::vec4(glm::vec3(1.0f) * blink, 1.0f), TextAlignment::Left);

        // Put A text with your highscore here
        float highScore = Application::GetPersistentData().highScore;
        if (highScore > 0.0f)
            UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "High Score: " + std::to_string(static_cast<int>(highScore)), glm::vec2(-0.65f, 0.125f), 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 0.8f), TextAlignment::Left);
    }

    void GameStateImpl::OnExit(GameStates::MainMenuState &state)
    {
    }

    GameStateMachine::OptionalState GameStateImpl::TransitionTo(const GameStates::MainMenuState &state)
    {
        if (state.shouldEnterGame)
        {
            return GameStates::LoadingState{};
        }

        return std::nullopt;
    }

#pragma endregion

#pragma region LoadingState

    void GameStateImpl::OnEnter(GameStates::LoadingState &state)
    {
        state.loadingFuture = std::async(
            std::launch::async,
            []()
            {
                // Load game
            });
    }

    void GameStateImpl::OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::LoadingState &state)
    {
    }

    void GameStateImpl::OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::LoadingState &state)
    {
    }

    void GameStateImpl::OnRender(Vultron::SceneRenderer &renderer, GameStates::LoadingState &state)
    {
        UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "Loading scene...", glm::vec2(0.0f, 0.0f), 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    }

    void GameStateImpl::OnExit(GameStates::LoadingState &state)
    {
        state.loadingFuture.get();
    }

    GameStateMachine::OptionalState GameStateImpl::TransitionTo(const GameStates::LoadingState &state)
    {
        if (state.loadingFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            return GameStates::PlayingState{};
        }

        return std::nullopt;
    }

#pragma endregion

#pragma region PlayingState

    constexpr int32_t c_tilesPerRow = 20;
    constexpr float c_tileSize = 400.0f;
    constexpr float c_tileScale = 0.5f;

    constexpr glm::vec3 c_corruptionBeginColor = glm::vec3(0.0f, 0.0f, 0.0f);
    constexpr glm::vec3 c_corruptionEndColor = glm::vec3(1.0f, 0.0f, 0.0f);

    using WeaponEntity = Entity<Transform, Renderable, WeaponFireAction, ProjectileBulletEmitter>;

    struct
    {
        Entity<Transform, PhysicsProxy, PlayerMovement, PlayerAnimations, Health, Inventory> playerEntity;
        std::array<WeaponEntity, 2> weaponEntities;

        Entity<Transform, CameraSocket, CameraShakes> cameraEntity;
        EntityList<Transform, PhysicsProxy, Renderable, Lifetime> staticEntities;
        EntityList<ProjectileType, Transform, PhysicsProxy, Renderable, Lifetime> projectiles;
        EntityList<EnemyType, Transform, PhysicsProxy, Renderable, EnemyAI, SoundEmitter, Health> enemies;

        std::unordered_map<BodyID, float> damageEvents;

        std::array<Tile, c_tilesPerRow * c_tilesPerRow> tiles;
        BodyID floorBodyID = 0;
        std::vector<int32_t> nonCorruptedTiles;

        DynamicTimer waveTimer = DynamicTimer(false);
        uint32_t wave = 0;

        EventHandle ambienceEvent = 0;
        DynamicTimer enemySoundTimer = DynamicTimer(false);

        std::vector<ParticleEmitJob> particleJobs;

        float startTime = 0.0f;

        bool isGameOver = false;
    } g_entityStore;

    WeaponEntity &GetCurrentPlayerWeapon()
    {
        return g_entityStore.weaponEntities[g_entityStore.playerEntity.GetComponent<Inventory>().currentWeaponIndex];
    }

    int32_t GetTileIndex(const glm::vec3 &position)
    {
        constexpr float gridHalfSize = c_tileSize * c_tileScale * c_tilesPerRow * 0.5f;
        // Return -1 if outside of the grid
        if (position.x < -gridHalfSize || position.x > gridHalfSize || position.z < -gridHalfSize || position.z > gridHalfSize)
            return -1;

        // Calculate the grid position
        int32_t x = static_cast<int32_t>((position.x + gridHalfSize) / (c_tileSize * c_tileScale));
        int32_t z = static_cast<int32_t>((position.z + gridHalfSize) / (c_tileSize * c_tileScale));
        return z * c_tilesPerRow + x;
    }

    glm::vec3 GetTilePosition(int32_t index)
    {
        constexpr float gridHalfSize = c_tileSize * c_tileScale * c_tilesPerRow * 0.5f;
        constexpr float tileHalfSize = c_tileSize * c_tileScale * 0.5f;
        int32_t x = index % c_tilesPerRow;
        int32_t z = index / c_tilesPerRow;
        return glm::vec3(x * c_tileSize * c_tileScale + tileHalfSize - gridHalfSize, 0.0f, z * c_tileSize * c_tileScale + tileHalfSize - gridHalfSize);
    }

    std::vector<int32_t> GetTilesInRadius(const glm::vec3 &center, float radius)
    {
        int32_t centerIndex = GetTileIndex(center);

        if (centerIndex < 0)
            return {};

        int32_t centerX = centerIndex % c_tilesPerRow;
        int32_t centerZ = centerIndex / c_tilesPerRow;

        float tileWorldSize = c_tileSize * c_tileScale;

        float tileRadius = radius / tileWorldSize;

        int32_t tileRange = static_cast<int32_t>(std::ceil(tileRadius));

        std::vector<int32_t> result;
        result.reserve((2 * tileRange + 1) * (2 * tileRange + 1)); // optional optimization

        for (int32_t dz = -tileRange; dz <= tileRange; ++dz)
        {
            for (int32_t dx = -tileRange; dx <= tileRange; ++dx)
            {
                int32_t tileX = centerX + dx;
                int32_t tileZ = centerZ + dz;

                if (tileX < 0 || tileX >= c_tilesPerRow || tileZ < 0 || tileZ >= c_tilesPerRow)
                    continue;

                int32_t candidateIndex = tileZ * c_tilesPerRow + tileX;

                glm::vec3 tilePos = GetTilePosition(candidateIndex);

                float distXZ = glm::distance(
                    glm::vec2(center.x, center.z),
                    glm::vec2(tilePos.x, tilePos.z));

                if (distXZ <= radius)
                {
                    result.push_back(candidateIndex);
                }
            }
        }

        return result;
    }

    void CreateEnemy(EnemyType type, glm::vec3 position)
    {
        auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
        auto &physicsWorld = Application::GetPhysicsWorld();

        auto meshHandle = GetHandle(MK_ASSET_PATH("models/drone/drone.dat"));

        static const EnumArray<EnemyType, glm::mat4> c_enemyTransform = {
            glm::scale(glm::mat4(1.0f), glm::vec3(100.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            glm::scale(glm::mat4(1.0f), glm::vec3(80.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
            glm::scale(glm::mat4(1.0f), glm::vec3(500.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        };

        static const EnumArray<EnemyType, float> c_enemyHealth = {
            50.0f,
            25.0f,
            1000.0f,
        };

        static const EnumArray<EnemyType, RenderHandle> c_enemyMaterial = {
            GetHandle("DroneMaterial"),
            GetHandle("FastMaterial"),
            GetHandle("HeavyMaterial"),
        };

        std::optional<EventHandle> soundEvent = std::nullopt;
        if (type == EnemyType::Fast)
        {
            auto &audioSystem = Application::GetAudioSystem();
            soundEvent = audioSystem.CreateEvent("event:/enemy/fast");
            audioSystem.PlayEventAtPosition(soundEvent.value(), position);
        }

        auto bodyID = physicsWorld.CreateRigidBody(
            {
                .position = position,
                .rotation = glm::identity<glm::quat>(),
                .initialVelocity = glm::vec3(0.0f),
                .mass = 1.0f,
                .friction = 1.0f,
                .continuousCollision = false,
                .gravityFactor = 0.0f,
                .shape = MeshShape(renderer.GetMeshVertices(meshHandle), renderer.GetMeshIndices(meshHandle), c_enemyTransform[type]),
                .layer = ObjectLayer::Enemy,
            },
            BodyType::Rigidbody);
        RigidBodyState state = physicsWorld.GetRigidBodyState(bodyID);
        g_entityStore.enemies.push_back(
            CreateEntity(
                EnemyType(type),
                Transform{.position = position, .rotation = glm::identity<glm::quat>()},
                PhysicsProxy{
                    .bodyID = bodyID,
                    .currentState = state,
                    .previousState = state,
                },
                Renderable{
                    .mesh = meshHandle,
                    .material = c_enemyMaterial[type],
                    .renderMatrix = c_enemyTransform[type],
                },
                EnemyAI{},
                SoundEmitter{.event = soundEvent},
                Health{.current = c_enemyHealth[type], .max = c_enemyHealth[type]}));
    }

    void GameStateImpl::OnEnter(GameStates::PlayingState &state)
    {
        for (int32_t i = 0; i < g_entityStore.tiles.size(); ++i)
        {
            glm::vec3 position = GetTilePosition(i);
            int32_t j = GetTileIndex(position);

            assert(i == j);
        }

        auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
        auto &physicsWorld = Application::GetPhysicsWorld();
        auto &audioSystem = Application::GetAudioSystem();

        g_entityStore = {};
        g_entityStore.startTime = Application::GetTimeSinceStart();

        glm::vec3 playerPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::quat playerRotation = glm::identity<glm::quat>();
        g_entityStore.playerEntity = CreateEntity(
            Transform{.position = playerPosition, .rotation = playerRotation},
            PhysicsProxy{
                .bodyID = physicsWorld.CreateRigidBody(
                    {
                        .position = playerPosition,
                        .rotation = playerRotation,
                        .initialVelocity = glm::vec3(0.0f),
                        .mass = 1.0f,
                        .friction = 0.0f,
                        .continuousCollision = true,
                        .shape = CapsuleShape(35.0f, 49.0f),
                        .layer = ObjectLayer::Player,
                    },
                    BodyType::Character),
            },
            PlayerMovement{
                .dashSpeed = 2000.0f,
                .jumpSpeed = 750.0f,
            },
            PlayerAnimations{
                Animation{
                    .duration = 12.0f / 60.0f,
                    .loop = false,
                    .keyframes = {
                        {.position = glm::vec3(0.0f, 0.0f, -5.0f),
                         .rotation = glm::quat(glm::vec3(0.0f, glm::radians(0.0f), 0.0f)), // Initial slight rotation
                         .time = 0.0f},
                        {.position = glm::vec3(0.0f, -1.0f, 15.0f),
                         .rotation = glm::quat(glm::vec3(0.0f, glm::radians(-0.0f), 0.0f)), // Small recoil rotation forward
                         .time = 2.0f / 60.0f},
                        {.position = glm::vec3(0.0f, 0.0f, 0.0f),
                         .rotation = glm::quat(glm::vec3(0.0f, glm::radians(0.0f), 0.0f)), // Return to neutral
                         .time = 12.0f / 60.0f},
                    },
                },
                Animation{
                    .duration = 0.25f,
                    .loop = false,
                    .keyframes = {
                        {.position = glm::vec3(0.0f, -20.0f, 0.0f), .rotation = glm::quat(glm::vec3(glm::radians(-45.0f), 0.0f, 0.0f)), .time = 0.0f},
                        {.position = glm::vec3(0.0f, 0.0f, 0.0f), .rotation = glm::quat(glm::vec3(0.0f, glm::radians(0.0f), 0.0f)), .time = 0.25f},
                    },
                },
                Animation{
                    .duration = 0.3f,
                    .loop = false,
                    .keyframes = {
                        {.position = glm::vec3(0.0f, -10.0f, 0.0f), .rotation = glm::quat(glm::vec3(glm::radians(0.0f), 0.0f, 0.0f)), .time = 0.0f},
                        {.position = glm::vec3(0.0f, 0.0f, 0.0f), .rotation = glm::quat(glm::vec3(0.0f, glm::radians(0.0f), 0.0f)), .time = 0.3f},
                    },
                },
            },
            Health{}, Inventory{.currentWeaponIndex = 0});

        g_entityStore.weaponEntities = {
            CreateEntity(
                Transform{.position = playerPosition, .rotation = playerRotation},
                Renderable{
                    .mesh = GetHandle(MK_ASSET_PATH("models/gun.dat")),
                    .material = GetHandle("WeaponMaterial"),
                    .renderMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                },
                WeaponFireAction{
                    .automatic = true,
                    .fireRate = 700.0f,
                    .fireSoundEvent = "event:/weapons/plasma",
                },
                ProjectileBulletEmitter{
                    .type = ProjectileType::PlasmaBullet,
                    .speed = 8000.0f,
                    .damage = 10.0f,
                    .lifetime = 1.0f,
                }),
            CreateEntity(
                Transform{.position = playerPosition, .rotation = playerRotation},
                Renderable{
                    .mesh = GetHandle(MK_ASSET_PATH("models/launcher/launcher.dat")),
                    .material = GetHandle("WeaponMaterial"),
                    .renderMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
                },
                WeaponFireAction{
                    .automatic = false,
                    .fireRate = 20.0f,
                    .fireSoundEvent = "event:/weapons/launcher",
                },
                ProjectileBulletEmitter{
                    .type = ProjectileType::Rocket,
                    .speed = 3000.0f,
                    .damage = 100.0f,
                    .lifetime = 10.0f,
                }),
        };

        glm::vec3 cameraOffset = glm::vec3(0.0f, 0.0f, 200.0f);
        g_entityStore.cameraEntity = CreateEntity(
            Transform{.position = cameraOffset, .rotation = glm::identity<glm::quat>()},
            CameraSocket{},
            CameraShakes{
                // Fire
                CameraShake{
                    .duration = 0.15f,
                    .frequency = 5.0f,
                    .pitch = glm::radians(1.0f),
                    .yaw = glm::radians(1.0f),
                },
                // Damage
                CameraShake{
                    .duration = 0.2f,
                    .frequency = 4.0f,
                    .pitch = glm::radians(3.0f),
                    .yaw = glm::radians(3.0f),
                },
            });

        // Create floor collider
        g_entityStore.floorBodyID = physicsWorld.CreateRigidBody(
            {
                .position = glm::vec3(0.0f, 0.0f, 0.0f),
                .rotation = glm::identity<glm::quat>(),
                .initialVelocity = glm::vec3(0.0f),
                .mass = 0.0f,
                .friction = 0.0f,
                .continuousCollision = false,
                .shape = BoxShape(glm::vec3(c_tilesPerRow * c_tileSize * c_tileScale / 2.0f, 10.0f, c_tilesPerRow * c_tileSize * c_tileScale / 2.0f)),
                .layer = ObjectLayer::NonMoving,
            },
            BodyType::Rigidbody);

        physicsWorld.RegisterContactListener(g_entityStore.floorBodyID);

        g_entityStore.tiles.fill({});

        renderer.SetParticleAtlasMaterial(GetHandle("ParticleAtlasMaterial"));
        // renderer.SetSkybox(GetHandle("skybox"));
        renderer.SetEnvironmentMap(GetHandle("environment"));

        g_entityStore.ambienceEvent = audioSystem.CreateEvent("event:/ambience");
        audioSystem.PlayEvent(g_entityStore.ambienceEvent);
    }

    void GameStateImpl::OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::PlayingState &state)
    {
        if (inputState.Pressed(InputActionType::Escape))
        {
            state.shouldExitGame = true;
        }

        if (inputState.Pressed(InputActionType::DebugOption1))
        {
            g_debugCamera.active = !g_debugCamera.active;
        }

        if (g_debugCamera.active)
        {
            g_debugCamera.yaw += inputState.lookAxis.x * 1.0f;
            g_debugCamera.pitch = glm::clamp(inputState.lookAxis.y * 1.0f + g_debugCamera.pitch, -glm::half_pi<float>(), glm::half_pi<float>());
            g_debugCamera.rotation = glm::quat(glm::vec3(g_debugCamera.pitch, g_debugCamera.yaw, 0.0f));

            glm::vec3 forward = g_debugCamera.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));

            g_debugCamera.position += forward * inputState.movementAxis.y * 500.0f * dt;
            g_debugCamera.position += right * inputState.movementAxis.x * 500.0f * dt;

            if (inputState.Pressed(InputActionType::NextOption))
            {
                g_debugCamera.fov += 5.0f;
            }
            else if (inputState.Pressed(InputActionType::PreviousOption))
            {
                g_debugCamera.fov -= 5.0f;
            }
        }

        // Input system
        if (!g_debugCamera.active)
        {
            Transform &playerTransform = g_entityStore.playerEntity.GetComponent<Transform>();
            PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
            PlayerMovement &playerMovement = g_entityStore.playerEntity.GetComponent<PlayerMovement>();
            CameraSocket &cameraSocket = g_entityStore.cameraEntity.GetComponent<CameraSocket>();
            PlayerAnimations &playerAnimations = g_entityStore.playerEntity.GetComponent<PlayerAnimations>();

            // Rotate around the Y axis
            float yaw = 0.0f;
            float pitch = 0.0f;

            float oldYaw = cameraSocket.yaw;
            float oldPitch = cameraSocket.pitch;

            if (glm::length(inputState.lookAxis) > 0.0f)
            {
                yaw = inputState.lookAxis.x;
                cameraSocket.yaw += yaw;
                glm::quat newRotation = glm::rotate(glm::identity<glm::quat>(), cameraSocket.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
                physicsWorld.SetRotation(playerProxy.bodyID, newRotation);

                pitch = inputState.lookAxis.y;
                cameraSocket.pitch = glm::clamp(cameraSocket.pitch + pitch, -glm::half_pi<float>() * 0.99f, glm::half_pi<float>() * 0.99f);
            }

            playerMovement.yawSpeed = (cameraSocket.yaw - oldYaw) / dt;
            playerMovement.pitchSpeed = (cameraSocket.pitch - oldPitch) / dt;

            CharacterGroundState groundState = physicsWorld.GetCharacterGroundState(playerProxy.bodyID);
            glm::vec3 velocity = glm::vec3(0.0f);
            velocity.y = playerProxy.currentState.linearVelocity.y;

            {
                // Move forward
                float speed = playerMovement.dashTimer.IsRunning() ? playerMovement.dashSpeed : 750.0f;
                glm::vec3 forward = playerTransform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 alongForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
                glm::vec3 right = glm::cross(alongForward, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::vec3 movement = (playerMovement.dashTimer.IsRunning() ? playerMovement.dashDirection : (alongForward * inputState.movementAxis.y + right * inputState.movementAxis.x)) * speed;
                float movementFactor = (playerMovement.dashTimer.IsRunning() ? 50.0f : 20.0f) * (groundState == CharacterGroundState::InAir ? 0.5f : 1.0f);
                movement = glm::mix(playerProxy.currentState.linearVelocity, movement, glm::clamp(movementFactor * dt, 0.0f, 1.0f));
                velocity.x = movement.x;
                velocity.z = movement.z;
                if (playerMovement.dashTimer.IsRunning())
                {
                    velocity.y = 0.0f;
                }
            }

            // Stamina system
            constexpr float c_staminaRegenRate = 0.5f;
            {
                playerMovement.stamina = glm::clamp(playerMovement.stamina + c_staminaRegenRate * dt, 0.0f, 1.0f);
            }

            constexpr float c_jumpStaminaCost = 0.4f;
            if (inputState.Pressed(InputActionType::Jump))
            {
                if (!playerMovement.wantsToJump && playerMovement.stamina >= c_jumpStaminaCost)
                {
                    playerMovement.wantsToJump = true;
                    playerMovement.stamina -= c_jumpStaminaCost;
                    playerAnimations[PlayerAnimationType::JumpAnimation].time = 0.0f;
                    audioSystem.PlayEvent("event:/monke/jump");
                }
            }

            constexpr float c_dashStaminaCost = 0.3f;
            playerMovement.dashTimer.Tick(dt);
            if (inputState.Pressed(InputActionType::Dash))
            {
                if (playerMovement.dashTimer.HasElapsed() && playerMovement.stamina >= c_dashStaminaCost)
                {
                    playerMovement.dashDirection = playerTransform.rotation * (glm::length(inputState.movementAxis) > 0.0f ? glm::normalize(glm::vec3(inputState.movementAxis.x, 0.0f, -inputState.movementAxis.y)) : (glm::vec3(0.0f, 0.0f, -1.0f)));
                    playerMovement.dashTimer.Reset(0.2f);
                    audioSystem.PlayEvent("event:/monke/dash");
                    playerMovement.stamina -= c_dashStaminaCost;
                }
            }

            physicsWorld.SetLinearVelocity(playerProxy.bodyID, velocity);
        }

        // Physics interpolation system
        if (!g_debugCamera.active)
        {
            float alpha = glm::clamp(Application::GetTimeSincePhysics() / c_fixedUpdateInterval, 0.0f, 1.0f);
            ForEach<Transform, PhysicsProxy>(
                [&](Transform &transform, PhysicsProxy &proxy)
                {
                    transform.position = glm::mix(proxy.previousState.position, proxy.currentState.position, alpha);
                    transform.rotation = glm::slerp(proxy.previousState.rotation, proxy.currentState.rotation, alpha);
                },
                g_entityStore.playerEntity, g_entityStore.staticEntities, g_entityStore.projectiles, g_entityStore.enemies);
        }

        // Camera attach to player
        {
            Transform &playerTransform = g_entityStore.playerEntity.GetComponent<Transform>();
            PlayerMovement &playerMovement = g_entityStore.playerEntity.GetComponent<PlayerMovement>();
            Transform &cameraTransform = g_entityStore.cameraEntity.GetComponent<Transform>();
            CameraSocket &cameraSocket = g_entityStore.cameraEntity.GetComponent<CameraSocket>();

            cameraTransform.position = playerTransform.position + glm::vec3(0.0f, 60.0f, -10.0f);
            // Set from pitch and yaw
            cameraTransform.rotation = glm::rotate(glm::identity<glm::quat>(), cameraSocket.yaw, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::rotate(glm::identity<glm::quat>(), cameraSocket.pitch, glm::vec3(1.0f, 0.0f, 0.0f));
            cameraSocket.fov = glm::mix(cameraSocket.fov, playerMovement.dashTimer.IsRunning() ? 75.0f : 70.0f, glm::clamp(40.0f * dt, 0.0f, 1.0f));
        }

        // Camera shake
        {
            ForEach<CameraShakes>(
                [&](CameraShakes &cameraShakes)
                {
                    for (auto &shake : cameraShakes)
                    {
                        shake.time = glm::clamp(shake.time + dt, 0.0f, shake.duration);
                    }
                },
                g_entityStore.cameraEntity);
        }

        // Camera audio
        {
            Transform &playerTransform = g_entityStore.playerEntity.GetComponent<Transform>();
            PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
            audioSystem.SetListenerState(playerProxy.currentState.position, playerProxy.currentState.rotation, playerProxy.currentState.linearVelocity);
        }

        // Weapon switch
        {
            PlayerAnimations &playerAnimations = g_entityStore.playerEntity.GetComponent<PlayerAnimations>();
            int32_t newIndex = -1;

            bool next = inputState.Pressed(InputActionType::NextOption);
            bool previous = inputState.Pressed(InputActionType::PreviousOption);

            if (next || previous)
            {
                int32_t currentIndex = g_entityStore.playerEntity.GetComponent<Inventory>().currentWeaponIndex;
                newIndex = currentIndex + (next ? 1 : -1);
                if (newIndex < 0)
                {
                    newIndex = g_entityStore.weaponEntities.size() - 1;
                }
                else if (newIndex >= static_cast<int32_t>(g_entityStore.weaponEntities.size()))
                {
                    newIndex = 0;
                }
            }

            for (int32_t i = 0; i < static_cast<int32_t>(g_entityStore.weaponEntities.size()); i++)
            {
                if (inputState.Pressed(InputActionType::Option1, i))
                {
                    newIndex = i;
                    break;
                }
            }

            Inventory &inventory = g_entityStore.playerEntity.GetComponent<Inventory>();
            if (newIndex >= 0 && newIndex != inventory.currentWeaponIndex)
            {
                inventory.currentWeaponIndex = newIndex;
                playerAnimations[PlayerAnimationType::EquipAnimation].time = 0.0f;
            }
        }

        // Weapon animation
        {
            PlayerAnimations &playerAnimations = g_entityStore.playerEntity.GetComponent<PlayerAnimations>();
            for (auto &animation : playerAnimations)
            {
                UpdateAnimation(animation, dt);
            }
        }

        // Weapon attach to camera
        {
            Transform &playerTransform = g_entityStore.playerEntity.GetComponent<Transform>();
            Transform &cameraTransform = g_entityStore.cameraEntity.GetComponent<Transform>();
            CameraSocket &cameraSocket = g_entityStore.cameraEntity.GetComponent<CameraSocket>();
            Transform &weaponTransform = GetCurrentPlayerWeapon().GetComponent<Transform>();
            PlayerMovement &playerMovement = g_entityStore.playerEntity.GetComponent<PlayerMovement>();
            PlayerAnimations &playerAnimations = g_entityStore.playerEntity.GetComponent<PlayerAnimations>();

            PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
            glm::vec3 velocity = playerProxy.currentState.linearVelocity;
            glm::vec2 groundVelocity = glm::vec2(velocity.x, velocity.z);

            Transform weaponSwayTransform = Transform{};
            if (glm::length(groundVelocity) > glm::epsilon<float>())
            {
                glm::vec3 forward = playerTransform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::vec2 groundDirection = glm::length(groundVelocity) > glm::epsilon<float>() ? glm::normalize(groundVelocity) : glm::vec2(0.0f);
                glm::vec2 lookDirection = glm::length(glm::vec2(forward.x, forward.z)) > glm::epsilon<float>() ? glm::normalize(glm::vec2(forward.x, forward.z)) : glm::vec2(0.0f);
                glm::vec2 sideDirection = glm::length(glm::vec2(right.x, right.z)) > glm::epsilon<float>() ? glm::normalize(glm::vec2(right.x, right.z)) : glm::vec2(0.0f);

                // Get angle between velcoty and look direction
                float walkFactor = glm::length(velocity) / 750.0f;
                float forwardFactor = walkFactor * glm::dot(lookDirection, groundDirection);
                float upFactor = 1.0f - glm::abs(cameraSocket.pitch / glm::radians(90.0f));
                weaponSwayTransform.position += glm::vec3(0.0f, 0.0f, 3.0f * upFactor * forwardFactor);

                float rightFactor = walkFactor * glm::dot(groundDirection, sideDirection);
                weaponSwayTransform.rotation = glm::rotate(weaponSwayTransform.rotation, rightFactor * glm::radians(-3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            }

            glm::vec3 angularVelocity = glm::vec3(playerMovement.pitchSpeed, playerMovement.yawSpeed, 0.0f);
            if (glm::length(angularVelocity) > glm::epsilon<float>())
            {
                // make it so that the weapon rotates in the opposite direction of the player's rotation
                float amount = glm::clamp(glm::length(angularVelocity) * 0.005f, 0.0f, glm::radians(3.0f));
                weaponSwayTransform.rotation = glm::rotate(weaponSwayTransform.rotation, amount, glm::normalize(angularVelocity));
            }

            static Transform oldWeaponSwayTransform = weaponSwayTransform;
            weaponSwayTransform.position = glm::mix(oldWeaponSwayTransform.position, weaponSwayTransform.position, glm::clamp(20.0f * dt, 0.0f, 1.0f));
            weaponSwayTransform.rotation = glm::slerp(oldWeaponSwayTransform.rotation, weaponSwayTransform.rotation, glm::clamp(20.0f * dt, 0.0f, 1.0f));
            assert_transform_valid(oldWeaponSwayTransform);
            assert_transform_valid(weaponSwayTransform);
            oldWeaponSwayTransform = weaponSwayTransform;

            Transform animationTransform = Transform{};
            for (auto &animation : playerAnimations)
            {
                Transform clipTransform = GetAnimationTransform(animation);
                animationTransform.position += clipTransform.position;
                animationTransform.rotation *= clipTransform.rotation;
            }

            static Transform oldAnimationTransform = animationTransform;
            animationTransform.position = glm::mix(oldAnimationTransform.position, animationTransform.position, glm::clamp(25.0f * dt, 0.0f, 1.0f));
            animationTransform.rotation = glm::slerp(oldAnimationTransform.rotation, animationTransform.rotation, glm::clamp(25.0f * dt, 0.0f, 1.0f));
            assert_transform_valid(oldAnimationTransform);
            assert_transform_valid(animationTransform);
            oldAnimationTransform = animationTransform;

            weaponTransform.SetMatrix(cameraTransform.GetMatrix() * glm::translate(glm::mat4(1.0f), glm::vec3(20.0f, -40.0f, -50.0f)) * weaponSwayTransform.GetMatrix() * animationTransform.GetMatrix());

            assert_transform_valid(weaponTransform);
        }

        // Tick fire for all weapons
        for (auto &weapon : g_entityStore.weaponEntities)
        {
            WeaponFireAction &fireAction = weapon.GetComponent<WeaponFireAction>();
            fireAction.fireTimer.Tick(dt);
        }

        static float timeScale = 1.0f;
        timeScale = glm::mix(timeScale, inputState.Down(InputActionType::Aim) ? 0.2f : 1.0f, glm::clamp(10.0f * dt, 0.0f, 1.0f));
        Application::SetTimeScale(timeScale);

        // Player fire
        {
            WeaponFireAction &fireAction = GetCurrentPlayerWeapon().GetComponent<WeaponFireAction>();
            {
                bool wantsToFire = fireAction.automatic ? inputState.Down(InputActionType::Attack) : inputState.Pressed(InputActionType::Attack);
                fireAction.fire = wantsToFire && fireAction.fireTimer.HasElapsed();
                if (fireAction.fire)
                {
                    fireAction.fireTimer.Reset(60.0f / fireAction.fireRate);
                    audioSystem.PlayEvent(std::string(fireAction.fireSoundEvent));
                    g_entityStore.cameraEntity.GetComponent<CameraShakes>()[CameraShakeType::Weapon].time = 0.0f;
                }
            }

            if (fireAction.fire)
            {
                ProjectileBulletEmitter &emitter = GetCurrentPlayerWeapon().GetComponent<ProjectileBulletEmitter>();
                Transform &weaponTransform = GetCurrentPlayerWeapon().GetComponent<Transform>();
                Transform &cameraTransform = g_entityStore.cameraEntity.GetComponent<Transform>();
                PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
                glm::vec3 forward = weaponTransform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 position = weaponTransform.position + forward * 150.0f;
                glm::vec3 velocity = forward * emitter.speed;
                glm::quat rotation = weaponTransform.rotation;
                float scale = 0.1f;
                glm::vec4 color = glm::vec4(0.0f, 5.0f, 10.0f, 1.0f);

                auto bodyId = physicsWorld.CreateRigidBody(
                    {
                        .position = position,
                        .rotation = rotation,
                        .initialVelocity = velocity,
                        .mass = 1.0f,
                        .friction = 0.0f,
                        .continuousCollision = true,
                        .gravityFactor = emitter.gravity,
                        .shape = SphereShape(scale * 100.0f),
                        .layer = ObjectLayer::PlayerProjectile,
                    },
                    BodyType::Rigidbody);
                physicsWorld.RegisterContactListener(bodyId);
                auto currentState = physicsWorld.GetRigidBodyState(bodyId);
                auto previousState = currentState;

                g_entityStore.projectiles.push_back(CreateEntity(
                    ProjectileType(emitter.type),
                    Transform{.position = position, .rotation = rotation, .scale = glm::vec3(scale)},
                    PhysicsProxy{
                        .bodyID = bodyId,
                        .currentState = currentState,
                        .previousState = previousState,
                    },
                    Renderable{
                        .mesh = GetHandle(MK_ASSET_PATH("models/sphere.dat")),
                        .material = GetHandle("WhiteMaterial"),
                        .color = color,
                    },
                    Lifetime{.timer = DynamicTimer(emitter.lifetime)}));

                g_entityStore.playerEntity.GetComponent<PlayerAnimations>()[PlayerAnimationType::ShootAnimation].time = 0.0f;
            }
        }

        // Non corrupted tiles
        {
            g_entityStore.nonCorruptedTiles.clear();
            for (int i = 0; i < c_tilesPerRow * c_tilesPerRow; i++)
            {
                if (g_entityStore.tiles[i].corruption < 1.0f)
                {
                    g_entityStore.nonCorruptedTiles.push_back(i);
                }
            }

            std::shuffle(g_entityStore.nonCorruptedTiles.begin(), g_entityStore.nonCorruptedTiles.end(), std::default_random_engine());
        }

        // Reset tile amount
        {
            for (auto &tile : g_entityStore.tiles)
            {
                tile.reset = glm::max(tile.reset - (1.0f / 0.5f) * dt, 0.0f);
            }
        }

        // Wave spawn system
        {
            if (g_entityStore.waveTimer.Tick(dt))
            {
                g_entityStore.wave++;
                g_entityStore.waveTimer.Reset(glm::mix(20.0f, 40.0f, glm::clamp(static_cast<float>(g_entityStore.wave) / 10.0f, 0.0f, 1.0f)));
                int32_t numEnemies = 8 + g_entityStore.wave * 2;
                for (int i = 0; i < numEnemies; i++)
                {
                    float angle = glm::radians(static_cast<float>(i) / static_cast<float>(numEnemies) * 360.0f);
                    glm::vec3 position = glm::vec3(glm::cos(angle), 0.0f, glm::sin(angle)) * 8000.0f + glm::vec3(0.0f, 150.0f, 0.0f);
                    CreateEnemy(EnemyType::Drone, position);
                }

                float offsetAngle = glm::radians(static_cast<float>(rand() % 360));
                // Spawn a small wave of fast enemies, make it in a cluster
                int32_t numFastEnemies = g_entityStore.wave % 2 == 0 ? (4 + g_entityStore.wave) : 0;
                for (int i = 0; i < numFastEnemies; i++)
                {
                    float angle = offsetAngle + glm::radians(static_cast<float>(i) / static_cast<float>(numFastEnemies) * 30.0f);
                    glm::vec3 position = glm::vec3(glm::cos(angle), 0.15f, glm::sin(angle)) * 6000.0f;
                    CreateEnemy(EnemyType::Fast, position);
                }

                offsetAngle = glm::radians(static_cast<float>(rand() % 360));
                int32_t numHeavyEnemies = static_cast<int32_t>(glm::floor(0.25f * g_entityStore.wave));
                for (int i = 0; i < numHeavyEnemies; i++)
                {
                    float angle = offsetAngle + glm::radians(static_cast<float>(i) / static_cast<float>(numHeavyEnemies) * 360.0f);
                    CreateEnemy(EnemyType::Heavy, glm::vec3(glm::cos(angle), 0.1f, glm::sin(angle)) * 4000.0f);
                }
            }
        }

        // Damage system
        ForEach<PhysicsProxy, Health>(
            [&](PhysicsProxy &proxy, Health &health)
            {
                if (g_entityStore.damageEvents.find(proxy.bodyID) != g_entityStore.damageEvents.end())
                {
                    health.current = glm::clamp(health.current - g_entityStore.damageEvents[proxy.bodyID], 0.0f, health.max);
                }
            },
            g_entityStore.playerEntity,
            g_entityStore.enemies);

        Filter<Transform, PhysicsProxy, Renderable, SoundEmitter, Health>(
            [&](Transform &transform, PhysicsProxy &proxy, Renderable &renderable, SoundEmitter &soundEmitter, Health &health) -> bool
            {
                if (health.current <= 0.0f)
                {
                    physicsWorld.SetGravityFactor(proxy.bodyID, 1.0f);

                    // Stop audio event
                    if (soundEmitter.event.has_value())
                    {
                        audioSystem.StopEvent(soundEmitter.event.value());
                        audioSystem.ReleaseEvent(soundEmitter.event.value());
                    }

                    // Add static entity with same everything
                    g_entityStore.staticEntities.push_back(
                        CreateEntity(
                            Transform(transform),
                            PhysicsProxy(proxy),
                            Renderable{
                                .mesh = renderable.mesh,
                                .material = renderable.material,
                                .renderMatrix = renderable.renderMatrix,
                                .color = renderable.color,
                                .emissive = glm::vec4(0.0f),
                            },
                            Lifetime{.timer = DynamicTimer(5.0f)}));

                    SceneRenderer &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
                    ParticleHelper::SpawnExplosionEffect(g_entityStore.particleJobs, transform.position);
                    Application::GetAudioSystem().PlayEventAtPosition("event:/enemy/death", transform.position, glm::vec3(0.0f));

                    return true;
                }

                return false;
            },
            g_entityStore.enemies);

        // Player damage reaction
        {
            Health &playerHealth = g_entityStore.playerEntity.GetComponent<Health>();
            if (g_entityStore.damageEvents.find(g_entityStore.playerEntity.GetComponent<PhysicsProxy>().bodyID) != g_entityStore.damageEvents.end())
            {
                g_entityStore.cameraEntity.GetComponent<CameraShakes>()[CameraShakeType::Damage].time = 0.0f;
            }

            if (playerHealth.current <= 0.0f)
            {
                g_entityStore.isGameOver = true;
            }
        }

        // Player health regen
        {
            Health &playerHealth = g_entityStore.playerEntity.GetComponent<Health>();
            playerHealth.current = glm::clamp(playerHealth.current + 0.5f * dt, 0.0f, playerHealth.max);
        }

        g_entityStore.damageEvents.clear();

        // Enemy AI system
        ForEach<Transform, PhysicsProxy, Health, EnemyType, EnemyAI>(
            [&](Transform &transform, PhysicsProxy &proxy, Health &health, EnemyType &type, EnemyAI &ai)
            {
                switch (type)
                {
                case EnemyType::Drone:
                {
                    glm::vec3 direction = glm::vec3(0.0f);
                    // If outdide of the grid, move towards the center
                    if (GetTileIndex(transform.position) == -1)
                    {
                        direction = glm::normalize(-transform.position);
                    }
                    else
                    {
                        // Have we achieved our goal?
                        if (ai.target.has_value())
                        {
                            uint32_t tileIndex = GetTileIndex(ai.target.value());
                            if (tileIndex != -1 && g_entityStore.tiles[tileIndex].corruption >= 1.0f)
                            {
                                ai.target.reset();
                            }
                        }

                        // If we don't have a target, set it
                        if (!ai.target.has_value())
                        {
                            if (!g_entityStore.nonCorruptedTiles.empty())
                            {
                                ai.target = GetTilePosition(g_entityStore.nonCorruptedTiles.back());
                                g_entityStore.nonCorruptedTiles.pop_back();
                            }
                            else
                            {
                                // Random tile
                                ai.target = GetTilePosition(rand() % g_entityStore.tiles.size());
                            }
                        }

                        glm::vec3 toTarget = ai.target.value() - transform.position;
                        // We just stay at the current position if the target is too close
                        if (glm::length(toTarget) > 50.0f)
                        {
                            // Move towards the target
                            direction = glm::normalize(toTarget);
                        }
                    }

                    if (glm::length(direction) > glm::epsilon<float>())
                    {
                        physicsWorld.SetLinearVelocity(
                            proxy.bodyID,
                            direction * 300.0f);
                    }

                    glm::vec3 enemyToPlayer = g_entityStore.playerEntity.GetComponent<Transform>().position - transform.position;
                    if (glm::length(enemyToPlayer) > 100.0f)
                    {
                        glm::quat rotation = glm::normalize(glm::quatLookAt(glm::normalize(enemyToPlayer), glm::vec3(0.0f, 1.0f, 0.0f)));
                        physicsWorld.SetRotation(
                            proxy.bodyID,
                            rotation);
                    }
                }
                break;

                case EnemyType::Fast:
                {
                    constexpr float c_attackRange = 150.0f;

                    // Just go toeards the player fast
                    glm::vec3 enemyToPlayer = g_entityStore.playerEntity.GetComponent<Transform>().position - transform.position;
                    if (glm::length(enemyToPlayer) > c_attackRange)
                    {
                        glm::vec3 direction = glm::normalize(enemyToPlayer);
                        glm::quat rotation = glm::normalize(glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
                        physicsWorld.SetRotation(
                            proxy.bodyID,
                            rotation);
                        physicsWorld.SetLinearVelocity(
                            proxy.bodyID,
                            direction * 800.0f);
                    }
                    else
                    {
                        health.current = 0.0f;
                        g_entityStore.damageEvents[g_entityStore.playerEntity.GetComponent<PhysicsProxy>().bodyID] += 20.0f / (1.0f + glm::length(enemyToPlayer) / c_attackRange);
                    }
                }
                break;

                case EnemyType::Heavy:
                {
                    constexpr float c_attackEnterRange = 3000.0f;
                    constexpr float c_attackLeaveRange = 4000.0f;
                    constexpr float c_attackDamage = 10.0f;

                    glm::vec3 enemyToPlayer = g_entityStore.playerEntity.GetComponent<Transform>().position - transform.position;
                    if (!ai.isAttacking && glm::length(enemyToPlayer) < c_attackEnterRange)
                    {
                        ai.isAttacking = true;
                    }
                    else if (ai.isAttacking && glm::length(enemyToPlayer) > c_attackLeaveRange)
                    {
                        ai.isAttacking = false;
                    }

                    glm::vec3 direction = glm::normalize(enemyToPlayer);
                    glm::quat rotation = glm::normalize(glm::quatLookAt(direction, glm::vec3(0.0f, 1.0f, 0.0f)));
                    physicsWorld.SetRotation(
                        proxy.bodyID,
                        rotation);
                    physicsWorld.SetLinearVelocity(
                        proxy.bodyID,
                        ai.isAttacking ? glm::vec3(0.0f) : direction * 300.0f);
                }
                break;
                default:
                    break;
                }
            },
            g_entityStore.enemies);

        // Enemy attack system
        {
            constexpr float c_attackRange = 3000.0f;
            constexpr float c_attackDamage = 10.0f;
            ForEach<Transform, EnemyType, EnemyAI>(
                [&](Transform &transform, EnemyType &type, EnemyAI &ai)
                {
                    if (type == EnemyType::Drone)
                    {
                        // If the shoot timer is up, shoot, then reset to random value
                        if (ai.shootTimer.Tick(dt))
                        {
                            glm::vec3 enemyToPlayer = g_entityStore.playerEntity.GetComponent<Transform>().position - transform.position;
                            if (glm::length(enemyToPlayer) < c_attackRange)
                            {
                                glm::vec3 forward = transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                                glm::vec3 position = transform.position + forward * 150.0f;
                                glm::vec3 velocity = forward * 2000.0f;
                                glm::quat rotation = transform.rotation;
                                float scale = 0.1f;
                                glm::vec4 color = glm::vec4(10.0f, 0.0f, 0.0f, 1.0f);

                                auto bodyId = physicsWorld.CreateRigidBody(
                                    {
                                        .position = position,
                                        .rotation = rotation,
                                        .initialVelocity = velocity,
                                        .mass = 1.0f,
                                        .friction = 0.0f,
                                        .continuousCollision = true,
                                        .gravityFactor = 0.0f,
                                        .shape = SphereShape(scale * 100.0f),
                                        .layer = ObjectLayer::EnemyProjectile,
                                    },
                                    BodyType::Rigidbody);
                                physicsWorld.RegisterContactListener(bodyId);
                                auto currentState = physicsWorld.GetRigidBodyState(bodyId);
                                auto previousState = currentState;

                                g_entityStore.projectiles.push_back(CreateEntity(
                                    ProjectileType::EnemyBullet,
                                    Transform{.position = position, .rotation = rotation, .scale = glm::vec3(scale)},
                                    PhysicsProxy{
                                        .bodyID = bodyId,
                                        .currentState = currentState,
                                        .previousState = previousState,
                                    },
                                    Renderable{
                                        .mesh = GetHandle(MK_ASSET_PATH("models/sphere.dat")),
                                        .material = GetHandle("WhiteMaterial"),
                                        .color = color,
                                    },
                                    Lifetime{.timer = DynamicTimer(5.0f)}));
                            }

                            ai.shootTimer.Reset(2.0f + static_cast<float>(rand() % 6));
                        }
                    }
                    else if (type == EnemyType::Heavy)
                    {
                        glm::vec3 enemyToPlayer = g_entityStore.playerEntity.GetComponent<Transform>().position - transform.position;
                        if (ai.shootTimer.Tick(dt))
                        {
                            if (ai.isAttacking)
                            {
                                glm::vec3 forward = transform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                                glm::vec3 position = transform.position + forward * 150.0f;
                                glm::vec3 velocity = forward * 3000.0f;
                                glm::quat rotation = transform.rotation;
                                float scale = 0.5f;
                                glm::vec4 color = glm::vec4(0.0f, 0.0f, 10.0f, 1.0f);

                                auto bodyId = physicsWorld.CreateRigidBody(
                                    {
                                        .position = position,
                                        .rotation = rotation,
                                        .initialVelocity = velocity,
                                        .mass = 1.0f,
                                        .friction = 0.0f,
                                        .continuousCollision = true,
                                        .gravityFactor = 0.0f,
                                        .shape = SphereShape(scale * 100.0f),
                                        .layer = ObjectLayer::EnemyProjectile,
                                    },
                                    BodyType::Rigidbody);
                                physicsWorld.RegisterContactListener(bodyId);
                                auto currentState = physicsWorld.GetRigidBodyState(bodyId);
                                auto previousState = currentState;

                                g_entityStore.projectiles.push_back(CreateEntity(
                                    ProjectileType::EnemyBullet,
                                    Transform{.position = position, .rotation = rotation, .scale = glm::vec3(scale)},
                                    PhysicsProxy{
                                        .bodyID = bodyId,
                                        .currentState = currentState,
                                        .previousState = previousState,
                                    },
                                    Renderable{
                                        .mesh = GetHandle(MK_ASSET_PATH("models/sphere.dat")),
                                        .material = GetHandle("WhiteMaterial"),
                                        .color = color,
                                    },
                                    Lifetime{.timer = DynamicTimer(5.0f)}));
                            }

                            ai.shootTimer.Reset(0.5f);
                        }
                    }
                },
                g_entityStore.enemies);
        }

        // Enemy corruption system
        constexpr float c_corruptionRate = 0.4f;
        ForEach<Transform, EnemyType>(
            [&](Transform &transform, EnemyType &type)
            {
                int32_t tileIndex = GetTileIndex(transform.position);
                glm::vec3 tilePosition = GetTilePosition(tileIndex);
                if (tileIndex != -1 && glm::length(tilePosition - transform.position) < 200.0f)
                {
                    g_entityStore.tiles[tileIndex].corruption = glm::clamp(g_entityStore.tiles[tileIndex].corruption + c_corruptionRate * dt, 0.0f, 1.0f);
                }
            },
            g_entityStore.enemies);

        // Check if the the tiles are all corrupted, and set game over if so
        float totalCorruption = 0.0f;
        for (auto &tile : g_entityStore.tiles)
        {
            totalCorruption += tile.corruption;
        }
        totalCorruption /= static_cast<float>(g_entityStore.tiles.size());

        if (totalCorruption >= 0.98f)
        {
            g_entityStore.isGameOver = true;
        }

        // Enemy sound system
        {
            // When the timer if up, pick a random enemy, set their sound emitter to play a sound
            if (g_entityStore.enemySoundTimer.Tick(dt))
            {
                if (!g_entityStore.enemies.empty())
                {
                    int32_t randomIndex = rand() % g_entityStore.enemies.size();

                    auto &enemy = g_entityStore.enemies[randomIndex];
                    if (enemy.GetComponent<EnemyType>() != EnemyType::Fast)
                    {

                        auto &soundEmitter = enemy.GetComponent<SoundEmitter>();
                        soundEmitter.event = audioSystem.CreateEvent("event:/enemy/drone");
                        audioSystem.PlayEventAtPosition(soundEmitter.event.value(), enemy.GetComponent<Transform>().position, enemy.GetComponent<PhysicsProxy>().currentState.linearVelocity);
                    }

                    g_entityStore.enemySoundTimer.Reset(float(rand() % 5));
                }
            }

            // Update the sound emitter positions
            ForEach<Transform, PhysicsProxy, SoundEmitter>(
                [&](Transform &transform, PhysicsProxy &proxy, SoundEmitter &soundEmitter)
                {
                    if (soundEmitter.event)
                    {
                        audioSystem.SetEventPosition(soundEmitter.event.value(), transform.position, proxy.currentState.linearVelocity);
                    }
                },
                g_entityStore.enemies);
        }
    }

    void GameStateImpl::OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::PlayingState &state)
    {
        if (g_debugCamera.active)
        {
            return;
        }

        {
            PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
            PlayerMovement &playerMovement = g_entityStore.playerEntity.GetComponent<PlayerMovement>();
            if (playerMovement.wantsToJump)
            {
                physicsWorld.SetLinearVelocity(
                    playerProxy.bodyID,
                    glm::vec3(
                        playerProxy.currentState.linearVelocity.x,
                        playerMovement.jumpSpeed,
                        playerProxy.currentState.linearVelocity.z));
                playerMovement.wantsToJump = false;
            }
        }

        physicsWorld.StepSimulation(dt, numSteps);

        // Player ground state
        {
            CharacterGroundState newGroundState = physicsWorld.GetCharacterGroundState(g_entityStore.playerEntity.GetComponent<PhysicsProxy>().bodyID);
            CharacterGroundState &groundState = g_entityStore.playerEntity.GetComponent<PlayerMovement>().groundState;
            if (newGroundState == CharacterGroundState::OnGround && groundState == CharacterGroundState::InAir)
            {
                g_entityStore.playerEntity.GetComponent<PlayerAnimations>()[PlayerAnimationType::JumpAnimation].time = 0.0f;
            }
            groundState = newGroundState;
        }

        // Physics system
        {
            ForEach<Transform, PhysicsProxy>(
                [&](Transform &transform, PhysicsProxy &proxy)
                {
                    proxy.previousState = proxy.currentState;
                    proxy.currentState = physicsWorld.GetRigidBodyState(proxy.bodyID);
                },
                g_entityStore.playerEntity,
                g_entityStore.staticEntities,
                g_entityStore.projectiles,
                g_entityStore.enemies);
        }

        // Projectiles trail system
        {
            ForEach<Transform, PhysicsProxy, ProjectileType>(
                [&](Transform &transform, PhysicsProxy &proxy, ProjectileType &type)
                {
                    switch (type)
                    {
                    case ProjectileType::Rocket:
                        ParticleHelper::SpawnSpark(
                            g_entityStore.particleJobs,
                            transform.position,
                            // Purple
                            glm::vec4(glm::vec3(0.5f, 0.0f, 0.5f) * 10.0f, 1.0f),
                            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                        break;
                    default:
                        break;
                    }
                },
                g_entityStore.projectiles);
        }

        // Projectiles hit system
        {
            Filter<ProjectileType, Transform, PhysicsProxy, Renderable>(
                [&](ProjectileType &type, Transform &transform, PhysicsProxy &proxy, Renderable &renderable) -> bool
                {
                    const auto &contacts = physicsWorld.GetContacts(proxy.bodyID);

                    if (!contacts.empty())
                    {
                        for (const auto &contact : contacts)
                        {
                            if (contact.body == g_entityStore.floorBodyID && physicsWorld.GetObjectLayer(proxy.bodyID) == ObjectLayer::PlayerProjectile)
                            {
                                int32_t tileIndex = GetTileIndex(contact.position);
                                if (tileIndex != -1)
                                {
                                    g_entityStore.tiles[tileIndex].corruption = 0.0f; // glm::clamp(g_entityStore.tileCorruption[tileIndex] - 0.1f, 0.0f, 1.0f);
                                    g_entityStore.tiles[tileIndex].reset = 1.0f;
                                }

                                if (type == ProjectileType::Rocket)
                                {
                                    std::vector<int32_t> hitTiles = GetTilesInRadius(contact.position, 500.0f);
                                    for (auto &tileIndex : hitTiles)
                                    {
                                        g_entityStore.tiles[tileIndex].corruption = 0.0f; // glm::clamp(g_entityStore.tileCorruption[tileIndex] - 0.1f, 0.0f, 1.0f);
                                        g_entityStore.tiles[tileIndex].reset = 1.0f;
                                    }
                                }
                            }
                            else
                            {
                                constexpr EnumArray<ProjectileType, float> c_damageValues = {
                                    20.0f,  // PlasmaBullet
                                    150.0f, // Rocket
                                    10.0f,  // EnemyBullet
                                };

                                g_entityStore.damageEvents[contact.body] += c_damageValues[type];
                            }

                            if (physicsWorld.GetObjectLayer(contact.body) == ObjectLayer::Enemy)
                            {
                                Application::GetAudioSystem().PlayEventAtPosition("event:/enemy/hit", contact.position, glm::vec3(0.0f));
                            }

                            switch (type)
                            {
                            case ProjectileType::EnemyBullet:
                                ParticleHelper::SpawnImpactEffect(g_entityStore.particleJobs, contact.position, -contact.normal, glm::vec4(10.0f, 0.0f, 0.0f, 1.0f));
                                break;
                            case ProjectileType::PlasmaBullet:
                                ParticleHelper::SpawnImpactEffect(g_entityStore.particleJobs, contact.position, -contact.normal, glm::vec4(0.0f, 5.0f, 10.0f, 1.0f));
                                break;
                            case ProjectileType::Rocket:
                            {
                                constexpr float c_radius = 500.0f;
                                constexpr float c_explosionStrength = 2000.0f;
                                constexpr float c_falloffFactor = 0.5f * c_radius;
                                ParticleHelper::SpawnIceExplosionEffect(g_entityStore.particleJobs, contact.position);
                                Application::GetAudioSystem().PlayEventAtPosition("event:/explosion", contact.position, glm::vec3(0.0f));
                                std::vector<BodyID> hitBodies = physicsWorld.CastSphere(contact.position, c_radius);
                                for (auto &hitBody : hitBodies)
                                {
                                    if (hitBody != proxy.bodyID)
                                    {
                                        glm::vec3 position = physicsWorld.GetPosition(hitBody);
                                        float distance = glm::length(position - contact.position);
                                        float falloff = 1.0f / (1.0f + glm::pow(distance / c_falloffFactor, 2.0f));

                                        g_entityStore.damageEvents[hitBody] += 200.0f * falloff;
                                        physicsWorld.SetLinearVelocity(
                                            hitBody,
                                            (glm::normalize(position - contact.position) + glm::vec3(0.0f, 0.5f, 0.0f)) * c_explosionStrength * falloff);
                                    }
                                }
                            }
                            break;
                            default:
                                break;
                            }
                        }

                        physicsWorld.RemoveRigidBody(proxy.bodyID);
                        return true;
                    }

                    return false;
                },
                g_entityStore.projectiles);
        }

        // Lifetime system
        {
            Filter<Transform, PhysicsProxy, Lifetime>(
                [&](Transform &transform, PhysicsProxy &proxy, Lifetime &lifetime) -> bool
                {
                    if (lifetime.timer.Tick(dt))
                    {
                        physicsWorld.RemoveRigidBody(proxy.bodyID);
                        return true;
                    }

                    // also check not out of bounds in y
                    if (transform.position.y < -2000.0f || transform.position.y > 5000.0f)
                    {
                        physicsWorld.RemoveRigidBody(proxy.bodyID);
                        return true;
                    }

                    return false;
                },
                g_entityStore.staticEntities,
                g_entityStore.projectiles);
        }
    }

    void GameStateImpl::OnRender(Vultron::SceneRenderer &renderer, GameStates::PlayingState &state)
    {
        std::array<PointLightData, 4> pointsLights = {};

        ForEach<Transform, CameraSocket, CameraShakes>(
            [&](Transform &transform, CameraSocket &socket, CameraShakes &shakes)
            {
                float time = Application::GetTimeSinceStart();
                glm::quat shakeRotation = glm::identity<glm::quat>();
                glm::vec3 shakePosition = glm::vec3(0.0f);

                for (auto &shake : shakes)
                {
                    float t = glm::pow(glm::clamp(1.0f - shake.time / shake.duration, 0.0f, 1.0f), 3.0f);
                    float yaw = t * shake.yaw * PerlinNoiseHelper::Perlin(time * shake.frequency, 0.0f, 0);
                    float pitch = t * shake.pitch * PerlinNoiseHelper::Perlin(time * shake.frequency, 0.0f, 1);

                    shakeRotation = glm::rotate(shakeRotation, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
                    shakeRotation = glm::rotate(shakeRotation, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
                }

                renderer.SetCamera({
                    .position = transform.position + shakePosition,
                    .rotation = transform.rotation * shakeRotation,
                    .fov = socket.fov,
                });
            },
            g_entityStore.cameraEntity);

        ForEach<Transform, Renderable>(
            [&](Transform &transform, Renderable &renderable)
            {
                renderer.SubmitRenderJob(StaticRenderJob{
                    .mesh = renderable.mesh,
                    .material = renderable.material,
                    .transform = transform.GetMatrix() * renderable.renderMatrix,
                    .texCoord = renderable.uvOffset,
                    .texSize = renderable.uvScale,
                    .color = renderable.color,
                    .emissiveColor = renderable.emissive,
                });
            },
            GetCurrentPlayerWeapon(),
            g_entityStore.staticEntities,
            g_entityStore.projectiles,
            g_entityStore.enemies);

        // Muzzle flash light
        {
            auto &weaponTransform = GetCurrentPlayerWeapon().GetComponent<Transform>();
            auto &weaponFireAction = GetCurrentPlayerWeapon().GetComponent<WeaponFireAction>();

            if (weaponFireAction.fireTimer.IsRunning())
            {
                float timeSinceStart = 60.0f / weaponFireAction.fireRate - weaponFireAction.fireTimer.GetTimeRemaining();
                pointsLights[0] = {
                    .position = weaponTransform.position + weaponTransform.rotation * glm::vec3(0.0f, 0.0f, -150.0f),
                    .radius = 1500.0f,
                    .color = glm::vec4(0.0f, 10.0f, 20.0f, 1.0f) * glm::mix(0.0f, 1.0f, glm::clamp(1.0f - timeSinceStart / 0.05f, 0.0f, 1.0f)),
                };
            }
        }

        if (g_debugCamera.active)
        {
            renderer.SetCamera({
                .position = g_debugCamera.position,
                .rotation = g_debugCamera.rotation,
                .fov = g_debugCamera.fov,
            });

            auto &physicsWorld = Application::GetPhysicsWorld();
            ForEach<Transform, PhysicsProxy>(
                [&](const Transform &transform, const PhysicsProxy &proxy)
                {
                    const auto &collision = physicsWorld.GetCollisionData(proxy.bodyID);
                    if (collision.has_value())
                    {
                        PhysicsRenderingHelper::RenderCollision(renderer, transform.position, transform.rotation, collision.value());
                    }
                },
                g_entityStore.playerEntity,
                g_entityStore.staticEntities,
                g_entityStore.projectiles,
                g_entityStore.enemies);

            const auto &collision = physicsWorld.GetCollisionData(g_entityStore.floorBodyID);
            if (collision.has_value())
            {
                PhysicsRenderingHelper::RenderCollision(renderer, glm::vec3(0.0f), glm::quat(1.0f, 0.0f, 0.0f, 0.0f), collision.value());
            }
        }

        float totalCorruption = 0.0f;
        for (auto &tile : g_entityStore.tiles)
        {
            totalCorruption += tile.corruption;
        }
        totalCorruption /= g_entityStore.tiles.size();

        float timeSinceStart = Application::GetTimeSinceStart();
        for (int32_t i = 0; i < g_entityStore.tiles.size(); i++)
        {
            glm::vec3 tilePosition = GetTilePosition(i);
            // Hash the x and z values to get a unique value
            Tile &tile = g_entityStore.tiles[i];
            float corruption = tile.corruption;
            bool isCorrupted = corruption > 0.0f;

            uint32_t hash = i ^ 0x5f3759df;
            uint32_t hash2 = hash ^ 0x5f3759df;
            uint32_t hash3 = (hash2 >> 16) ^ (hash2 << 16);

            float time = glm::mix(totalCorruption, 1.0f, 2.0f) * timeSinceStart;

            glm::vec3 corruptColor = glm::mix(
                glm::vec4(c_corruptionBeginColor, 1.0f),
                glm::vec4(c_corruptionEndColor, 1.0f),
                glm::pow(corruption, 4));

            float reset = glm::mix(1.0f, 10.0f, glm::pow(tile.reset, 3.0f));
            renderer.SubmitRenderJob(StaticRenderJob{
                .mesh = GetHandle(MK_ASSET_PATH("models/floor/floor.dat")),
                .material = !isCorrupted ? GetHandle("FloorMaterial") : GetHandle("FloorCorruptedMaterial"),
                .transform = glm::translate(glm::mat4(1.0f), tilePosition) *
                             glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                             glm::rotate(glm::mat4(1.0f), glm::radians((hash % 4) * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
                             glm::scale(glm::mat4(1.0f), glm::vec3(c_tileScale)),
                .texCoord = isCorrupted ? glm::vec2(glm::sin(time * 2.0f + glm::radians(float(hash3 % 360))), glm::cos(time * glm::mix(corruption, 0.25f, 1.0f) * 2.0f + glm::radians(float(hash2 % 360)))) : glm::vec2(0.0f),
                .color = glm::vec4(isCorrupted ? corruptColor : glm::vec3(reset), 1.0f),
                .emissiveColor = glm::vec4(isCorrupted ? corruptColor * 5.0f : glm::vec3(reset), 1.0f),
            });

            // Pick a random point in the tile and spawn a particle
            if constexpr (false)
            {
                // Plus minus half the tile size
                glm::vec3 position = tilePosition + glm::vec3(
                                                        (rand() % 1000) / 1000.0f - 0.5f,
                                                        0.0f,
                                                        (rand() % 1000) / 1000.0f - 0.5f) *
                                                        c_tileSize * c_tileScale;
                // ParticleHelper::SpawnSpark(g_entityStore.particleJobs, position);
            }
        }

        // Render tree
        {
            renderer.SubmitRenderJob(StaticRenderJob{
                .mesh = GetHandle(MK_ASSET_PATH("models/tree/tree.dat")),
                .material = GetHandle("FloorMaterial"),
                .transform = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
            });
        }

        // Render stamina bar
        {
            float stamina = g_entityStore.playerEntity.GetComponent<PlayerMovement>().stamina;

            glm::vec2 position = glm::vec2(0.0f, 0.85f);
            glm::vec2 size = glm::vec2(0.25f, 0.0125f);

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("WhiteSpriteMaterial"),
                .position = position, // + glm::vec2(stamina * 0.1f - 0.1f, 0.0f),
                .size = size * glm::vec2(stamina, 1.0f),
                .zOrder = 1.0f,
            });

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("WhiteSpriteMaterial"),
                .position = position, // + glm::vec2(stamina * 0.1f - 0.1f, 0.0f),
                .size = size,
                .color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f),
            });
        }

        // Render health bar
        {
            float health = g_entityStore.playerEntity.GetComponent<Health>().current;
            float maxHealth = g_entityStore.playerEntity.GetComponent<Health>().max;
            float healthPercentage = health / maxHealth;

            glm::vec2 position = glm::vec2(0.0f, 0.82f);
            glm::vec2 size = glm::vec2(0.25f, 0.0125f);

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("WhiteSpriteMaterial"),
                .position = position, // + glm::vec2(stamina * 0.1f - 0.1f, 0.0f),
                .size = size * glm::vec2(healthPercentage, 1.0f),
                // Light Blue
                .color = glm::vec4(0.0f, 1.0f, 0.8f, 1.0f),
                .zOrder = 1.0f,
            });

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("WhiteSpriteMaterial"),
                .position = position, // + glm::vec2(stamina * 0.1f - 0.1f, 0.0f),
                .size = size,
                .color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f),
            });
        }

        // Render corruption bar
        {
            glm::vec2 position = glm::vec2(0.0f, -0.85f);
            glm::vec2 size = glm::vec2(0.3f, 0.02f);

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("WhiteSpriteMaterial"),
                .position = position, // + glm::vec2(stamina * 0.1f - 0.1f, 0.0f),
                .size = size * glm::vec2(totalCorruption, 1.0f),
                .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
                .zOrder = 1.0f,
            });

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("WhiteSpriteMaterial"),
                .position = position, // + glm::vec2(stamina * 0.1f - 0.1f, 0.0f),
                .size = size,
                .color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f),
            });
        }

        // Render crosshair
        {
            constexpr float c_crosshairSize = 0.005f;

            glm::vec2 position = glm::vec2(0.0f, 0.0f);
            glm::vec2 size = glm::vec2(1.0f, renderer.GetAspectRatio()) * c_crosshairSize;

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("WhiteSpriteMaterial"),
                .position = position,
                .size = size,
                .color = glm::vec4(1.0f),
                .borderRadius = glm::vec4(c_crosshairSize),
                .zOrder = 1.0f,
            });
        }

        // Render blood hud
        {
            // Blood texture over the entire with opacity based on health
            float health = g_entityStore.playerEntity.GetComponent<Health>().current;
            float maxHealth = g_entityStore.playerEntity.GetComponent<Health>().max;
            float healthPercentage = glm::pow(1.0f - health / maxHealth, 3);

            const CameraShake &cameraShake = g_entityStore.cameraEntity.GetComponent<CameraShakes>()[CameraShakeType::Damage];
            float trauma = glm::min(1.0f, glm::pow(1.0f - cameraShake.time / cameraShake.duration, 2.0f) * 2.0f);

            glm::vec2 position = glm::vec2(0.0f, 0.0f);
            glm::vec2 size = glm::vec2(2.0f);

            renderer.SubmitRenderJob(SpriteRenderJob{
                .material = GetHandle("BloodHudMaterial"),
                .position = position,
                .size = size,
                .color = glm::vec4(1.0f, 1.0f, 1.0f, glm::mix(trauma, 1.0f, healthPercentage)),
                .zOrder = 1.0f,
            });
        }

        // Particle emitters
        {
            for (auto &job : g_entityStore.particleJobs)
            {
                renderer.SubmitRenderJob(job);
            }

            g_entityStore.particleJobs.clear();
        }

        {
            // Render
            size_t mem = renderer.GetMemoryUsage();
        }

        renderer.SetPointLights(pointsLights);
        renderer.SetDeltaTime(Application::GetDeltaTime() * Application::GetTimeScale());

// Debug stuff
#if 0

        // Debug point lights
        for (int i = 0; i < pointsLights.size(); i++)
        {
            auto &pointLight = pointsLights[i];
            if (pointLight.radius > 0.0f)
            {
                renderer.SubmitRenderJob(StaticRenderJob{
                    .mesh = GetHandle(MK_ASSET_PATH("models/sphere.dat")),
                    .material = GetHandle("WhiteMaterial"),
                    .transform = glm::translate(glm::mat4(1.0f), pointLight.position) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)),
                    .color = pointLight.color,
                });
            }
        }

        for (int32_t i = 0; i < g_entityStore.tileCorruption.size(); i++)
        {
            glm::vec3 tilePosition = GetTilePosition(i);
            glm::vec4 color = glm::vec4(0.0f, 5.0f, 10.0f, 1.0f) * g_entityStore.tileCorruption[i];
            renderer.SubmitRenderJob(StaticRenderJob{
                .mesh = GetHandle(MK_ASSET_PATH("models/sphere.dat")),
                .material = GetHandle("WhiteMaterial"),
                .transform = glm::translate(glm::mat4(1.0f), tilePosition) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)),
                .color = color,
            });
        }
#endif
    }

    void GameStateImpl::OnExit(GameStates::PlayingState &state)
    {
        // Check time played, if
        float score = Application::GetTimeSinceStart() - g_entityStore.startTime;
        if (score > Application::GetPersistentData().highScore)
        {
            Application::SetPersistentData({.highScore = score});
            Application::WritePersistentData();
        }

        auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
        renderer.SetSkybox(std::nullopt);
        renderer.SetEnvironmentMap(std::nullopt);

        auto &audioSystem = Application::GetAudioSystem();
        audioSystem.StopAllEvents();
        audioSystem.ReleaseAllEvents();

        auto &eventBus = Application::GetEventBus();
        eventBus.Unsubscribe(EventBus::Domain::Scene);

        auto &physicsWorld = Application::GetPhysicsWorld();
        physicsWorld.RemoveAllRigidBodies();
    }

    GameStateMachine::OptionalState GameStateImpl::TransitionTo(const GameStates::PlayingState &state)
    {
        if (g_entityStore.isGameOver)
        {
            return GameStates::MainMenuState{};
        }

        if (state.shouldExitGame)
        {
            return GameStates::MainMenuState{};
        }

        return std::nullopt;
    }

#pragma endregion
}