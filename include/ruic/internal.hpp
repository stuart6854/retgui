#pragma once

#include "ruic/widgets.hpp"

#include <vector>

namespace ruic
{
    struct RuicDrawData
    {
        DrawList drawList{};
    };

    struct RuicContext
    {
        WidgetPtr root{ nullptr };
        bool dirty{ true };

        RuicDrawData drawData{};
    };
}