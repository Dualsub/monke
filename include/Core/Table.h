#pragma once

#include "Core/Core.h"

#include <cassert>

#define DECLARE_TABLE(EnumType, RowType) \
    template <>                          \
    const RowType Table<EnumType, RowType>::s_rows[] =

namespace mk
{
    template <typename EnumType, typename RowType>
    class Table
    {
    private:
        constexpr static uint32_t c_numRows = static_cast<uint32_t>(EnumType::Count);

        static const RowType s_rows[c_numRows];

    public:
        Table() = default;
        ~Table() = default;

        static const RowType &GetRow(EnumType row)
        {
            return s_rows[static_cast<uint32_t>(row)];
        }
    };

    template <typename EnumType, typename RowType>
    const RowType Table<EnumType, RowType>::s_rows[c_numRows] = {};

    template <typename EnumType, typename RowType>
    const RowType &GetRow(EnumType row)
    {
        assert((static_cast<uint32_t>(row) < static_cast<uint32_t>(EnumType::Count) && row != EnumType::None) && "Invalid row");
        return Table<EnumType, RowType>::GetRow(row);
    }
}