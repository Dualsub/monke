#pragma once

#include "Core/StateMachine.h"

#include <future>

namespace Vultron
{
    class SceneRenderer;
}

namespace mk
{
    class AudioSystem;
    class InputState;
    class PhysicsWorld;

    namespace GameStates
    {
        struct InitialLoadState
        {
            std::future<void> future;
        };

        struct MainMenuState
        {
            bool shouldEnterGame = false;
        };

        struct LoadingState
        {
            std::future<void> loadingFuture;
        };

        struct PlayingState
        {
            bool shouldExitGame = false;
        };
    }

    using GameStateMachine = StateMachine<GameStates::InitialLoadState, GameStates::MainMenuState, GameStates::LoadingState, GameStates::PlayingState>;

    struct GameStateImpl
    {
        static GameStateMachine::OptionalState TransitionAnyTo(const GameStateMachine::State &state);

#pragma region InitialLoadState

        static void OnEnter(GameStates::InitialLoadState &state);
        static void OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::InitialLoadState &state);
        static void OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::InitialLoadState &state);
        static void OnRender(Vultron::SceneRenderer &renderer, GameStates::InitialLoadState &state);
        static void OnExit(GameStates::InitialLoadState &state);
        static GameStateMachine::OptionalState TransitionTo(const GameStates::InitialLoadState &state);

#pragma endregion

#pragma region MainMenuState

        static void OnEnter(GameStates::MainMenuState &state);
        static void OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::MainMenuState &state);
        static void OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::MainMenuState &state);
        static void OnRender(Vultron::SceneRenderer &renderer, GameStates::MainMenuState &state);
        static void OnExit(GameStates::MainMenuState &state);
        static GameStateMachine::OptionalState TransitionTo(const GameStates::MainMenuState &state);

#pragma endregion

#pragma region LoadingState

        static void OnEnter(GameStates::LoadingState &state);
        static void OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::LoadingState &state);
        static void OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::LoadingState &state);
        static void OnRender(Vultron::SceneRenderer &renderer, GameStates::LoadingState &state);
        static void OnExit(GameStates::LoadingState &state);
        static GameStateMachine::OptionalState TransitionTo(const GameStates::LoadingState &state);

#pragma endregion

#pragma region PlayingState

        static void OnEnter(GameStates::PlayingState &state);
        static void OnUpdate(float dt, AudioSystem &audioSystem, PhysicsWorld &physicsWorld, const InputState &inputState, GameStates::PlayingState &state);
        static void OnFixedUpdate(float dt, uint32_t numSteps, PhysicsWorld &physicsWorld, GameStates::PlayingState &state);
        static void OnRender(Vultron::SceneRenderer &renderer, GameStates::PlayingState &state);
        static void OnExit(GameStates::PlayingState &state);
        static GameStateMachine::OptionalState TransitionTo(const GameStates::PlayingState &state);

#pragma endregion
    };
}