#pragma once

#include <cmath>
#include <vector>
#include <cstdint>

namespace retgui
{
    using I32 = std::int32_t;

    using U8 = std::uint8_t;
    using U32 = std::uint32_t;

    using TexId = std::uint64_t;

    using DrawIdx = U32;

#define RETGUI_COL32_R_SHIFT 0u
#define RETGUI_COL32_G_SHIFT 8u
#define RETGUI_COL32_B_SHIFT 16u
#define RETGUI_COL32_A_SHIFT 24u
#define RETGUI_COL32(R, G, B, A)                                                                                    \
    (((U32)(A) << RETGUI_COL32_A_SHIFT) | ((U32)(B) << RETGUI_COL32_B_SHIFT) | ((U32)(G) << RETGUI_COL32_G_SHIFT) | \
     ((U32)(R) << RETGUI_COL32_R_SHIFT))
#define RUIC_COL32_WHITE RETGUI_COL32(255, 255, 255, 255)

    auto ColorToUInt32(float r, float g, float b, float a) -> U32;

    struct Color
    {
        float r;
        float g;
        float b;
        float a;

        Color() = default;
        explicit Color(float r, float g, float b, float a) : r(r), g(g), b(b), a(a) {}
        ~Color() = default;

        auto Int32() const -> U32 { return ColorToUInt32(r, g, b, a); }

        static auto white() -> Color { return Color{ 1, 1, 1, 1 }; }
        static auto black() -> Color { return Color{ 0, 0, 0, 1 }; }
        static auto transparent() -> Color { return Color{ 0, 0, 0, 0 }; }
    };

    struct Vec2
    {
        float x{ 0.0f };
        float y{ 0.0f };

        auto operator+(const Vec2& rhs) const -> Vec2 { return { x + rhs.x, y + rhs.y }; }
        auto operator-(const Vec2& rhs) const -> Vec2 { return { x - rhs.x, y - rhs.y }; }
        auto operator*(const Vec2& rhs) const -> Vec2 { return { x * rhs.x, y * rhs.y }; }
        auto operator/(const Vec2& rhs) const -> Vec2 { return { x / rhs.x, y / rhs.y }; }

        auto operator+=(const Vec2& rhs) -> const Vec2&
        {
            x = x + rhs.x;
            y = y + rhs.y;
            return *this;
        }
        auto operator-=(const Vec2& rhs) -> const Vec2&
        {
            x = x - rhs.x;
            y = y - rhs.y;
            return *this;
        }
        auto operator*=(const Vec2& rhs) -> const Vec2&
        {
            x = x * rhs.x;
            y = y * rhs.y;
            return *this;
        }
        auto operator/=(const Vec2& rhs) -> const Vec2&
        {
            x = x / rhs.x;
            y = y / rhs.y;
            return *this;
        }
    };

    struct Rect
    {
        Vec2 tl{};
        Vec2 br{};

        auto width() const -> float { return br.x - tl.x; }
        auto height() const -> float { return br.y - tl.y; }
    };

    /*
     * Used for positioning and sizing of Elements
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

        bool operator==(const Dim& rhs) const;
        bool operator!=(const Dim& rhs) const;

        static auto zero() -> Dim { return Dim{ 0, 0 }; }
        static auto relative() -> Dim { return Dim{ 1, 0 }; }
        static auto percent() -> Dim { return Dim{ 0.01f, 0 }; }
        static auto pixel() -> Dim { return Dim{ 0, 1 }; }
    };
    struct Dim2
    {
        Dim x{};
        Dim y{};

        Dim2() = default;
        explicit Dim2(const Dim& x, const Dim& y) : x(x), y(y) {}
        Dim2(const Dim2& other) : Dim2(other.x, other.y) {}

        auto operator+(const Dim2& rhs) const -> Dim2;
        auto operator-(const Dim2& rhs) const -> Dim2;
        auto operator*(const Dim2& rhs) const -> Dim2;
        auto operator/(const Dim2& rhs) const -> Dim2;

        auto operator+=(const Dim2& rhs) -> const Dim2&;
        auto operator-=(const Dim2& rhs) -> const Dim2&;
        auto operator*=(const Dim2& rhs) -> const Dim2&;
        auto operator/=(const Dim2& rhs) -> const Dim2&;

        bool operator==(const Dim2& rhs) const;
        bool operator!=(const Dim2& rhs) const;
    };

    struct DrawVert
    {
        Vec2 pos{};
        Vec2 uv{};
        std::uint32_t col{};
    };

    struct DrawList
    {
        std::vector<DrawVert> VertBuffer;
        std::vector<DrawIdx> IdxBuffer;

        void add_line(const Vec2& a, const Vec2& b);
        void add_rect(const Vec2& min, const Vec2& max, std::uint32_t color, const Vec2& uvMin, const Vec2& uvMax);
    };
}