#pragma once

#include <tuple>
#include <vector>
#include <bitset>
#include <unordered_map>
#include <map>
#include <array>
#include <cassert>
#include <cstddef>
#include <atomic>
#include <limits>
#include <cstring>

namespace mk
{
    //============================================================
    // Allocator and CustomAllocator
    //============================================================

    struct MemoryAllocator
    {
        inline static std::atomic<std::size_t> total_allocated_memory = 0;

        static void *allocate(std::size_t size) noexcept
        {
            if (size == 0)
                return nullptr;

            void *ptr = std::malloc(size);
            if (ptr)
            {
                total_allocated_memory += size; // Update total allocated memory
            }
            return ptr;
        }

        static void deallocate(void *ptr, std::size_t size) noexcept
        {
            if (ptr)
            {
                total_allocated_memory -= size; // Update total allocated memory
                std::free(ptr);
            }
        }

        static std::size_t get_total_allocated_memory() noexcept
        {
            return total_allocated_memory.load();
        }
    };

    template <typename T>
    struct CustomAllocator
    {
        using value_type = T;

        CustomAllocator() = default;

        template <typename U>
        CustomAllocator(const CustomAllocator<U> &) noexcept {}

        // Allocate memory for n objects of type T
        T *allocate(std::size_t n) noexcept
        {
            if (n == 0)
                return nullptr;

            if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            {
                return nullptr; // Handle allocation failure by returning nullptr
            }

            void *ptr = MemoryAllocator::allocate(n * sizeof(T));
            return static_cast<T *>(ptr);
        }

        // Deallocate memory for n objects of type T
        void deallocate(T *p, std::size_t n) noexcept
        {
            MemoryAllocator::deallocate(static_cast<void *>(p), n * sizeof(T));
        }

        template <typename U, typename... Args>
        void construct(U *p, Args &&...args)
        {
            new (p) U(std::forward<Args>(args)...);
        }

        template <typename U>
        void destroy(U *p) noexcept
        {
            p->~U();
        }

        template <typename U>
        bool operator==(const CustomAllocator<U> &) const { return true; }

        template <typename U>
        bool operator!=(const CustomAllocator<U> &) const { return false; }
    };

    //============================================================
    // Aliases
    //============================================================
    template <typename T>
    using Vector = std::vector<T, CustomAllocator<T>>;

    template <typename Key, typename Value>
    using UnorderedMap = std::unordered_map<
        Key,
        Value,
        std::hash<Key>,
        std::equal_to<Key>,
        CustomAllocator<std::pair<const Key, Value>>>;

    template <typename Key, typename Value, typename Hash, typename Equal>
    using UnorderedMapCustom = std::unordered_map<
        Key,
        Value,
        Hash,
        Equal,
        CustomAllocator<std::pair<const Key, Value>>>;

    //============================================================
    // ECS Definitions
    //============================================================

    constexpr uint32_t c_maxComponents = 32u;

    using Entity = uint32_t;
    using ComponentBitset = std::bitset<c_maxComponents>;
    using ComponentId = uint32_t;
    using DataBuffer = std::vector<uint8_t>;

    //============================================================
    // Hash/Equality for ComponentBitset
    //============================================================
    struct ComponentBitsetHash
    {
        size_t operator()(const ComponentBitset &bitset) const
        {
            // This approach works if c_maxComponents <= 32.
            // If c_maxComponents were larger, you'd need a more robust hash.
            return std::hash<unsigned long>()(bitset.to_ulong());
        }
    };

    struct ComponentBitsetEqual
    {
        bool operator()(const ComponentBitset &lhs, const ComponentBitset &rhs) const
        {
            return lhs == rhs;
        }
    };

    //============================================================
    // Archetype and EntityRecord
    //============================================================
    struct Archetype
    {
        std::array<int32_t, c_maxComponents> componentOffsets = {-1};
        size_t entitySize = 0;
        Vector<Entity> entities;
        DataBuffer dataBuffer;
    };

