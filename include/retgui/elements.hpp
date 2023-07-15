#pragma once

#include "retgui.hpp"
#include "types.hpp"
#include "fonts.hpp"

#include <memory>
#include <string>
#include <functional>
#include <type_traits>

namespace retgui
{
#define RETGUI_ELEMENT_STATE_HOVERED U8(1u << 0u)  // 1
#define RETGUI_ELEMENT_STATE_FOCUSED U8(1u << 1u)  // 2
#define RETGUI_ELEMENT_STATE_ACTIVE U8(1u << 2u)   // 4

    class Element;

    template <typename T>
    using ElementPtr = std::shared_ptr<T>;

    template <typename T>
    auto create_element() -> ElementPtr<T>
    {
        static_assert(std::is_base_of<Element, T>::value, "T must derive from Element.");
        return std::make_shared<T>();
    }

    // https://stackoverflow.com/questions/34639447/disable-or-hide-some-parent-class-functions-in-c
    class Element : public std::enable_shared_from_this<Element>
    {
    public:
        Element() = default;
        virtual ~Element() = default;

        void update();
        virtual void render(DrawList& drawList) const;

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

        auto get_render_color() const -> const Color&;

        auto get_state() const -> U8;
        void add_state(U8 state);
        void remove_state(U8 state);

        bool is_cursor_inside() const;

        virtual void on_mouse_button_down(int button) {}
        virtual void on_mouse_button_up(int button) {}

    protected:
        auto get_hovered_color() const -> const Color& { return m_hoveredColor; }
        auto set_hovered_color(const Color& color) -> ElementBasePtr;

        auto get_active_color() const -> const Color& { return m_activeColor; }
        auto set_active_color(const Color& color) -> ElementBasePtr;

        void set_enabled_states(U8 states);

    private:
        ElementBasePtr m_parent{ nullptr };
        ElementBasePtr m_prevSibling{ nullptr };
        ElementBasePtr m_nextSibling{ nullptr };
        ElementBasePtr m_firstChild{ nullptr };
        ElementBasePtr m_lastChild{ nullptr };

        Dim2 m_position{};
        Dim2 m_size{};

        Color m_color{ Color::white() };
        Color m_hoveredColor{ m_color };
        Color m_activeColor{ m_color };

        U8 m_state{};
        U8 m_enabledStates{};
    };

    class Button : public Element
    {
    public:
        Button();
        ~Button() = default;

        using Element::get_hovered_color;
        using Element::set_hovered_color;

        using Element::get_active_color;
        using Element::set_active_color;

        auto get_on_clicked() const -> const std::function<void()>& { return m_onClicked; }
        void set_on_clicked(std::function<void()>&& callback);

        void on_mouse_button_up(int button) override;

    private:
        std::function<void()> m_onClicked;
    };

    class Label : public Element
    {
    public:
        Label();
        ~Label() = default;

        void render(DrawList& drawList) const override;

        auto get_font() const -> Font* { return m_font; }
        void set_font(Font* font);

        auto get_text() const -> const std::string& { return m_text; }
        void set_text(const std::string& text);

    private:
        Font* m_font{ nullptr };
        std::string m_text{};
    };

}