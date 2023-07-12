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
        if(m_firstChild == nullptr)
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

    auto Widget::set_pos_relative(const Vec2& relativePos) -> WidgetPtr
    {
        m_posRelative = relativePos;
        set_dirty();
        return shared_from_this();
    }

    auto Widget::set_pos_pixel_offset(const Vec2& pixelOffset) -> WidgetPtr
    {
        m_posPixelOffset = pixelOffset;
        set_dirty();
        return shared_from_this();
    }

    auto Widget::set_size_relative(const Vec2& relativeSize) -> WidgetPtr
    {
        m_sizeRelative = relativeSize;
        set_dirty();
        return shared_from_this();
    }

    auto Widget::set_size_pixel_offset(const Vec2& pixelOffset) -> WidgetPtr
    {
        m_sizePixelOffset = pixelOffset;
        set_dirty();
        return shared_from_this();
    }

    auto Widget::get_widget_bounds() const -> Rect
    {
        Vec2 calculatedRelativePos{};
        Vec2 calculatedRelativeSize{};
        if (get_parent() != nullptr)
        {
            const auto parentBounds = get_parent()->get_widget_bounds();
            calculatedRelativePos = parentBounds.tl;
        }

        auto TL = calculatedRelativePos + m_posPixelOffset;
        auto BR = TL + (calculatedRelativeSize + m_sizePixelOffset);
        return { TL, BR };
    }

}