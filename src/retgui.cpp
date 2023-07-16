#include "retgui/retgui.hpp"
#include "retgui/internal.hpp"

namespace retgui
{
    RetGuiContext* g_retGui{ nullptr };  // NOLINT

    auto create_context() -> RetGuiContext*
    {
        auto* ctx = new RetGuiContext;
        set_current_context(ctx);

        ctx->root = create_element<Element>();

        return ctx;
    }

    void destroy_context(RetGuiContext* ctx)
    {
        auto* ctxToDestroy = ctx;
        if (ctxToDestroy == nullptr)
        {
            ctxToDestroy = g_retGui;
        }

        // #TODO: Shutdown/De-initialise context
        delete ctxToDestroy;
    }

    auto get_current_context() -> RetGuiContext*
    {
        return g_retGui;
    }

    void set_current_context(RetGuiContext* ctx)
    {
        g_retGui = ctx;
    }

    void add_to_root(const ElementBasePtr& element)
    {
        g_retGui->root->add_child(element);
    }

    void remove_from_root(const ElementBasePtr& element)
    {
        g_retGui->root->remove_child(element);
    }

    void set_root_size(std::uint32_t width, std::uint32_t height)
    {
        g_retGui->displaySize = { float(width), float(height) };
        g_retGui->root->set_size(Dim2{ Dim{ 0.0f, float(width) }, Dim{ 0.0f, float(height) } });
        g_retGui->dirty = true;
    }

    void set_dirty()
    {
        g_retGui->dirty = true;
    }

    void update_element(Element* element)
    {
        auto child = element->get_first_child();
        while (child != nullptr)
        {
            update_element(child.get());
            child = child->get_next_sibling();
        }

        element->update();
    }

    void update()
    {
        auto child = g_retGui->root->get_first_child();
        while (child != nullptr)
        {
            update_element(child.get());
            child = child->get_next_sibling();
        }
    }

    void render_element(const Element* element)
    {
        element->render(g_retGui->drawData);

        auto child = element->get_first_child();
        while (child != nullptr)
        {
            render_element(child.get());
            child = child->get_next_sibling();
        }
    }

    bool render()
    {
        if (!g_retGui->dirty)
        {
            return false;
        }

        auto* drawData = &g_retGui->drawData;
        *drawData = DrawData{};
        auto child = g_retGui->root->get_first_child();
        while (child != nullptr)
        {
            render_element(child.get());
            child = child->get_next_sibling();
        }

        if (!drawData->DrawCmds.empty())
        {
            drawData->DrawCmds.back().IndexCount = drawData->IndexBuffer.size() - drawData->DrawCmds.back().IndexOffset;
        }

        g_retGui->dirty = false;
        return true;
    }

    auto get_draw_data() -> const DrawData*
    {
        return &g_retGui->drawData;
    }

}