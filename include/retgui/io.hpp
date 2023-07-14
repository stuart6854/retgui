#pragma once

#include "retgui/types.hpp"

#include <array>

namespace retgui
{
    struct IO
    {
        Vec2 cursorPos{};
        std::array<bool, 8> mouseBtns{};
    };
}
