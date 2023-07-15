#include "retgui/elements.hpp"

#include "retgui/retgui.hpp"
#include "retgui/io.hpp"
#include "retgui/internal.hpp"

namespace retgui
{
    void Element::update()
    {
        auto* context = get_current_context();
        const auto& io = context->io;

        if (m_enabledStates & RETGUI_ELEMENT_STATE_HOVERED)
        {
            if (context->hoveredElement == this || context->hoveredElement == nullptr)
            {
                bool cursorInside = is_cursor_inside();
                if (cursorInside)
                {
                    context->hoveredElement = this;
                    add_state(RETGUI_ELEMENT_STATE_HOVERED);
                    if (m_enabledStates & RETGUI_ELEMENT_STATE_ACTIVE)
                    {
                        // #TODO: If any mouse btns are down
                        if (!(m_state & RETGUI_ELEMENT_STATE_ACTIVE) && io.mouseBtns[0])
                        {
                            context->activeElement = this;
                            add_state(RETGUI_ELEMENT_STATE_ACTIVE);
                            on_mouse_button_down(0);
                        }
                        //                        else if ((m_state & RETGUI_ELEMENT_STATE_ACTIVE) && !io.mouseBtns[0])  // #TODO: If all
                        //                        mouse btns are up
                        //                        {
                        //                            context->activeElement = nullptr;
                        //                            remove_state(RETGUI_ELEMENT_STATE_ACTIVE);
                        //                            on_mouse_button_up(0);
                        //                        }
                    }
                }
                else
                {
                    context->hoveredElement = nullptr;
                    remove_state(RETGUI_ELEMENT_STATE_HOVERED);
                }
            }

            if (context->activeElement == this)
            {
                if ((m_state & RETGUI_ELEMENT_STATE_ACTIVE) && !io.mouseBtns[0])  // #TODO: If all mouse btns are up
                {
                    context->activeElement = nullptr;
                    remove_state(RETGUI_ELEMENT_STATE_ACTIVE);
                    if (is_cursor_inside())
                    {
                        on_mouse_button_up(0);
                    }
                }
            }
        }
    }

    void Element::render(DrawList& drawList) const
    {
        const auto bounds = get_bounds();
        const auto& color = get_render_color();

        drawList.add_rect(bounds.tl, bounds.br, color.Int32(), { 0, 1 }, { 1, 0 });
    }

    auto Element::get_parent() const -> ElementBasePtr
    {
        return m_parent;
    }

    auto Element::get_prev_sibling() const -> ElementBasePtr
    {
        return m_prevSibling;
    }

    auto Element::get_next_sibling() const -> ElementBasePtr
    {
        return m_nextSibling;
    }

    auto Element::get_first_child() const -> ElementBasePtr
    {
        return m_firstChild;
    }

    auto Element::get_last_child() const -> ElementBasePtr
    {
        return m_lastChild;
    }

    auto Element::add_child(const ElementBasePtr& element) -> ElementBasePtr
    {
        if (m_firstChild == nullptr)
        {
            m_firstChild = element;
        }
        if (m_lastChild != nullptr)
        {
            m_lastChild->m_nextSibling = element;
        }

        element->m_prevSibling = m_lastChild;
        m_lastChild = element;
        element->m_parent = shared_from_this();

        set_dirty();
        return shared_from_this();
    }

    auto Element::add_child(Element& element) -> ElementBasePtr
    {
        return add_child(element.shared_from_this());
    }

    void Element::remove_child(const ElementBasePtr& element)
    {
        auto childPtr = m_firstChild;
        while (childPtr != nullptr && childPtr != element)
        {
            childPtr = childPtr->m_nextSibling;
        }

        if (childPtr == nullptr)
        {
            return;
        }

        childPtr->m_parent = nullptr;
        if (childPtr->m_prevSibling)
        {
            childPtr->m_prevSibling->m_nextSibling = childPtr->m_nextSibling;
        }
        if (childPtr->m_nextSibling)
        {
            childPtr->m_nextSibling->m_prevSibling = childPtr->m_prevSibling;
        }

        set_dirty();
    }

    auto Element::set_position(const Dim2& position) -> ElementBasePtr
    {
        m_position = position;
        set_dirty();
        return shared_from_this();
    }

    auto Element::set_size(const Dim2& size) -> ElementBasePtr
    {
        m_size = size;
        set_dirty();
        return shared_from_this();
    }

