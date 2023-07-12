#include "ruic/ruic.hpp"
#include "ruic/internal.hpp"

namespace ruic
{
    RuicContext* g_ruic{ nullptr };  // NOLINT

    auto create_context() -> RuicContext*
    {
        auto* ctx = new RuicContext;
        set_current_context(ctx);

        ctx->root = create_element<Element>();

        return ctx;
    }

    void destroy_context(RuicContext* ctx)
    {
        auto* ctxToDestroy = ctx;
        if (ctxToDestroy == nullptr)
        {
            ctxToDestroy = g_ruic;
        }

        // #TODO: Shutdown/De-initialise context
        delete ctxToDestroy;
    }

    auto get_current_context() -> RuicContext*
    {
        return g_ruic;
    }

    void set_current_context(RuicContext* ctx)
    {
        g_ruic = ctx;
    }

    void add_to_root(const ElementBasePtr& element)
    {
        g_ruic->root->add_child(element);
    }

    void remove_from_root(const ElementBasePtr& element)
    {
        g_ruic->root->remove_child(element);
    }

    void set_root_size(std::uint32_t width, std::uint32_t height)
    {
        g_ruic->root->set_size(Dim2{ Dim{ 0.0f, float(width) }, Dim{ 0.0f, float(height) } });
        g_ruic->dirty = true;
    }

    void set_dirty()
    {
        g_ruic->dirty = true;
    }

    void render_element(const Element* element)
    {
        const auto bounds = element->get_bounds();
        g_ruic->drawData.drawList.add_rect(bounds.tl, bounds.br, element->get_color().Int32());

        auto child = element->get_first_child();
        while (child != nullptr)
        {
            render_element(child.get());
            child = child->get_next_sibling();
        }
    }

    bool render()
    {
        if (!g_ruic->dirty)
        {
            return false;
        }

        auto* drawData = &g_ruic->drawData;
        *drawData = RuicDrawData{};
        auto child = g_ruic->root->get_first_child();
        while (child != nullptr)
        {
            render_element(child.get());
            child = child->get_next_sibling();
        }

        g_ruic->dirty = false;
        return true;
    }

    auto get_draw_data() -> const RuicDrawData*
    {
        return &g_ruic->drawData;
    }

}