#include "retgui_internal.hpp"

namespace retgui::internal
{
    static std::unique_ptr<RetContext> g_context;

    void create_context()
    {
        g_context = std::make_unique<RetContext>();
        g_context->rootWidget = std::make_unique<RetRootWidget>();
    }

    void destroy_context()
    {
        g_context = nullptr;
    }

    void render()
    {
        RET_ASSERT(g_context != nullptr);
    }

}