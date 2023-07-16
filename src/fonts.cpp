#include "retgui/fonts.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <fstream>

namespace retgui
{
    constexpr auto WhitePixelSize = 6;

    int NextPowerOf2(int n)
    {
        n |= (n >> 16);
        n |= (n >> 8);
        n |= (n >> 4);
        n |= (n >> 2);
        n |= (n >> 1);
        ++n;
        return n;
    }

    static auto load_font_data(const std::string& fontFilename) -> std::vector<std::uint8_t>
    {
        std::ifstream file(fontFilename, std::ios::binary | std::ios::ate);

        std::int64_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<std::uint8_t> buffer(fileSize);
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

        file.close();
        return buffer;
    }

    auto Font::calc_text_size(const std::string& text) -> Vec2
    {
        return {};
    }

    auto Fonts::add_font_from_file(const std::string& fontFilename, float fontSize, std::vector<CharsetRange> charsetRanges) -> Font*
    {
        if (charsetRanges.empty())
        {
            // Add latin characters by default
            charsetRanges = {
                { 0x0020, 0x00FF },  // Basic Latin + Latin Supplement
            };
        }

        m_fonts.push_back(std::make_unique<Font>());
        auto& font = m_fonts.back();

        auto fontData = load_font_data(fontFilename);
        stbtt_fontinfo sbttFontInfo{};
        if (!stbtt_InitFont(&sbttFontInfo, fontData.data(), 0))
        {
            throw std::runtime_error("Failed to init font.");
        }

        font->FontSize = fontSize;
        const auto scale = stbtt_ScaleForPixelHeight(&sbttFontInfo, fontSize);

        std::int32_t ascent{};
        std::int32_t descent{};
        std::int32_t lineGap{};
        stbtt_GetFontVMetrics(&sbttFontInfo, &ascent, &descent, &lineGap);

        font->Ascender = std::roundf(float(ascent) * scale);    // Scale this
        font->Descender = std::roundf(float(descent) * scale);  // Scale this
        font->LineGap = std::roundf(float(lineGap) * scale);    // Scale this
        font->LineSpacing = font->Ascender - font->Descender + font->LineGap;

        std::int32_t highestGlyph{};
        for (const auto& charsetRange : charsetRanges)
        {
            highestGlyph = std::max(highestGlyph, charsetRange.End);
        }
        font->glyphs.resize(highestGlyph + 1);

        for (const auto& charsetRange : charsetRanges)
        {
            for (std::int32_t i = charsetRange.Begin; i <= charsetRange.End; ++i)
            {
                if (!stbtt_FindGlyphIndex(&sbttFontInfo, i))
                {
                    // Codepoint/Glyph is not in the font.
                    continue;
                }

                auto& glyph = font->glyphs[i];

                std::int32_t advance{};
                std::int32_t leftBearing{};
                stbtt_GetCodepointHMetrics(&sbttFontInfo, i, &advance, &leftBearing);

                glyph.AdvanceX = std::roundf(float(advance) * scale);  // Scale this

                // Get glyph bounding box (might be offset for chars that dip above/below the line)
                stbtt_GetCodepointBitmapBox(&sbttFontInfo, i, scale, scale, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);

                std::int32_t bitmapWidth{};
                std::int32_t bitmapHeight{};
                std::int32_t xOffset{};
                std::int32_t yOffset{};
                auto* bitmap = stbtt_GetCodepointBitmap(&sbttFontInfo, scale, scale, i, &bitmapWidth, &bitmapHeight, &xOffset, &yOffset);

                auto& charToPack = m_fontCharsToPack.emplace_back();
                charToPack.FontIdx = m_fonts.size() - 1;
                charToPack.CodePoint = i;
                charToPack.Bitmap = bitmap;
                charToPack.Width = bitmapWidth;
                charToPack.Height = bitmapHeight;
            }
        }

        return font.get();
    }

