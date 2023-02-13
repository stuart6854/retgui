#pragma once

#include <vector>
#include <type_traits>

#include <cassert>
#define RET_ASSERT(_expr) assert(_expr)

struct RetVec2
{
    float x, y;

    RetVec2() = default;
    RetVec2(float x, float y);

    auto length() const -> float;

    auto normalize() -> RetVec2&;
};

struct RetVec4
{
    float x, y, z, w;
};

/* Public widget interface */
class RetWidget
{
public:
private:
    // Internal widget id/ptr/handle
};

struct RetDrawVert
{
    RetVec2 position;
    RetVec2 texCoord;
    RetVec4 color;
};

using RetDrawIdx = uint16_t;

struct RetDrawList
{
    std::vector<RetDrawVert> vertices{};
    std::vector<RetDrawIdx> indices{};

    auto index_count() const -> uint32_t;

    void add_line(RetVec2 a, RetVec2 b, RetVec4 color, float thickness);
    void add_rect(RetVec2 top_left, RetVec2 bottom_right, RetVec4 color, RetVec2 uv0 = { 0, 0 }, RetVec2 uv1 = { 1, 1 });
};

struct RetDrawData
{
    uint32_t screen_width{};
    uint32_t screen_height{};
    std::vector<RetDrawList> draw_lists{};
};

namespace retgui
{
    void create_context();
    void destroy_context();

    auto get_draw_data() -> RetDrawData*;

    /* Removes all widgets from the screen. */
    void clear_screen();

    /* Create a new widget of type T to the screen. The widget is added to the screen root. */
    template <typename T>
    std::enable_if_t<std::is_base_of_v<RetWidget, T>, int> create_widget(RetWidget* parent = nullptr);

    /* Remove a widget from the Screen and destroy it. */
    void destroy_widget(RetWidget* widget);
}

//////////////////////////////////////////////////////////////////////////
////                       Implementation                             ////
//////////////////////////////////////////////////////////////////////////

template <typename T>
std::enable_if_t<std::is_base_of_v<RetWidget, T>, int> retgui::create_widget(RetWidget* parent)
{
}