#pragma once

#include "types.hpp"

#include <string>
#include <memory>
#include <filesystem>
#include <unordered_map>

class stbtt_fontinfo;

namespace retgui
{
    struct CharsetRange
    {
        std::int32_t Begin;
        std::int32_t End;
    };

    /* A structure that describes a glyph. */
    struct GlyphMetrics
    {
        // Bounding Box (around the origin)
        I32 x0;
        I32 y0;
        I32 x1;
        I32 y1;
        // Coordinates in atlas
        float ux0;
        float uy0;
        float ux1;
        float uy1;
        // The distance from the origin to the origin of the next glyph. This is usually a value > 0.
        float AdvanceX;
    };

    struct Font
    {
        float FontSize;     // Size this font was generated with.
        float Ascender;     // The pixel extents above the baseline in pixels(usually positive).
        float Descender;    // The extents below the baseline in pixels (usually negative).
        float LineSpacing;  // The baseline-to-baseline distance. Note: This is usually larger than the sum of the ascender and descender.
        float LineGap;      // The spacing in pixels between one rows descent and the next rows ascent.
        float MaxAdvanceWidth;  // This field gives the maximum horizontal cursor advance for all glyphs in the font.
        std::vector<GlyphMetrics> glyphs{};

        auto get_glyph(U32 codePoint) const -> const GlyphMetrics*
        {
            if (codePoint >= glyphs.size())
            {
                return nullptr;
            }
            return &glyphs[codePoint];
        }

        auto calc_text_size(const std::string& text) -> Vec2;
    };

    class Fonts
    {
    public:
        auto add_font_from_file(const std::string& fontFilename, float fontSize, std::vector<CharsetRange> charsetRanges = {}) -> Font*;

        /* Build & Retrieve atlas. */
        void get_texture_data_as_alpha8(std::vector<U8>& outPixels, U32& outWidth, U32& outHeight);
        void get_texture_data_as_rgba32(std::vector<U32>& outPixels, U32& outWidth, U32& outHeight);

        auto get_tex_id() const -> TexId { return m_texture; }
        void set_tex_id(TexId texture);

        auto get_white_pixel_coords() const -> const Vec2& { return m_whitePixelCoords; }

    private:
        std::vector<std::unique_ptr<Font>> m_fonts{};
        TexId m_texture{};
        Vec2 m_whitePixelCoords{};

        struct FontCharToPack
        {
            U32 FontIdx{};
            U32 CodePoint{};
            std::uint8_t* Bitmap{ nullptr };
            I32 Width{};
            I32 Height{};
        };
        std::vector<FontCharToPack> m_fontCharsToPack;
    };
}