    struct EntityRecord
    {
        uint32_t entityIndex = 0;
        ComponentBitset componentBitset;
    };

    using ArchetypeMap = UnorderedMapCustom<ComponentBitset, Archetype, ComponentBitsetHash, ComponentBitsetEqual>;
    using EntityMap = UnorderedMap<Entity, EntityRecord>;
    using ComponentBitsetList = Vector<ComponentBitset>;
    using ComponentBitsetMap = UnorderedMap<ComponentId, ComponentBitsetList>;

    //============================================================
    // EntityStore
    //============================================================
    class EntityStore
    {
    private:
        inline static ComponentId s_nextComponentId = 0;
        inline static Entity s_nextEntity = 0;

        ArchetypeMap m_archetypes;
        EntityMap m_entities;
        ComponentBitsetMap m_componentBitsets;

        template <typename T>
        ComponentId GetComponentId()
        {
            static ComponentId id = s_nextComponentId++;
            assert(id < c_maxComponents && "Component limit reached");
            return id;
        }

        template <typename... Components>
        ComponentBitset GetComponentBitset()
        {
            ComponentBitset componentBitset;
            ((componentBitset.set(GetComponentId<Components>())), ...);
            return componentBitset;
        }

    public:
        EntityStore() = default;
        ~EntityStore() = default;

        template <typename... Components>
        Entity CreateEntity(Components... components)
        {
            // (Optional) Enforce trivially copyable components:
            // (void)std::initializer_list<int>{ (static_assert(std::is_trivially_copyable<Components>::value, "Component must be trivially copyable"), 0)... };

            ComponentBitset componentBitset = GetComponentBitset<Components...>();

            size_t entitySize = 0;
            ((entitySize += sizeof(Components)), ...);

            auto it = m_archetypes.find(componentBitset);
            if (it == m_archetypes.end())
            {
                std::array<int32_t, c_maxComponents> componentOffsets = {-1};
                int32_t offset = 0;
                ((componentOffsets[GetComponentId<Components>()] = offset, offset += sizeof(Components)), ...);

                Archetype archetype{
                    .componentOffsets = componentOffsets,
                    .entitySize = entitySize,
                    .entities = {},
                    .dataBuffer = {}};

                m_archetypes.emplace(componentBitset, std::move(archetype));

                std::array<ComponentId, sizeof...(Components)> componentIds = {GetComponentId<Components>()...};
                for (ComponentId componentId : componentIds)
                {
                    m_componentBitsets[componentId].push_back(componentBitset);
                }
            }

            Archetype &archetype = m_archetypes[componentBitset];
            uint32_t entityIndex = static_cast<uint32_t>(archetype.dataBuffer.size() / entitySize);

            size_t writePos = archetype.dataBuffer.size();
            archetype.dataBuffer.resize(writePos + entitySize);

            // Copy each component into the data buffer at the correct offset
            ([&]
             {
                const auto& comp = components;
                size_t offset = archetype.componentOffsets[GetComponentId<Components>()];
                std::memcpy(archetype.dataBuffer.data() + writePos + offset, &comp, sizeof(Components)); }(), ...);

            EntityRecord entityRecord{
                .entityIndex = entityIndex,
                .componentBitset = componentBitset};

            Entity entity = s_nextEntity++;
            m_entities.emplace(entity, entityRecord);
            archetype.entities.push_back(entity);

            return entity;
        }

