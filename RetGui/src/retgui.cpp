#include "retgui.hpp"

#include "retgui_internal.hpp"

#include <cmath>

namespace retgui
{
    void create_context()
    {
        internal::create_context();
    }

    void destroy_context()
    {
        internal::destroy_context();
    }

}

/* RetVec2 */

RetVec2::RetVec2(float x, float y) : x(x), y(y) {}

auto RetVec2::length() const -> float
{
    return std::sqrt(x * x + y * y);
}

auto RetVec2::normalize() -> RetVec2&
{
    const auto len = length();
    x /= len;
    y /= len;
    return *this;
}

/* RetDrawList */

auto RetDrawList::index_count() const -> uint32_t
{
    return static_cast<uint32_t>(indices.size());
}

void RetDrawList::add_line(RetVec2 a, RetVec2 b, RetVec4 color, float thickness)
{
    const RetVec2 line_dir = RetVec2(b.x - a.x, b.y - a.y).normalize();
    const RetVec2 line_norm = { line_dir.y, -line_dir.x };
    const RetVec2 line_norm_thickness = { line_norm.x * thickness, line_norm.y * thickness };

    const auto vertCount = static_cast<uint16_t>(vertices.size());
    vertices.push_back({ { a.x - line_norm_thickness.x, a.y - line_norm_thickness.y }, { 0, 0 }, color });
    vertices.push_back({ { b.x - line_norm_thickness.x, b.y - line_norm_thickness.y }, { 0, 0 }, color });
    vertices.push_back({ { b.x + line_norm_thickness.x, b.y + line_norm_thickness.y }, { 0, 0 }, color });
    vertices.push_back({ { a.x + line_norm_thickness.x, a.y + line_norm_thickness.y }, { 0, 0 }, color });

    indices.push_back(vertCount);
    indices.push_back(vertCount + 1);
    indices.push_back(vertCount + 2);
    indices.push_back(vertCount + 2);
    indices.push_back(vertCount + 3);
    indices.push_back(vertCount + 0);
}

void RetDrawList::add_rect(RetVec2 top_left, RetVec2 bottom_right, RetVec4 color, RetVec2 uv0, RetVec2 uv1)
{
    const auto vertCount = static_cast<uint16_t>(vertices.size());
    vertices.push_back({ top_left, uv0, color });
    vertices.push_back({ { bottom_right.x, top_left.y }, { uv0.y, uv1.x }, color });
    vertices.push_back({ bottom_right, uv1, color });
    vertices.push_back({ { top_left.x, bottom_right.y }, { uv0.x, uv1.y }, color });

    indices.push_back(vertCount);
    indices.push_back(vertCount + 1);
    indices.push_back(vertCount + 2);
    indices.push_back(vertCount + 2);
    indices.push_back(vertCount + 3);
    indices.push_back(vertCount + 0);
}
