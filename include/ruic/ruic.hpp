#pragma once

#include <vector>
#include <cstdint>
#include <memory>

namespace ruic
{
    // ----------------------------------------------------
    // Forward Declarations
    // ----------------------------------------------------
    struct RuicContext;
    struct RuicDrawData;

    // ----------------------------------------------------
    // Context creation and access
    // ----------------------------------------------------
    auto create_context() -> RuicContext*;
    void destroy_context(RuicContext* ctx = nullptr);
    auto get_current_context() -> RuicContext*;
    void set_current_context(RuicContext* ctx);

    class Element;

    template <typename T>
    using ElementPtr = std::shared_ptr<T>;
    using ElementBasePtr = ElementPtr<Element>;

    void add_to_root(const ElementBasePtr& element);
    void remove_from_root(const ElementBasePtr& element);

    void set_root_size(std::uint32_t width, std::uint32_t height);

    void set_dirty();

    bool render();  // Returns TRUE if render data changed
    auto get_draw_data() -> const RuicDrawData*;

    using DrawIdx = std::uint32_t;

#define RUIC_COL32_R_SHIFT 0
#define RUIC_COL32_G_SHIFT 8
#define RUIC_COL32_B_SHIFT 16
#define RUIC_COL32_A_SHIFT 24
#define RUIC_COL32_A_MASK 0xFF000000
#define RUIC_COL32(R, G, B, A)                                                                 \
    (((std::uint32_t)(A) << RUIC_COL32_A_SHIFT) | ((std::uint32_t)(B) << RUIC_COL32_B_SHIFT) | \
     ((std::uint32_t)(G) << RUIC_COL32_G_SHIFT) | ((std::uint32_t)(R) << RUIC_COL32_R_SHIFT))
#define RUIC_COL32_WHITE RUIC_COL32(255, 255, 255, 255)

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
        void add_rect(const Vec2& min, const Vec2& max);
    };

}