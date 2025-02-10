#pragma once

#include <array>
#include <queue>
#include <iterator>
#include <bitset>

namespace ACS
{
    template <typename T, size_t N>
    class Pool
    {
    private:
        using IndexType = size_t;

        std::array<T, N> m_pool;
        std::bitset<N> m_isActive;
        std::queue<IndexType> m_freeIndices;

        size_t m_size = 0;

    public:
        IndexType Add(const T &item)
        {
            if (m_freeIndices.empty())
            {
                m_pool[m_size] = item;
                m_isActive[m_size] = true;
                return m_size++;
            }

            IndexType index = m_freeIndices.front();
            m_freeIndices.pop();
            m_pool[index] = item;
            m_isActive[index] = true;
            return index;
        }

        void Remove(IndexType index)
        {
            m_isActive[index] = false;
            m_freeIndices.push(index);
        }

        T &operator[](IndexType index)
        {
            return m_pool[index];
        }

        size_t Size() const
        {
            return m_size;
        }

        size_t Capacity() const
        {
            return N;
        }

        size_t Count() const
        {
            return Size() - m_freeIndices.size();
        }

        size_t InactiveCount() const
        {
            return m_freeIndices.size();
        }

        bool Empty() const
        {
            return Size() == 0;
        }

        class Iterator
        {
        public:
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T *;
            using reference = T &;
            using iterator_category = std::forward_iterator_tag;

        private:
            const Pool<T, N> *pool;
            IndexType index;

            void SkipInactive()
            {
                while (index < N && !pool->m_isActive[index])
                {
                    ++index;
                }
            }

        public:
            Iterator(const Pool<T, N> *p, IndexType idx) : pool(p), index(idx)
            {
                SkipInactive();
            }

            reference operator*() const
            {
                return const_cast<reference>(pool->m_pool[index]);
            }

            pointer operator->() const
            {
                return &pool->m_pool[index];
            }

            Iterator &operator++()
            {
                ++index;
                SkipInactive();
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const Iterator &other) const
            {
                return index == other.index && pool == other.pool;
            }

            bool operator!=(const Iterator &other) const
            {
                return !(*this == other);
            }

            IndexType GetIndex() const
            {
                return index;
            }
        };

        Iterator begin() const
        {
            return Iterator(this, 0);
        }

        Iterator end() const
        {
            return Iterator(this, N);
        }
    };
}