#pragma once

#include <variant>
#include <optional>

namespace ACS
{
    template <typename... States>
    class StateMachine
    {
    public:
        using State = std::variant<States...>;
        using OptionalState = std::optional<State>;

    private:
        State m_state = {};

    public:
        StateMachine() = default;
        ~StateMachine() = default;

        template <typename F>
        void Visit(F &&f)
        {
            std::visit(f, m_state);
        }

        template <typename UpdateImpl, typename... Args>
        void Update(Args &&...args)
        {
            std::visit([this, &args...](auto &state)
                       { UpdateImpl::Update(args..., state); },
                       m_state);
        }

        template <typename TransitionImpl, typename... Args>
        void Transition(Args &&...args)
        {
            OptionalState nextState = TransitionImpl::TransitionAnyTo(args..., m_state);

            if (!nextState.has_value())
            {
                nextState = std::visit([this, &args...](auto &state)
                                       { return TransitionImpl::TransitionTo(args..., state); },
                                       m_state);
            }

            if (nextState.has_value())
            {
                std::visit([this, &args...](auto &state)
                           { TransitionImpl::OnExit(args..., state); },
                           m_state);

                m_state = std::move(nextState.value());

                std::visit([this, &args...](auto &state)
                           { TransitionImpl::OnEnter(args..., state); },
                           m_state);
            }
        }

        template <typename TransitionImpl, typename StateType, typename... Args>
        void SetState(StateType &&state, Args &&...args)
        {
            std::visit([this, &args...](auto &state)
                       { TransitionImpl::OnExit(args..., state); },
                       m_state);

            m_state = std::forward<StateType>(state);

            std::visit([this, &args...](auto &state)
                       { TransitionImpl::OnEnter(args..., state); },
                       m_state);
        }

        template <typename StateType>
        StateType &GetState()
        {
            return std::get<StateType>(m_state);
        }

        template <typename StateType>
        const StateType &GetState() const
        {
            return std::get<StateType>(m_state);
        }

        template <typename StateType>
        bool HasState() const
        {
            return std::holds_alternative<StateType>(m_state);
        }
    };
};