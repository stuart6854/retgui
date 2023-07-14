#pragma once

#include "retgui/io.hpp"
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
        IO io{};

        ElementBasePtr root{ nullptr };
        bool dirty{ true };

        Element* hoveredElement{ nullptr };
        Element* activeElement{ nullptr };

        RetGuiDrawData drawData{};
    };
}