        void DestroyEntity(Entity entity)
        {
            // Swap-and-pop removal
            EntityRecord &entityRecord = m_entities[entity];
            Archetype &archetype = m_archetypes[entityRecord.componentBitset];

            if (archetype.entities.empty())
            {
                // Nothing to destroy
                return;
            }

            if (archetype.entities.size() == 1)
            {
                // If there's only one entity, remove it and clear the archetype data
                m_entities.erase(entity);
                archetype.entities.clear();
                archetype.dataBuffer.clear();
                return;
            }

            size_t entityIndex = entityRecord.entityIndex;
            size_t entitySize = archetype.entitySize;
            size_t lastEntityIndex = archetype.entities.size() - 1;
            Entity lastEntity = archetype.entities[lastEntityIndex];

            // Move last entity into the vacated slot
            archetype.entities[entityIndex] = lastEntity;
            m_entities[lastEntity].entityIndex = static_cast<uint32_t>(entityIndex);

            // Remove the destroyed entity from the map
            archetype.entities.pop_back();
            m_entities.erase(entity);

            // Move last entity's data into the position of the destroyed entity
            if (entityIndex != lastEntityIndex)
            {
                uint8_t *entityData = archetype.dataBuffer.data() + entityIndex * entitySize;
                uint8_t *lastEntityData = archetype.dataBuffer.data() + lastEntityIndex * entitySize;
                std::memcpy(entityData, lastEntityData, entitySize);

                // Shrink the data buffer
                archetype.dataBuffer.resize(archetype.dataBuffer.size() - entitySize);
            }
        }

        template <typename T>
        T &GetComponent(Entity entity)
        {
            EntityRecord &entityRecord = m_entities[entity];
            Archetype &archetype = m_archetypes[entityRecord.componentBitset];
            size_t entityIndex = entityRecord.entityIndex;
            size_t entitySize = archetype.entitySize;
            size_t componentOffset = archetype.componentOffsets[GetComponentId<T>()];
            return *reinterpret_cast<T *>(archetype.dataBuffer.data() + entityIndex * entitySize + componentOffset);
        }

        template <typename... Components>
        void ForEach(auto &&func)
        {
            ComponentBitset componentBitset = GetComponentBitset<Components...>();
            // Retrieve the component ID of the last component type
            using LastComponent = std::tuple_element_t<sizeof...(Components) - 1, std::tuple<Components...>>;
            ComponentId lastComponentId = GetComponentId<LastComponent>();

            auto it = m_componentBitsets.find(lastComponentId);
            if (it == m_componentBitsets.end())
            {
                return;
            }

            for (const ComponentBitset &bitset : it->second)
            {
                // Check if this archetype includes all requested components
                if ((bitset & componentBitset) == componentBitset)
                {
                    Archetype &archetype = m_archetypes[bitset];
                    size_t entitySize = archetype.entitySize;
                    size_t entityCount = archetype.dataBuffer.size() / entitySize;

                    for (size_t i = 0; i < entityCount; ++i)
                    {
                        uint8_t *basePointer = archetype.dataBuffer.data() + i * entitySize;
                        func(
                            *(reinterpret_cast<Components *>(basePointer + archetype.componentOffsets[GetComponentId<Components>()]))...);
                    }
                }
            }
        }

        void ClearEntities()
        {
            // Clear only the entities, not the archetypes or component bitsets
            m_entities.clear();
            s_nextEntity = 0;
            for (auto &[_, archetype] : m_archetypes)
            {
                archetype.entities.clear();
                archetype.dataBuffer.clear();
            }

            // Clear the component bitsets
            for (auto &[_, bitsetList] : m_componentBitsets)
            {
                bitsetList.clear();
            }
        }
    };

    //============================================================
    // Tests
    //============================================================

    // // Example trivial component
    // struct Position
    // {
    //     float x, y;
    // };

    // struct Velocity
    // {
    //     float vx, vy;
    // };

    // struct Health
    // {
    //     int hp;
    // };

    // // Test: Create entities with a single component and retrieve it
    // void TestSingleComponent()
    // {
    //     mk::EntityStore store;
    //     mk::Entity e1 = store.CreateEntity(Position{10.0f, 20.0f});
    //     mk::Entity e2 = store.CreateEntity(Position{30.0f, 40.0f});

