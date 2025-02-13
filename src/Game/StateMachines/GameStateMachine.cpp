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
                    "TestMaterial",
                    {
                        .albedo = renderer.LoadImage(MK_ASSET_PATH("textures/grid.dat"), ImageType::Texture2DArray),
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

                renderer.LoadMesh(MK_ASSET_PATH("models/sphere.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/plane.dat"));
                renderer.LoadMesh(MK_ASSET_PATH("models/gun.dat"));

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
        Entity<Transform, PhysicsProxy, PlayerMovement> playerEntity;
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
            },
            PlayerMovement{
                .dashSpeed = 3000.0f,
                .jumpSpeed = 1000.0f,
            });

        g_entityStore.weaponEntity = CreateEntity(
            Transform{.position = playerPosition, .rotation = playerRotation},
            Renderable{
                .mesh = GetHandle(MK_ASSET_PATH("models/gun.dat")),
                .material = GetHandle("WeaponMaterial"),
                .renderMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            });

        glm::vec3 cameraOffset = glm::vec3(0.0f, 0.0f, 200.0f);
        g_entityStore.cameraEntity = CreateEntity(
            Transform{.position = cameraOffset, .rotation = glm::identity<glm::quat>()},
            CameraSocket{});

        auto spawnBox = [&](const glm::vec3 &position, const glm::vec3 &boxSize)
        {
            g_entityStore.staticEntities.push_back(CreateEntity(
                Transform{.position = position, .rotation = glm::identity<glm::quat>()},
                PhysicsProxy{
                    .bodyID = physicsWorld.CreateRigidBody(
                        {
                            .position = position,
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
                    .material = GetHandle("TestMaterial"),
                    .renderMatrix = glm::scale(glm::mat4(1.0f), boxSize * 0.002f),
                    .uvScale = glm::vec2(64.0f),
                }));
        };

        // Floor
        spawnBox(glm::vec3(0.0f), glm::vec3(250.0f, 10.0f, 250.0f) * 10.0f);
        // Walls
        spawnBox(glm::vec3(0.0f, 50.0f, -250.0f) * 10.0f, glm::vec3(250.0f, 50.0f, 5.0f) * 10.0f);
        spawnBox(glm::vec3(0.0f, 50.0f, 250.0f) * 10.0f, glm::vec3(250.0f, 50.0f, 5.0f) * 10.0f);
        spawnBox(glm::vec3(-250.0f, 50.0f, 0.0f) * 10.0f, glm::vec3(5.0f, 50.0f, 250.0f) * 10.0f);
        spawnBox(glm::vec3(250.0f, 50.0f, 0.0f) * 10.0f, glm::vec3(5.0f, 50.0f, 250.0f) * 10.0f);

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
            PlayerMovement &playerMovement = g_entityStore.playerEntity.GetComponent<PlayerMovement>();
            CameraSocket &cameraSocket = g_entityStore.cameraEntity.GetComponent<CameraSocket>();

            // Rotate around the Y axis
            float yaw = 0.0f;
            float pitch = 0.0f;
            if (glm::length(inputState.lookAxis) > 0.0f)
            {
                yaw = inputState.lookAxis.x;
                glm::quat newRotation = glm::rotate(playerTransform.rotation, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
                newRotation = glm::normalize(newRotation);
                physicsWorld.SetRotation(playerProxy.bodyID, newRotation);

                pitch = inputState.lookAxis.y * 0.5f;
                cameraSocket.pitch = glm::clamp(cameraSocket.pitch + pitch, -glm::half_pi<float>() * 0.99f, glm::half_pi<float>() * 0.99f);
            }

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
                float movementFactor = playerMovement.dashTimer.IsRunning() ? 50.0f : 20.0f;
                movement = glm::mix(playerProxy.currentState.linearVelocity, movement, glm::clamp(movementFactor * dt, 0.0f, 1.0f));
                velocity.x = movement.x;
                velocity.z = movement.z;
            }

            if (inputState.Pressed(InputActionType::Jump))
            {
                playerMovement.wantsToJump = true;
            }

            playerMovement.dashTimer.Tick(dt);
            if (inputState.Pressed(InputActionType::Dash))
            {
                if (playerMovement.dashTimer.HasElapsed())
                {
                    playerMovement.dashDirection = playerTransform.rotation * (glm::length(inputState.movementAxis) > 0.0f ? glm::normalize(glm::vec3(inputState.movementAxis.x, 0.0f, -inputState.movementAxis.y)) : (glm::vec3(0.0f, 0.0f, -1.0f)));
                    playerMovement.dashTimer.Reset(0.2f);
                }
            }

            physicsWorld.SetLinearVelocity(playerProxy.bodyID, velocity);
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
            Transform &playerTransform = g_entityStore.playerEntity.GetComponent<Transform>();
            Transform &cameraTransform = g_entityStore.cameraEntity.GetComponent<Transform>();
            CameraSocket &cameraSocket = g_entityStore.cameraEntity.GetComponent<CameraSocket>();
            Transform &weaponTransform = g_entityStore.weaponEntity.GetComponent<Transform>();

            cameraTransform.position = playerTransform.position + glm::vec3(0.0f, 60.0f, -10.0f);
            cameraTransform.rotation = glm::rotate(playerTransform.rotation, cameraSocket.pitch, glm::vec3(1.0f, 0.0f, 0.0f));

            PhysicsProxy &playerProxy = g_entityStore.playerEntity.GetComponent<PhysicsProxy>();
            glm::vec3 velocity = playerProxy.currentState.linearVelocity;

            glm::mat4 weaponSwayTransform = glm::mat4(1.0f);
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
                weaponSwayTransform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 3.0f * upFactor * forwardFactor));

                float rightFactor = walkFactor * glm::dot(groundDirection, sideDirection);
                weaponSwayTransform *= glm::rotate(glm::mat4(1.0f), rightFactor * glm::radians(-3.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            }

            weaponTransform.SetMatrix(cameraTransform.GetMatrix() * glm::translate(glm::mat4(1.0f), glm::vec3(20.0f, -40.0f, -50.0f)) * weaponSwayTransform);
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
                    .fov = 70.0f,
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
            g_entityStore.staticEntities);

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
                g_entityStore.staticEntities);
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