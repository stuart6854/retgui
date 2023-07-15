#pragma once

#include "types.hpp"
#include "fonts.hpp"

#include <array>

namespace retgui
{
    struct IO
    {
        Fonts Fonts{};
        Vec2 cursorPos{};
        std::array<bool, 8> mouseBtns{};
    };
}