    auto Element::get_screen_position() const -> Vec2
    {
        Vec2 pos = { m_position.x.offset, m_position.y.offset };
        if (get_parent() != nullptr)
        {
            auto parentPos = get_parent()->get_screen_position();
            pos += parentPos;
            auto parentSize = get_parent()->get_screen_size();
            pos += parentSize* Vec2{ m_position.x.scale, m_position.y.scale };
        }
        return pos;
    }

    auto Element::get_screen_size() const -> Vec2
    {
        Vec2 size = { m_size.x.offset, m_size.y.offset };
        if (get_parent() != nullptr)
        {
            auto parentSize = get_parent()->get_screen_size();
            size += parentSize* Vec2{ m_size.x.scale, m_size.y.scale };
        }
        return size;
    }

    auto Element::get_bounds() const -> Rect
    {
        Vec2 pos = get_screen_position();
        Vec2 size = get_screen_size();

        auto TL = pos;
        auto BR = TL + size;
        return { TL, BR };
    }

    auto Element::set_color(const Color& color) -> ElementBasePtr
    {
        m_color = color;
        set_dirty();
        return shared_from_this();
    }

    auto Element::get_render_color() const -> const Color&
    {
        if (m_state & RETGUI_ELEMENT_STATE_ACTIVE)
        {
            return m_activeColor;
        }

        if (m_state & RETGUI_ELEMENT_STATE_HOVERED)
        {
            return m_hoveredColor;
        }

        return m_color;
    }

    auto Element::get_state() const -> U8
    {
        return m_state;
    }

    void Element::add_state(U8 state)
    {
        if (m_enabledStates & state)
        {
            m_state |= state;
            set_dirty();
        }
    }

    void Element::remove_state(U8 state)
    {
        if (m_enabledStates & state)
        {
            m_state &= ~state;
            set_dirty();
        }
    }

    bool Element::is_cursor_inside() const
    {
        auto& io = get_current_context()->io;
        const auto cursorPos = io.cursorPos;
        const auto bb = get_bounds();
        return (cursorPos.x >= bb.tl.x && cursorPos.x <= bb.br.x) && (cursorPos.y >= bb.tl.y && cursorPos.y <= bb.br.y);
    }

    auto Element::set_hovered_color(const Color& color) -> ElementBasePtr
    {
        m_hoveredColor = color;
        set_dirty();
        return shared_from_this();
    }

    auto Element::set_active_color(const Color& color) -> ElementBasePtr
    {
        m_activeColor = color;
        set_dirty();
        return shared_from_this();
    }

    void Element::set_enabled_states(U8 states)
    {
        m_enabledStates = states;
    }

    Button::Button()
    {
        set_enabled_states(RETGUI_ELEMENT_STATE_HOVERED | RETGUI_ELEMENT_STATE_ACTIVE);
    }

    void Button::set_on_clicked(std::function<void()>&& callback)
    {
        m_onClicked = callback;
    }

    void Button::on_mouse_button_up(int button)
    {
        m_onClicked();
    }

    Label::Label() = default;

    void Label::render(DrawList& drawList) const
    {
        const auto screenPos = get_screen_position();
        float x = screenPos.x;  // Align to be pixel perfect
        float y = screenPos.y;  // Align to be pixel-perfect

        y += float(m_font->Ascender);

        for (auto i = 0; i < m_text.size(); ++i)
        {
            char character = m_text[i];
            if (character == '\n')
            {
                x = screenPos.x;
                y += m_font->LineSpacing;
            }

            auto* glyph = m_font->get_glyph(U32(character));
            if (glyph == nullptr)
            {
                glyph = m_font->get_glyph('?');
            }

            Vec2 quadTL = { std::roundf(x + glyph->x0), std::roundf(y + glyph->y0) };
            Vec2 quadBR = { std::roundf(x + glyph->x1), std::roundf(y + glyph->y1) };
            Vec2 uvMin = { glyph->ux0, glyph->uy0 };
            Vec2 uvMax = { glyph->ux1, glyph->uy1 };

            drawList.add_rect(quadTL, quadBR, get_render_color().Int32(), uvMin, uvMax);

            double advance = 0.0;
            if (i < m_text.size() - 1)
            {
                advance = glyph->AdvanceX;
                //            char nextCharacter = string[i + 1];
                //            geometry.getAdvance(advance, character, nextCharacter);
            }

            float kerningOffset = 0.0f;
            x += float(advance) + kerningOffset;
        }
    }

    void Label::set_font(Font* font)
    {
        m_font = font;
        set_dirty();
    }

    void Label::set_text(const std::string& text)
    {
        m_text = text;
        set_dirty();
    }

}