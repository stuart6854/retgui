#pragma once

#include "ruic.hpp"

#include <memory>
#include <type_traits>

namespace ruic
{
    /*
     * Used for positioning and sizing of Widgets
     * Inspired by CEGUI.
     */
    struct Dim
    {
        float scale{};   // Percentage of parents size
        float offset{};  // Pixel offset

        Dim() = default;
        explicit Dim(float scale, float offset) : scale(scale), offset(offset) {}
        Dim(const Dim& other) : Dim(other.scale, other.offset) {}

        auto operator+(const Dim& rhs) const -> Dim;
        auto operator-(const Dim& rhs) const -> Dim;
        auto operator*(const Dim& rhs) const -> Dim;
        auto operator/(const Dim& rhs) const -> Dim;

        auto operator+=(const Dim& rhs) -> const Dim&;
        auto operator-=(const Dim& rhs) -> const Dim&;
        auto operator*=(const Dim& rhs) -> const Dim&;
        auto operator/=(const Dim& rhs) -> const Dim&;

        bool operator==(const Dim& rhs) const { return scale == rhs.scale && offset == rhs.offset; }
        bool operator!=(const Dim& rhs) const { return !(*this == rhs); }

        static auto zero() -> Dim { return Dim{ 0, 0 }; }
        static auto relative() -> Dim { return Dim{ 1, 0 }; }
        static auto percent() -> Dim { return Dim{ 0.01f, 0 }; }
        static auto pixel() -> Dim { return Dim{ 0, 1 }; }
    };
    struct Dim2
    {
        Dim x{};
        Dim y{};
    };

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

        auto get_position() const -> const Dim2& { return m_position; }
        auto get_size() const -> const Dim2& { return m_size; }

        auto set_position(const Dim2& position) -> WidgetPtr;
        auto set_size(const Dim2& size) -> WidgetPtr;

        auto get_screen_position() const -> Vec2;
        auto get_screen_size() const -> Vec2;
        auto get_widget_bounds() const -> Rect;

    private:
        WidgetPtr m_parent{ nullptr };
        WidgetPtr m_prevSibling{ nullptr };
        WidgetPtr m_nextSibling{ nullptr };
        WidgetPtr m_firstChild{ nullptr };
        WidgetPtr m_lastChild{ nullptr };

        Dim2 m_position{};
        Dim2 m_size{};
    };

    template <typename T>
    auto create_widget() -> WidgetPtr
    {
        static_assert(std::is_base_of<Widget, T>::value, "T must derive from Widget.");
        return std::make_shared<T>();
    }
}