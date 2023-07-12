#include "ruic/elements.hpp"

#include "ruic/ruic.hpp"

namespace ruic
{
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
            pos += parentSize * Vec2{ m_position.x.scale, m_position.y.scale };
        }
        return pos;
    }

    auto Element::get_screen_size() const -> Vec2
    {
        Vec2 size = { m_size.x.offset, m_size.y.offset };
        if (get_parent() != nullptr)
        {
            auto parentSize = get_parent()->get_screen_size();
            size += parentSize * Vec2{ m_size.x.scale, m_size.y.scale };
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

}