#include "retgui/io.hpp"

#include "retgui/retgui.hpp"
#include "retgui/elements.hpp"
#include "retgui/internal.hpp"

namespace retgui
{
#pragma region Internal

    bool IsInsideBoundingBox(const Element* element, const Vec2& pos)
    {
        const auto bb = element->get_bounds();
        return (pos.x >= bb.tl.x && pos.x <= bb.br.x) && (pos.y >= bb.tl.y && pos.y <= bb.br.y);
    }

    bool trickle_cursor_pos(Element* element, const Vec2& cursorPos)
    {
        bool cursorInChild{ false };
        auto child = element->get_first_child();
        while (child != nullptr)
        {
            cursorInChild |= trickle_cursor_pos(child.get(), cursorPos);
            child = child->get_next_sibling();
        }

        if (cursorInChild)
        {
            // Cursor is hovering a child
            element->remove_state(RETGUI_ELEMENT_STATE_HOVERED);
            return true;
        }

        if (IsInsideBoundingBox(element, cursorPos))
        {
            // Cursor is hovering this element
            auto* lastHovered = get_current_context()->lastHoveredElement;
            if (lastHovered != nullptr && lastHovered != element)
            {
                lastHovered->remove_state(RETGUI_ELEMENT_STATE_HOVERED);
            }

            element->add_state(RETGUI_ELEMENT_STATE_HOVERED);
            get_current_context()->lastHoveredElement = element;
            return true;
        }

        // Cursor is not hovering this element or its children
        element->remove_state(RETGUI_ELEMENT_STATE_HOVERED);
        return false;
    }

#pragma endregion

    void trickle_cursor_pos(const Vec2& cursorPos)
    {
        auto child = get_current_context()->root->get_first_child();
        while (child != nullptr)
        {
            trickle_cursor_pos(child.get(), cursorPos);
            child = child->get_next_sibling();
        }
    }

    void handle_mouse_button(int button, bool isDown)
    {
        auto* context = get_current_context();

        if (context->lastHoveredElement != nullptr)
        {
            auto* hoveredElement = context->lastHoveredElement;
            if (isDown)
            {
                hoveredElement->add_state(RETGUI_ELEMENT_STATE_ACTIVE);
                hoveredElement->on_mouse_button_down(button);
            }
            else
            {
                hoveredElement->remove_state(RETGUI_ELEMENT_STATE_ACTIVE);
                hoveredElement->on_mouse_button_up(button);
            }
        }
    }

}