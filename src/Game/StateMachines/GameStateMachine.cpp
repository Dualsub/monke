#include "Game/StateMachines/GameStateMachine.h"

#include "Core/Core.h"
#include "Application.h"
#include "UI/UIHelper.h"
#include "Game/Components.h"
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
                auto &renderer = const_cast<SceneRenderer &>(Application::GetRenderer());

                // Load assets
                renderer.CreateMaterial<SpriteMaterial>(
                    "WhiteSpriteMaterial",
                    {
                        .texture = GetHandle("white"),
                    });

                renderer.CreateMaterial<PBRMaterial>(
                    "WeaponMaterial",
                    {
                        .albedo = GetHandle("white"),
                        .normal = renderer.LoadImage(MK_ASSET_PATH("textures/normal.dat"), ImageType::Texture2DArray),
                        .metallicRoughnessAO = GetHandle("white"),
                    });

                renderer.LoadMesh(MK_ASSET_PATH("models/sphere.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/plane.dat"));

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
        UIHelper::RenderText(renderer, c_fontAtlasHandle, c_fontMaterialHandle, "Main Menu", glm::vec2(0.0f, 0.0f), 1.0f, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
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
        Entity<Transform, PhysicsProxy> playerEntity;
        Entity<Transform, Renderable> weaponEntity;
        Entity<Transform, CameraSocket> cameraEntity;
        std::vector<Entity<Transform, PhysicsProxy, Renderable>> staticEntities;
    } g_entityStore;

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
            });

        glm::vec3 weaponOffset = glm::vec3(0.0f, 0.0f, 0.0f);
        g_entityStore.weaponEntity = CreateEntity(
            Transform{.position = playerPosition + weaponOffset, .rotation = playerRotation},
            Renderable{
                .mesh = GetHandle(MK_ASSET_PATH("models/sphere.dat")),
                .material = GetHandle("WeaponMaterial"),
                .renderMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(50.0f, -20.0f, -100.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.1f)),
            });

        glm::vec3 cameraOffset = glm::vec3(0.0f, 0.0f, 200.0f);
        g_entityStore.cameraEntity = CreateEntity(
            Transform{.position = cameraOffset, .rotation = glm::identity<glm::quat>()},
            CameraSocket{});

        {
            glm::vec3 boxSize = glm::vec3(250.0f, 10.0f, 250.0f);
            g_entityStore.staticEntities.push_back(CreateEntity(
                Transform{.position = glm::vec3(0.0f, 0.0f, 0.0f), .rotation = glm::identity<glm::quat>()},
                PhysicsProxy{
                    .bodyID = physicsWorld.CreateRigidBody(
                        {
                            .position = glm::vec3(0.0f, 0.0f, 0.0f),
                            .rotation = glm::identity<glm::quat>(),
                            .initialVelocity = glm::vec3(0.0f),
                            .mass = 0.0f,
                            .friction = 0.0f,
                            .continuousCollision = false,
                            .shape = BoxShape(boxSize),
                            .layer = ObjectLayer::NonMoving,
                        },
                        BodyType::Rigidbody),
                },
                Renderable{
                    .mesh = GetHandle(MK_ASSET_PATH("models/plane.dat")),
                    .material = GetHandle("WeaponMaterial"),
                    .renderMatrix = glm::scale(glm::mat4(1.0f), boxSize * 0.002f),
                }));
        }

        renderer.SetSkybox(GetHandle("skybox"));
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

            // Rotate around the Y axis
            float yaw = 0.0f;
            if (glm::length(inputState.lookAxis) > 0.0f)
            {
                yaw = inputState.lookAxis.x;
                glm::quat newRotation = glm::rotate(playerTransform.rotation, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
                physicsWorld.SetRotation(playerProxy.bodyID, newRotation);
            }

            CharacterGroundState groundState = physicsWorld.GetCharacterGroundState(playerProxy.bodyID);
            if (groundState == CharacterGroundState::OnGround)
            {
                // Move forward
                float speed = 100.0f;
                glm::vec3 forward = playerTransform.rotation * glm::vec3(0.0f, 0.0f, -1.0f);
                glm::vec3 alongForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
                glm::vec3 right = glm::cross(alongForward, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::vec3 movement = (alongForward * inputState.movementAxis.y + right * inputState.movementAxis.x) * speed;
                physicsWorld.SetLinearVelocity(playerProxy.bodyID, movement);
            }
        }

        // Physics interpolation system
        {
            float alpha = glm::clamp(Application::GetTimeSincePhysics() / c_fixedUpdateInterval, 0.0f, 1.0f);
            ForEach<Transform, PhysicsProxy>(
                [&](Transform &transform, PhysicsProxy &proxy)
                {
                    transform.position = glm::mix(proxy.previousState.position, proxy.currentState.position, alpha);
                    transform.rotation = glm::slerp(proxy.previousState.rotation, proxy.currentState.rotation, alpha);
                },
                g_entityStore.playerEntity, g_entityStore.staticEntities);
        }

        // Attack to player system
        {
            ForEach<Transform>(
                [&](Transform &transform)
                {
                    transform.position = g_entityStore.playerEntity.GetComponent<Transform>().position;
                    transform.rotation = g_entityStore.playerEntity.GetComponent<Transform>().rotation;
                },
                g_entityStore.weaponEntity, g_entityStore.cameraEntity);
        }
    }

    void GameStateImpl::OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::PlayingState &state)
    {
        if (g_debugCamera.active)
        {
            return;
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
                g_entityStore.staticEntities);
        }
    }

    void GameStateImpl::OnRender(Vultron::SceneRenderer &renderer, GameStates::PlayingState &state)
    {
        ForEach<Transform, CameraSocket>(
            [&](Transform &transform, CameraSocket &socket)
            {
                renderer.SetCamera({
                    .position = transform.position,
                    .rotation = transform.rotation,
                    .fov = 60.0f,
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
                    .color = renderable.color,
                });
            },
            g_entityStore.weaponEntity,
            g_entityStore.staticEntities);

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
            g_entityStore.staticEntities);

        if (g_debugCamera.active)
        {
            renderer.SetCamera({
                .position = g_debugCamera.position,
                .rotation = g_debugCamera.rotation,
                .fov = g_debugCamera.fov,
            });
        }

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