    //     // Retrieve and check components
    //     Position &p1 = store.GetComponent<Position>(e1);
    //     Position &p2 = store.GetComponent<Position>(e2);
    //     assert(p1.x == 10.0f && p1.y == 20.0f);
    //     assert(p2.x == 30.0f && p2.y == 40.0f);

    //     std::cout << "TestSingleComponent passed.\n";
    // }

    // // Test: Create entities with multiple components and retrieve them
    // void TestMultipleComponents()
    // {
    //     mk::EntityStore store;
    //     mk::Entity e = store.CreateEntity(
    //         Position{5.0f, 15.0f},
    //         Velocity{1.0f, 0.5f},
    //         Health{100});

    //     Position &pos = store.GetComponent<Position>(e);
    //     Velocity &vel = store.GetComponent<Velocity>(e);
    //     Health &health = store.GetComponent<Health>(e);

    //     assert(pos.x == 5.0f && pos.y == 15.0f);
    //     assert(vel.vx == 1.0f && vel.vy == 0.5f);
    //     assert(health.hp == 100);

    //     std::cout << "TestMultipleComponents passed.\n";
    // }

    // // Test: Destroying entities
    // void TestDestroyEntity()
    // {
    //     mk::EntityStore store;
    //     mk::Entity e1 = store.CreateEntity(Position{1.0f, 2.0f});
    //     mk::Entity e2 = store.CreateEntity(Position{3.0f, 4.0f});
    //     mk::Entity e3 = store.CreateEntity(Position{5.0f, 6.0f});

    //     // Ensure initial state
    //     assert(store.GetComponent<Position>(e1).x == 1.0f);

    //     // Destroy e2
    //     store.DestroyEntity(e2);
    //     // e3 might have moved into e2's slot depending on internal logic.

    //     // Check that e1 is still valid
    //     assert(store.GetComponent<Position>(e1).x == 1.0f); // Should still be accessible

    //     // Destroy e1
    //     store.DestroyEntity(e1);
    //     // Destroy e3
    //     store.DestroyEntity(e3);

    //     // At this point, the archetype may be empty
    //     std::cout << "TestDestroyEntity passed.\n";
    // }

    // // Test: ForEach iteration
    // void TestForEach()
    // {
    //     mk::EntityStore store;
    //     store.CreateEntity(Position{1.0f, 2.0f}, Velocity{0.1f, 0.2f});
    //     store.CreateEntity(Position{3.0f, 4.0f}, Velocity{0.3f, 0.4f});
    //     store.CreateEntity(Position{5.0f, 6.0f}, Velocity{0.5f, 0.6f});

    //     int count = 0;
    //     store.ForEach<Position, Velocity>([&](Position &p, Velocity &v)
    //                                       {
    //     // Just verify that all are accessible
    //     assert(p.x >= 1.0f && p.y >= 2.0f);
    //     assert(v.vx >= 0.1f && v.vy >= 0.2f);
    //     count++; });

    //     assert(count == 3);
    //     std::cout << "TestForEach passed.\n";
    // }

    // // Test: Modify components after creation
    // void TestModifyComponents()
    // {
    //     mk::EntityStore store;
    //     mk::Entity e = store.CreateEntity(Position{1.0f, 1.0f});

    //     // Modify the component
    //     Position &p = store.GetComponent<Position>(e);
    //     p.x = 10.0f;
    //     p.y = 20.0f;

    //     // Retrieve again and check
    //     Position &p2 = store.GetComponent<Position>(e);
    //     assert(p2.x == 10.0f && p2.y == 20.0f);

    //     std::cout << "TestModifyComponents passed.\n";
    // }

    // void RunTests()
    // {
    //     TestSingleComponent();
    //     TestMultipleComponents();
    //     TestDestroyEntity();
    //     TestForEach();
    //     TestModifyComponents();
    // }

} // namespace mk
