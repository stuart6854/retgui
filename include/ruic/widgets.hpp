#pragma once

#include "ruic.hpp"

#include <memory>
#include <type_traits>

namespace ruic
{
    class Widget;
    using WidgetPtr = std::shared_ptr<Widget>;

    class Widget : public std::enable_shared_from_this<Widget>
    {
    public:
        Widget() = default;
        ~Widget() = default;

        auto get_parent() const -> WidgetPtr;
        auto get_prev_sibling() const -> WidgetPtr;
        auto get_next_sibling() const -> WidgetPtr;
        auto get_first_child() const -> WidgetPtr;
        auto get_last_child() const -> WidgetPtr;

        auto add_child(const WidgetPtr& widget) -> WidgetPtr;
        auto add_child(Widget& widget) -> WidgetPtr;
        void remove_child(const WidgetPtr& widget);

        auto get_pos_relative() const -> const Vec2& { return m_posRelative; }
        auto get_pos_pixel_offset() const -> const Vec2& { return m_posPixelOffset; }
        auto get_size_relative() const -> const Vec2& { return m_sizeRelative; }
        auto get_size_pixel_offset() const -> const Vec2& { return m_sizePixelOffset; }

        auto set_pos_relative(const Vec2& relativePos) -> WidgetPtr;
        auto set_pos_pixel_offset(const Vec2& pixelOffset) -> WidgetPtr;
        auto set_size_relative(const Vec2& relativeSize) -> WidgetPtr;
        auto set_size_pixel_offset(const Vec2& pixelOffset) -> WidgetPtr;

        auto get_widget_bounds() const -> Rect;

    private:
        WidgetPtr m_parent{ nullptr };
        WidgetPtr m_prevSibling{ nullptr };
        WidgetPtr m_nextSibling{ nullptr };
        WidgetPtr m_firstChild{ nullptr };
        WidgetPtr m_lastChild{ nullptr };

        Vec2 m_posRelative{};
        Vec2 m_posPixelOffset{};
        Vec2 m_sizeRelative{};
        Vec2 m_sizePixelOffset{};
    };

    template <typename T>
    auto create_widget() -> WidgetPtr
    {
        static_assert(std::is_base_of<Widget, T>::value, "T must derive from Widget.");
        return std::make_shared<T>();
    }
}