#pragma once

#include "ruic.hpp"

#include <memory>
#include <type_traits>

namespace ruic
{
    class Element;

    template <typename T>
    using ElementPtr = std::shared_ptr<T>;

    // https://stackoverflow.com/questions/34639447/disable-or-hide-some-parent-class-functions-in-c
    class Element : public std::enable_shared_from_this<Element>
    {
    public:
        Element() = default;
        ~Element() = default;

        auto get_parent() const -> ElementBasePtr;
        auto get_prev_sibling() const -> ElementBasePtr;
        auto get_next_sibling() const -> ElementBasePtr;
        auto get_first_child() const -> ElementBasePtr;
        auto get_last_child() const -> ElementBasePtr;

        auto add_child(const ElementBasePtr& element) -> ElementBasePtr;
        auto add_child(Element& element) -> ElementBasePtr;
        void remove_child(const ElementBasePtr& element);

        auto get_position() const -> const Dim2& { return m_position; }
        auto get_size() const -> const Dim2& { return m_size; }

        auto set_position(const Dim2& position) -> ElementBasePtr;
        auto set_size(const Dim2& size) -> ElementBasePtr;

        auto get_screen_position() const -> Vec2;
        auto get_screen_size() const -> Vec2;
        auto get_bounds() const -> Rect;

        auto get_color() const -> const Color& { return m_color; }
        auto set_color(const Color& color) -> ElementBasePtr;

    private:
        ElementBasePtr m_parent{ nullptr };
        ElementBasePtr m_prevSibling{ nullptr };
        ElementBasePtr m_nextSibling{ nullptr };
        ElementBasePtr m_firstChild{ nullptr };
        ElementBasePtr m_lastChild{ nullptr };

        Dim2 m_position{};
        Dim2 m_size{};

        Color m_color{ Color::white() };
    };

    template <typename T>
    auto create_element() -> ElementPtr<T>
    {
        static_assert(std::is_base_of<Element, T>::value, "T must derive from Element.");
        return std::make_shared<T>();
    }
}