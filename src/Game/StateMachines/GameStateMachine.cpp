#include "Game/StateMachines/GameStateMachine.h"

#include "Core/Core.h"
#include "Application.h"
#include "UI/UIHelper.h"
#include "Game/Components.h"
#include "Game/Helpers/ParticleHelper.h"
#include "Game/Helpers/PhysicsRenderingHelper.h"

#include <glm/glm.hpp>

#include <ranges>

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

    glm::mat4 GetAnimationTransform(Animation &animation)
    {
        // Return identity matrix if there are not enough keyframes to interpolate
        if (animation.keyframes.size() < 2)
            return glm::mat4(1.0f);

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

        const auto &keyFrame1 = animation.keyframes[keyFrame1Index];
        const auto &keyFrame2 = animation.keyframes[keyFrame2Index];

        // Interpolate position and rotation between the two keyframes
        glm::vec3 position = glm::mix(keyFrame1.position, keyFrame2.position, interpolationFactor);
        glm::quat rotation = glm::slerp(keyFrame1.rotation, keyFrame2.rotation, interpolationFactor);

        // Return the transformation matrix combining translation and rotation
        return glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation);
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
                auto &audioSystem = Application::GetAudioSystem();
                audioSystem.LoadBank(MK_ASSET_PATH("/audio/monke/Build/Desktop/Master.bank"), BankType::Master);
                audioSystem.LoadBank(MK_ASSET_PATH("/audio/monke/Build/Desktop/Master.strings.bank"), BankType::Strings);

                auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());

                // Load assets
                renderer.CreateMaterial<SpriteMaterial>(
                    "WhiteSpriteMaterial",
                    {
                        .texture = GetHandle("white"),
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
                        .albedoColor = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f),
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
                        .albedoColor = glm::vec4(glm::vec3(5.0f, 1.0f, 1.0f), 1.0f),
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
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures/drone_albedo.dat"), ImageType::Texture2DArray, true),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures/drone_normal.dat"), ImageType::Texture2DArray, true),
                        .metallicRoughnessAO = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures/drone_arm.dat"), ImageType::Texture2DArray, true),
                        .emissive = renderer.LoadImage(MK_ASSET_PATH("models/drone/textures/drone_emissive.dat"), ImageType::Texture2DArray, true),
                        .emissiveColor = glm::vec4(glm::vec3(20.0f), 1.0f),
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
                renderer.LoadMesh(MK_ASSET_PATH("models/floor/floor.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/drone/drone.dat"));

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
        if (inputState.Pressed(InputActionType::Attack))
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
        UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "Perfection.", glm::vec2(-0.65f, -0.125f), 5.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), TextAlignment::Left);
        UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "Press any key to start.", glm::vec2(-0.65f, 0.0f), 1.0f, glm::vec4(glm::vec3(1.0f) * blink, 1.0f), TextAlignment::Left);
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

    struct
    {
        Entity<Transform, PhysicsProxy, PlayerMovement, PlayerAnimations, Health> playerEntity;
        Entity<Transform, Renderable, WeaponFireAction, ProjectileBulletEmitter> weaponEntity;
        Entity<Transform, CameraSocket> cameraEntity;
        EntityList<Transform, PhysicsProxy, Renderable> staticEntities;
        EntityList<Transform, PhysicsProxy, Renderable, Lifetime> projectiles;
        EntityList<Transform, PhysicsProxy, Renderable, EnemyType, Health> enemies;

        std::unordered_map<BodyID, float> damageEvents;
    } g_entityStore;

    constexpr int32_t c_tilesPerRow = 20;
    constexpr float c_tileSize = 400.0f;
    constexpr float c_tileScale = 0.5f;

    void CreateEnemy(EnemyType type, glm::vec3 position)
    {
        auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
        auto &physicsWorld = Application::GetPhysicsWorld();

        g_entityStore.enemies.push_back(
            CreateEntity(
                Transform{.position = position, .rotation = glm::identity<glm::quat>()},
                PhysicsProxy{
                    .bodyID = physicsWorld.CreateRigidBody(
                        {
                            .position = position,
                            .rotation = glm::identity<glm::quat>(),
                            .initialVelocity = glm::vec3(0.0f),
                            .mass = 1.0f,
                            .friction = 0.0f,
                            .continuousCollision = false,
                            .shape = CapsuleShape(35.0f, 49.0f),
                            .layer = ObjectLayer::Enemy,
                        },
                        BodyType::Character),
                },
                Renderable{
                    .mesh = GetHandle(MK_ASSET_PATH("models/drone/drone.dat")),
                    .material = GetHandle("DroneMaterial"),
                    .renderMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
                    .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
                },
                EnemyType(type),
                Health{}));
    }

    void GameStateImpl::OnEnter(GameStates::PlayingState &state)
    {
        auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
        auto &physicsWorld = Application::GetPhysicsWorld();

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
                .shootAnimation = Animation{
                    .duration = 12.0f / 60.0f,
                    .loop = false,
                    .keyframes = {
                        {.position = glm::vec3(0.0f, 0.0f, -5.0f) * 0.25f,
                         .rotation = glm::quat(glm::vec3(0.0f, glm::radians(0.0f), 0.0f)), // Initial slight rotation
                         .time = 0.0f},
                        {.position = glm::vec3(0.0f, -1.0f, 15.0f) * 0.25f,
                         .rotation = glm::quat(glm::vec3(0.0f, glm::radians(-0.0f), 0.0f)), // Small recoil rotation forward
                         .time = 2.0f / 60.0f},
                        {.position = glm::vec3(0.0f, 0.0f, 0.0f),
                         .rotation = glm::quat(glm::vec3(0.0f, glm::radians(0.0f), 0.0f)), // Return to neutral
                         .time = 12.0f / 60.0f},
                    },
                },
            },
            Health{});

        g_entityStore.weaponEntity = CreateEntity(
            Transform{.position = playerPosition, .rotation = playerRotation},
            Renderable{
                .mesh = GetHandle(MK_ASSET_PATH("models/gun.dat")),
                .material = GetHandle("WeaponMaterial"),
                .renderMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            },
            WeaponFireAction{
                .automatic = true,
                .fireRate = 600.0f,
            },
            ProjectileBulletEmitter{
                .speed = 5000.0f,
                .damage = 10.0f,
                .lifetime = 1.0f,
            });

        glm::vec3 cameraOffset = glm::vec3(0.0f, 0.0f, 200.0f);
        g_entityStore.cameraEntity = CreateEntity(
            Transform{.position = cameraOffset, .rotation = glm::identity<glm::quat>()},
            CameraSocket{});

        auto spawnBox = [&](const glm::vec3 &position, const glm::vec3 &boxSize, bool isStatic = true)
        {
            g_entityStore.staticEntities.push_back(CreateEntity(
                Transform{.position = position, .rotation = glm::identity<glm::quat>()},
                PhysicsProxy{
                    .bodyID = physicsWorld.CreateRigidBody(
                        {
                            .position = position,
                            .rotation = glm::identity<glm::quat>(),
                            .initialVelocity = glm::vec3(0.0f),
                            .mass = isStatic ? 0.0f : 1.0f,
                            .friction = 0.0f,
                            .continuousCollision = false,
                            .shape = BoxShape(boxSize),
                            .layer = isStatic ? ObjectLayer::NonMoving : ObjectLayer::Moving,
                        },
                        BodyType::Rigidbody),
                },
                Renderable{
                    .mesh = GetHandle(MK_ASSET_PATH("models/plane.dat")),
                    .material = isStatic ? GetHandle("TestMaterial") : GetHandle("WhiteMaterial"),
                    .renderMatrix = glm::scale(glm::mat4(1.0f), boxSize * 0.002f),
                    .color = !isStatic ? glm::vec4(0.5f, 0.5f, 1.0f, 1.0f) : glm::vec4(1.0f),
                    .uvScale = glm::vec2(64.0f),
                }));
        };

        // Floor
        // spawnBox(glm::vec3(0.0f), glm::vec3(250.0f, 10.0f, 250.0f) * 10.0f);
        // // Walls
        // spawnBox(glm::vec3(0.0f, 50.0f, -250.0f) * 10.0f, glm::vec3(250.0f, 50.0f, 5.0f) * 10.0f);
        // spawnBox(glm::vec3(0.0f, 50.0f, 250.0f) * 10.0f, glm::vec3(250.0f, 50.0f, 5.0f) * 10.0f);
        // spawnBox(glm::vec3(-250.0f, 50.0f, 0.0f) * 10.0f, glm::vec3(5.0f, 50.0f, 250.0f) * 10.0f);
        // spawnBox(glm::vec3(250.0f, 50.0f, 0.0f) * 10.0f, glm::vec3(5.0f, 50.0f, 250.0f) * 10.0f);

        // // Spawn some boxes
        // for (int i = 0; i < 10; i++)
        // {
        //     glm::vec3 randomPosition = glm::vec3((rand() % 200) - 100, 50.0f, (rand() % 200) - 100) * 10.0f;
        //     spawnBox(randomPosition, glm::vec3(10.0f, 10.0f, 10.0f) * 10.0f, false);
        // }

        // Create floor collider
        physicsWorld.CreateRigidBody(
            {
                .position = glm::vec3(0.0f, 0.0f, 0.0f),
                .rotation = glm::identity<glm::quat>(),
                .initialVelocity = glm::vec3(0.0f),
                .mass = 0.0f,
                .friction = 0.0f,
                .continuousCollision = false,
                .shape = BoxShape(glm::vec3((c_tilesPerRow + 1) * c_tileSize * c_tileScale / 2.0f, 10.0f, (c_tilesPerRow + 1) * c_tileSize * c_tileScale / 2.0f)),
                .layer = ObjectLayer::NonMoving,
            },
            BodyType::Rigidbody);

        CreateEnemy(EnemyType::Basic, glm::vec3(0.0f, 50.0f, 0.0f));

        renderer.SetParticleAtlasMaterial(GetHandle("ParticleAtlasMaterial"));
        // renderer.SetSkybox(GetHandle("skybox"));
        renderer.SetEnvironmentMap(GetHandle("environment"));
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
                    // audioSystem.PlayEventAtPosition("event:/monke/dash", playerProxy.currentState.position, playerProxy.currentState.linearVelocity);
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
            std::cout << "Camera pitch: " << cameraSocket.pitch << std::endl;
            std::cout << "Camera yaw: " << cameraSocket.yaw << std::endl;
            std::cout << "Camera rotation: " << cameraTransform.rotation.x << ", " << cameraTransform.rotation.y << ", " << cameraTransform.rotation.z << ", " << cameraTransform.rotation.w << std::endl;

            cameraSocket.fov = glm::mix(cameraSocket.fov, playerMovement.dashTimer.IsRunning() ? 75.0f : 70.0f, glm::clamp(40.0f * dt, 0.0f, 1.0f));
        }

        // Camera audio
        {
            Transform &playerTransform = g_entityStore.playerEntity.GetComponent<Transform>();
            PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
            audioSystem.SetListenerState(playerProxy.currentState.position, playerProxy.currentState.rotation, playerProxy.currentState.linearVelocity);
        }

        // Weapon animation
        {
            PlayerAnimations &playerAnimations = g_entityStore.playerEntity.GetComponent<PlayerAnimations>();
            UpdateAnimation(playerAnimations.shootAnimation, dt);
        }

        // Weapon attach to camera
        {
            Transform &playerTransform = g_entityStore.playerEntity.GetComponent<Transform>();
            Transform &cameraTransform = g_entityStore.cameraEntity.GetComponent<Transform>();
            CameraSocket &cameraSocket = g_entityStore.cameraEntity.GetComponent<CameraSocket>();
            Transform &weaponTransform = g_entityStore.weaponEntity.GetComponent<Transform>();
            PlayerMovement &playerMovement = g_entityStore.playerEntity.GetComponent<PlayerMovement>();
            PlayerAnimations &playerAnimations = g_entityStore.playerEntity.GetComponent<PlayerAnimations>();

            PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
            glm::vec3 velocity = playerProxy.currentState.linearVelocity;

            static Transform oldWeaponSwayTransform;
            Transform weaponSwayTransform;
            if (glm::length(velocity) > glm::epsilon<float>())
            {
                glm::vec3 forward = playerTransform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::vec2 groundDirection = glm::normalize(glm::vec2(velocity.x, velocity.z));
                glm::vec2 lookDirection = glm::normalize(glm::vec2(forward.x, forward.z));
                glm::vec2 sideDirection = glm::normalize(glm::vec2(right.x, right.z));

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

            weaponSwayTransform.position = glm::mix(oldWeaponSwayTransform.position, weaponSwayTransform.position, glm::clamp(20.0f * dt, 0.0f, 1.0f));
            weaponSwayTransform.rotation = glm::slerp(oldWeaponSwayTransform.rotation, weaponSwayTransform.rotation, glm::clamp(20.0f * dt, 0.0f, 1.0f));
            oldWeaponSwayTransform = weaponSwayTransform;

            weaponTransform.SetMatrix(cameraTransform.GetMatrix() * glm::translate(glm::mat4(1.0f), glm::vec3(20.0f, -40.0f, -50.0f)) * weaponSwayTransform.GetMatrix() * GetAnimationTransform(playerAnimations.shootAnimation));
        }

        // Player fire
        {
            WeaponFireAction &fireAction = g_entityStore.weaponEntity.GetComponent<WeaponFireAction>();
            {
                fireAction.fireTimer.Tick(dt);
                bool wantsToFire = fireAction.automatic ? inputState.Down(InputActionType::Attack) : inputState.Pressed(InputActionType::Attack);
                fireAction.fire = wantsToFire && fireAction.fireTimer.HasElapsed();
                if (fireAction.fire)
                {
                    fireAction.fireTimer.Reset(60.0f / fireAction.fireRate);
                }
            }

            if (fireAction.fire)
            {
                ProjectileBulletEmitter &emitter = g_entityStore.weaponEntity.GetComponent<ProjectileBulletEmitter>();
                Transform &weaponTransform = g_entityStore.weaponEntity.GetComponent<Transform>();
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
                        .gravityFactor = 0.0f,
                        .shape = SphereShape(scale * 100.0f),
                        .layer = ObjectLayer::PlayerProjectile,
                    },
                    BodyType::Rigidbody);
                physicsWorld.RegisterContactListener(bodyId);
                auto currentState = physicsWorld.GetRigidBodyState(bodyId);
                auto previousState = currentState;

                g_entityStore.projectiles.push_back(CreateEntity(
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

                g_entityStore.playerEntity.GetComponent<PlayerAnimations>().shootAnimation.time = 0.0f;
            }
        }

        ForEach<Transform, PhysicsProxy, EnemyType>(
            [&](Transform &transform, PhysicsProxy &proxy, EnemyType &type)
            {
                if (type == EnemyType::Basic)
                {
                    glm::vec3 enemyToPlayer = g_entityStore.playerEntity.GetComponent<Transform>().position - transform.position;
                    if (glm::length(enemyToPlayer) < 200.0f)
                    {
                        return;
                    }

                    enemyToPlayer.y = 0.0f;
                    physicsWorld.SetLinearVelocity(
                        proxy.bodyID,
                        glm::normalize(enemyToPlayer) * 300.0f);

                    glm::quat rotation = glm::quatLookAt(glm::normalize(enemyToPlayer), glm::vec3(0.0f, 1.0f, 0.0f));
                    physicsWorld.SetRotation(
                        proxy.bodyID,
                        rotation);
                }
            },
            g_entityStore.enemies);

        // Damage system
        ForEach<PhysicsProxy, Health>(
            [&](PhysicsProxy &proxy, Health &health)
            {
                if (g_entityStore.damageEvents.find(proxy.bodyID) != g_entityStore.damageEvents.end())
                {
                    health.current -= g_entityStore.damageEvents[proxy.bodyID];
                    g_entityStore.damageEvents.erase(proxy.bodyID);
                }

                if (health.current <= 0.0f)
                {
                    physicsWorld.RemoveRigidBody(proxy.bodyID);
                }
            },
            g_entityStore.playerEntity,
            g_entityStore.enemies);

        Filter<PhysicsProxy, Health>(
            [&](PhysicsProxy &proxy, Health &health) -> bool
            {
                return health.current <= 0.0f;
            },
            g_entityStore.enemies);
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

        // Projectiles removal system
        {
            Filter<Transform, PhysicsProxy, Renderable, Lifetime>(
                [&](Transform &transform, PhysicsProxy &proxy, Renderable &renderable, Lifetime &lifetime) -> bool
                {
                    const auto &contacts = physicsWorld.GetContacts(proxy.bodyID);

                    if (!contacts.empty())
                    {
                        for (const auto &contact : contacts)
                        {
                            g_entityStore.damageEvents[contact.body] += 10.0f;
                        }

                        physicsWorld.RemoveRigidBody(proxy.bodyID);
                        return true;
                    }

                    if (transform.position.y < -1000.0f)
                    {
                        physicsWorld.RemoveRigidBody(proxy.bodyID);
                        return true;
                    }

                    if (lifetime.timer.Tick(dt))
                    {
                        physicsWorld.RemoveRigidBody(proxy.bodyID);
                        return true;
                    }

                    return false;
                },
                g_entityStore.projectiles);
        }
    }

    void GameStateImpl::OnRender(Vultron::SceneRenderer &renderer, GameStates::PlayingState &state)
    {
        std::array<PointLightData, 4> pointsLights = {};

        ForEach<Transform, CameraSocket>(
            [&](Transform &transform, CameraSocket &socket)
            {
                renderer.SetCamera({
                    .position = transform.position,
                    .rotation = transform.rotation,
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
                });
            },
            g_entityStore.weaponEntity,
            g_entityStore.staticEntities,
            g_entityStore.projectiles,
            g_entityStore.enemies);

        // Muzzle flash light
        {
            auto &weaponTransform = g_entityStore.weaponEntity.GetComponent<Transform>();
            auto &weaponFireAction = g_entityStore.weaponEntity.GetComponent<WeaponFireAction>();

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
        }

        float corruptedAmount = 0.0f;
        // for (int32_t x = -c_tilesPerRow / 2; x < c_tilesPerRow / 2; x++)
        // {
        //     for (int32_t z = -c_tilesPerRow / 2; z < c_tilesPerRow / 2; z++)
        //     {
        //         uint32_t hash = (x * 73856093) ^ (z * 19349663);
        //         uint32_t hash2 = hash ^ 0x5f3759df;
        //         uint32_t hash3 = (hash2 >> 16) ^ (hash2 << 16);
        //         bool isCorrupted = hash3 % 5 == 0;
        //         if (isCorrupted)
        //         {
        //             corruptedAmount += 1.0f;
        //         }
        //     }
        // }
        // corruptedAmount /= c_tilesPerRow * c_tilesPerRow;

        float timeSinceStart = Application::GetTimeSinceStart();
        for (int32_t x = -c_tilesPerRow / 2; x < c_tilesPerRow / 2; x++)
        {
            for (int32_t z = -c_tilesPerRow / 2; z < c_tilesPerRow / 2; z++)
            {
                // Hash the x and z values to get a unique value
                uint32_t hash = (x * 73856093) ^ (z * 19349663);
                uint32_t hash2 = hash ^ 0x5f3759df;
                uint32_t hash3 = (hash2 >> 16) ^ (hash2 << 16);
                bool isCorrupted = hash3 % 5 == 0;

                float time = glm::mix(corruptedAmount, 1.0f, 2.0f) * timeSinceStart;

                glm::vec3 tilePosition = glm::vec3(x * c_tileSize * c_tileScale, glm::sin(glm::radians(float(hash2 % 360)) + time) * 10.0f * corruptedAmount, z * c_tileSize * c_tileScale);

                renderer.SubmitRenderJob(StaticRenderJob{
                    .mesh = GetHandle(MK_ASSET_PATH("models/floor/floor.dat")),
                    .material = !isCorrupted ? GetHandle("FloorMaterial") : GetHandle("FloorCorruptedMaterial"),
                    .transform = glm::translate(glm::mat4(1.0f), tilePosition) *
                                 glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
                                 glm::rotate(glm::mat4(1.0f), glm::radians((hash % 4) * 90.0f), glm::vec3(0.0f, 0.0f, 1.0f)) *
                                 glm::scale(glm::mat4(1.0f), glm::vec3(c_tileScale)),
                    .texCoord = isCorrupted ? glm::vec2(glm::sin(time * 2.0f + glm::radians(float(hash3 % 360))), glm::cos(time * 2.0f + glm::radians(float(hash2 % 360)))) : glm::vec2(0.0f),
                });

                // Pick random position in tile
            }
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

        // Debug point lights
        if (false)
        {
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
        }

        renderer.SetPointLights(pointsLights);
        renderer.SetDeltaTime(Application::GetDeltaTime());
    }

    void GameStateImpl::OnExit(GameStates::PlayingState &state)
    {
        auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());
        renderer.SetSkybox(std::nullopt);
        renderer.SetEnvironmentMap(std::nullopt);

        auto &eventBus = Application::GetEventBus();
        eventBus.Unsubscribe(EventBus::Domain::Scene);
    }

    GameStateMachine::OptionalState GameStateImpl::TransitionTo(const GameStates::PlayingState &state)
    {
        if (state.shouldExitGame)
        {
            return GameStates::MainMenuState{};
        }

        return std::nullopt;
    }

#pragma endregion
}