#pragma once

#include "ruic/elements.hpp"

#include <vector>

namespace ruic
{
    struct RuicDrawData
    {
        DrawList drawList{};
    };

    struct RuicContext
    {
        Vec2 displaySize{};

        ElementBasePtr root{ nullptr };
        bool dirty{ true };

        RuicDrawData drawData{};
    };
}