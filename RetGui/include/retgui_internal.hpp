#pragma once

#include "retgui.hpp"

#include <memory>

class RetRootWidget : RetWidget
{
};

struct RetContext
{
    // Time
    // Delta time

    std::unique_ptr<RetRootWidget> rootWidget;
    // Focused element/widget

    std::unique_ptr<RetDrawData> draw_data;
};

namespace retgui::internal
{
    void create_context();
    void destroy_context();

    /* Compiles the data and draw lists required to render the UI. */
    void render();
}