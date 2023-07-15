#include "retgui/types.hpp"

namespace retgui
{
    auto ColorToUInt32(float r, float g, float b, float a) -> U32
    {
        auto red = U32(std::roundf(r * 255.0f));
        auto green = U32(std::roundf(g * 255.0f));
        auto blue = U32(std::roundf(b * 255.0f));
        auto alpha = U32(std::roundf(a * 255.0f));
        return RETGUI_COL32(red, green, blue, alpha);
    }

    auto Dim::operator+(const Dim& rhs) const -> Dim
    {
        return Dim{ scale + rhs.scale, offset + rhs.offset };
    }

    auto Dim::operator-(const Dim& rhs) const -> Dim
    {
        return Dim{ scale - rhs.scale, offset - rhs.offset };
    }

    auto Dim::operator*(const Dim& rhs) const -> Dim
    {
        return Dim{ scale * rhs.scale, offset * rhs.offset };
    }

    auto Dim::operator/(const Dim& rhs) const -> Dim
    {
        return Dim{ scale / rhs.scale, offset / rhs.offset };
    }

    auto Dim::operator+=(const Dim& rhs) -> const Dim&
    {
        *this = *this + rhs;
        return *this;
    }

    auto Dim::operator-=(const Dim& rhs) -> const Dim&
    {
        *this = *this - rhs;
        return *this;
    }

    auto Dim::operator*=(const Dim& rhs) -> const Dim&
    {
        *this = *this * rhs;
        return *this;
    }

    auto Dim::operator/=(const Dim& rhs) -> const Dim&
    {
        *this = *this / rhs;
        return *this;
    }

    bool Dim::operator==(const Dim& rhs) const
    {
        return scale == rhs.scale && offset == rhs.offset;
    }

    bool Dim::operator!=(const Dim& rhs) const
    {
        return !(*this == rhs);
    }

    auto Dim2::operator+(const Dim2& rhs) const -> Dim2
    {
        return Dim2{ x + rhs.x, y + rhs.y };
    }

    auto Dim2::operator-(const Dim2& rhs) const -> Dim2
    {
        return Dim2{ x - rhs.x, y - rhs.y };
    }

    auto Dim2::operator*(const Dim2& rhs) const -> Dim2
    {
        return Dim2{ x * rhs.x, y * rhs.y };
    }

    auto Dim2::operator/(const Dim2& rhs) const -> Dim2
    {
        return Dim2{ x / rhs.x, y / rhs.y };
    }

    auto Dim2::operator+=(const Dim2& rhs) -> const Dim2&
    {
        *this = *this + rhs;
        return *this;
    }

    auto Dim2::operator-=(const Dim2& rhs) -> const Dim2&
    {
        *this = *this - rhs;
        return *this;
    }

    auto Dim2::operator*=(const Dim2& rhs) -> const Dim2&
    {
        *this = *this * rhs;
        return *this;
    }

    auto Dim2::operator/=(const Dim2& rhs) -> const Dim2&
    {
        *this = *this / rhs;
        return *this;
    }

    bool Dim2::operator==(const Dim2& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }

    bool Dim2::operator!=(const Dim2& rhs) const
    {
        return !(*this == rhs);
    }

    void DrawList::add_line(const Vec2& a, const Vec2& b) {}

    void DrawList::add_rect(const Vec2& min, const Vec2& max, std::uint32_t color, const Vec2& uvMin, const Vec2& uvMax)
    {
        DrawVert a{ min, uvMin, color };                              // TL
        DrawVert b{ { min.x, max.y }, { uvMin.x, uvMax.y }, color };  // BL
        DrawVert c{ max, uvMax, color };                              // BR
        DrawVert d{ { max.x, min.y }, { uvMax.x, uvMin.y }, color };  // TR

        const auto idxOffset = VertBuffer.size();
        VertBuffer.push_back(a);
        VertBuffer.push_back(b);
        VertBuffer.push_back(c);
        VertBuffer.push_back(d);

        IdxBuffer.push_back(idxOffset + 0);
        IdxBuffer.push_back(idxOffset + 1);
        IdxBuffer.push_back(idxOffset + 2);
        IdxBuffer.push_back(idxOffset + 2);
        IdxBuffer.push_back(idxOffset + 3);
        IdxBuffer.push_back(idxOffset + 0);
    }
}