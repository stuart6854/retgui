#pragma once

#include "retgui/elements.hpp"

#include <vector>

namespace retgui
{
    struct RetGuiDrawData
    {
        DrawList drawList{};
    };

    struct RetGuiContext
    {
        Vec2 displaySize{};

        ElementBasePtr root{ nullptr };
        bool dirty{ true };

        Element* lastHoveredElement{ nullptr };
        Element* lastFocusedElement{ nullptr };

        RetGuiDrawData drawData{};
    };
}