#pragma once

#include "Core/EnumArray.h"

#include <vector>
#include <unordered_map>
#include <typeindex>
#include <variant>
#include <functional>

namespace ACS
{
    class EventBus
    {
    public:
        enum class Domain
        {
            // From lowest level to highest level
            Engine,
            Game,
            Scene,

            // Keep this last
            Count,
            None,
        };

        using EventID = std::type_index;
        inline const static EventID c_invalidEventId = typeid(void);

        template <typename Context, typename T>
        void Subscribe(const std::function<void(Context &, const T &)> &callback, Domain domain = Domain::Scene)
        {
            static_assert(std::is_trivially_copyable_v<T>);

            EventID id = typeid(T);
            m_subscribers[domain][id].push_back([callback](void *context, const void *event)
                                                { callback(*reinterpret_cast<Context *>(context), *reinterpret_cast<const T *>(event)); });
        }

        void Unsubscribe(Domain domain = Domain::Scene)
        {
            m_subscribers[domain].clear();
        }

        template <typename Context>
        void SetContext(Domain domain, Context &context)
        {
            m_context[domain] = reinterpret_cast<void *>(&context);
        }

        void ClearContext(Domain domain)
        {
            m_context[domain] = nullptr;
        }

        template <typename T>
        void Dispatch(const T &event, Domain domain = Domain::Scene)
        {
            static_assert(std::is_trivially_copyable_v<T>);

            EventID id = typeid(T);
            auto it = m_subscribers[domain].find(id);
            if (it != m_subscribers[domain].end())
            {
                for (const Callback &callback : it->second)
                {
                    callback(m_context[domain], &event);
                }
            }
        }

        template <typename T>
        void QueueEvent(const T &event)
        {
            static_assert(std::is_trivially_copyable_v<T>);

            constexpr std::size_t alignment = alignof(EventHeader) > alignof(T) ? alignof(EventHeader) : alignof(T);

            EventHeader header = {};
            header.id = typeid(T);
            header.size = sizeof(T);
            header.paddedSize = AlignTo(sizeof(T), alignment);

            std::size_t oldOffset = m_eventBufferOffset;
            if (m_eventBufferOffset + sizeof(EventHeader) + header.paddedSize > m_eventBuffer.size())
            {
                m_eventBuffer.resize(m_eventBuffer.size() + sizeof(EventHeader) + header.paddedSize);
            }

            std::memcpy(&m_eventBuffer[m_eventBufferOffset], &header, sizeof(EventHeader));
            std::memcpy(&m_eventBuffer[m_eventBufferOffset + sizeof(EventHeader)], &event, sizeof(T));

            m_eventBufferOffset += sizeof(EventHeader) + header.paddedSize;
        }

        void ProcessEvents(Domain domain = Domain::Scene)
        {
            void *context = m_context[domain];

            size_t offset = 0;
            while (offset < m_eventBufferOffset)
            {
                const EventHeader *header = reinterpret_cast<const EventHeader *>(m_eventBuffer.data() + offset);

                std::size_t dataOffset = offset + sizeof(EventHeader);

                auto it = m_subscribers[domain].find(header->id);
                if (it != m_subscribers[domain].end())
                {
                    const void *event = static_cast<const void *>(m_eventBuffer.data() + dataOffset);
                    for (const Callback &callback : it->second)
                    {
                        callback(context, event);
                    }
                }

                offset += sizeof(EventHeader) + header->paddedSize;
            }
        }

        void ClearEvents()
        {
            m_eventBufferOffset = 0;
        }

        void Update()
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(Domain::Count); i++)
            {
                ProcessEvents(static_cast<Domain>(i));
            }

            ClearEvents();
        }

    private:
        using Callback = std::function<void(void *, const void *)>;

        using SubscriberMap = std::unordered_map<EventID, std::vector<Callback>>;

        EnumArray<Domain, SubscriberMap> m_subscribers;
        EnumArray<Domain, void *> m_context;

        struct EventHeader
        {
            EventID id = c_invalidEventId;
            uint32_t size = 0;
            uint32_t paddedSize = 0;
        };

        std::vector<char> m_eventBuffer;
        std::size_t m_eventBufferOffset = 0;

        inline static std::size_t AlignTo(std::size_t size, std::size_t alignment)
        {
            return (size + alignment - 1) & ~(alignment - 1);
        }
    };
}