    void Fonts::get_texture_data_as_alpha8(std::vector<U8>& outPixels, U32& outWidth, U32& outHeight)
    {
        std::vector<stbrp_rect> packedRects{};

        for (auto i = 0; i < m_fontCharsToPack.size(); ++i)
        {
            const auto& fontCharToPack = m_fontCharsToPack[i];

            auto& packedRect = packedRects.emplace_back();
            packedRect.id = i;
            packedRect.w = fontCharToPack.Width;
            packedRect.h = fontCharToPack.Height;
        }

        // Add white pixels
        {
            auto& packedWhitePixels = packedRects.emplace_back();
            packedWhitePixels.w = WhitePixelSize;
            packedWhitePixels.h = WhitePixelSize;
        }

        auto atlasWidth = 128;
        auto atlasHeight = 128;
        stbrp_context rpContext{};
        std::vector<stbrp_node> nodes(NextPowerOf2(atlasWidth));
        stbrp_init_target(&rpContext, atlasWidth, atlasHeight, nodes.data(), nodes.size());
        while (!stbrp_pack_rects(&rpContext, packedRects.data(), packedRects.size()))
        {
            atlasWidth = NextPowerOf2(atlasWidth);
            atlasHeight = NextPowerOf2(atlasHeight);
            nodes.assign(NextPowerOf2(atlasWidth), {});
            stbrp_init_target(&rpContext, atlasWidth, atlasHeight, nodes.data(), nodes.size());
        }

        outWidth = atlasWidth;
        outHeight = atlasHeight;
        outPixels = std::vector<U8>(atlasWidth * atlasHeight, 0);
        for (auto i = 0; i < m_fontCharsToPack.size(); ++i)
        {
            const auto& packedFontChar = m_fontCharsToPack[i];
            const auto& packedRect = packedRects[i];

            auto& font = m_fonts[packedFontChar.FontIdx];
            auto& glyph = font->glyphs[packedFontChar.CodePoint];
            // UVs should TL -> BR
            glyph.ux0 = float(packedRect.x) / float(atlasWidth);
            glyph.uy0 = float(atlasHeight - packedRect.y) / float(atlasHeight);
            glyph.ux1 = glyph.ux0 + (float(packedRect.w) / float(atlasWidth));
            glyph.uy1 = glyph.uy0 - (float(packedRect.h) / float(atlasHeight));

            for (int y = 0; y < packedRect.h; ++y)
            {
                for (int x = 0; x < packedRect.w; ++x)
                {
                    auto atlasIndex = (packedRect.x + x) + (packedRect.y + y) * atlasWidth;
                    outPixels[atlasIndex] = packedFontChar.Bitmap[x + y * packedRect.w];
                }
            }

            stbtt_FreeBitmap(packedFontChar.Bitmap, nullptr);
        }

        // Add white pixels
        {
            const auto& packedWhitePixels = packedRects.back();
            for (int y = 0; y < packedWhitePixels.h; ++y)
            {
                for (int x = 0; x < packedWhitePixels.w; ++x)
                {
                    auto atlasIndex = (packedWhitePixels.x + x) + (packedWhitePixels.y + y) * atlasWidth;
                    outPixels[atlasIndex] = 255;  // White
                }
            }

            m_whitePixelCoords = {
                (float(packedWhitePixels.x) + float(packedWhitePixels.w) * 0.5f) / float(atlasWidth),
                (float(atlasHeight - packedWhitePixels.y) + float(packedWhitePixels.h) * 0.5f) / float(atlasHeight),
            };
        }

        // Flip atlas pixels vertically
        for (auto y = 0; y < atlasHeight / 2; ++y)
        {
            for (auto x = 0; x < atlasWidth; ++x)
            {
                auto temp = outPixels[x + y * atlasWidth];
                outPixels[x + y * atlasWidth] = outPixels[x + ((atlasHeight - 1) * atlasWidth) - y * atlasWidth];
                outPixels[x + ((atlasHeight - 1) * atlasWidth) - y * atlasWidth] = temp;
            }
        }
    }

    void Fonts::get_texture_data_as_rgba32(std::vector<U32>& outPixels, U32& outWidth, U32& outHeight)
    {
        std::vector<U8> pixelsU8;
        get_texture_data_as_alpha8(pixelsU8, outWidth, outHeight);
        if (!pixelsU8.empty())
        {
            outPixels.resize(outWidth * outHeight * 4);
            const U8* src = pixelsU8.data();
            U32* dst = outPixels.data();
            for (auto i = 0; i < outWidth * outHeight; i++)
            {
                *dst = RETGUI_COL32(255, 255, 255, (*src));
                src++;
                dst++;
            }
        }
    }

    void Fonts::set_tex_id(TexId texture)
    {
        m_texture = texture;
    }

}