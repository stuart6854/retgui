#include "ruic/widgets.hpp"

#include "ruic/ruic.hpp"

namespace ruic
{
    auto Widget::get_parent() const -> WidgetPtr
    {
        return m_parent;
    }

    auto Widget::get_prev_sibling() const -> WidgetPtr
    {
        return m_prevSibling;
    }

    auto Widget::get_next_sibling() const -> WidgetPtr
    {
        return m_nextSibling;
    }

    auto Widget::get_first_child() const -> WidgetPtr
    {
        return m_firstChild;
    }

    auto Widget::get_last_child() const -> WidgetPtr
    {
        return m_lastChild;
    }

    auto Widget::add_child(const WidgetPtr& widget) -> WidgetPtr
    {
        if (m_firstChild == nullptr)
        {
            m_firstChild = widget;
        }
        if (m_lastChild != nullptr)
        {
            m_lastChild->m_nextSibling = widget;
        }

        widget->m_prevSibling = m_lastChild;
        m_lastChild = widget;
        widget->m_parent = shared_from_this();

        set_dirty();
        return shared_from_this();
    }

    auto Widget::add_child(Widget& widget) -> WidgetPtr
    {
        return add_child(widget.shared_from_this());
    }

    void Widget::remove_child(const WidgetPtr& widget)
    {
        auto childPtr = m_firstChild;
        while (childPtr != nullptr && childPtr != widget)
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

    auto Widget::set_position(const Dim2& position) -> WidgetPtr
    {
        m_position = position;
        set_dirty();
        return shared_from_this();
    }

    auto Widget::set_size(const Dim2& size) -> WidgetPtr
    {
        m_size = size;
        set_dirty();
        return shared_from_this();
    }

    auto Widget::get_screen_position() const -> Vec2
    {
        Vec2 pos = { m_position.x.offset, m_position.y.offset };
        if (get_parent() != nullptr)
        {
            // #TODO: Calculate bounds from parent
        }
        return pos;
    }

    auto Widget::get_screen_size() const -> Vec2
    {
        Vec2 size = { m_size.x.offset, m_size.y.offset };
        if (get_parent() != nullptr)
        {
            // #TODO: Calculate bounds from parent
        }
        return size;
    }

    auto Widget::get_widget_bounds() const -> Rect
    {
        Vec2 pos = get_screen_position();
        Vec2 size = get_screen_size();

        auto TL = pos;
        auto BR = TL + size;
        return { TL, BR };
    }
}