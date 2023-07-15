#include "retgui/fonts.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <fstream>

namespace retgui
{
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
        struct FontCharPackedRect
        {
            U32 fontIdx{};
            U32 codePoint{};
            U32 packedIndex{};
        };
        std::vector<stbrp_rect> packedRects{};

        for (auto i = 0; i < m_fontCharsToPack.size(); ++i)
        {
            const auto& fontCharToPack = m_fontCharsToPack[i];

            const auto packedRectIdx = packedRects.size();
            auto& packedRect = packedRects.emplace_back();
            packedRect.id = i;
            packedRect.w = fontCharToPack.Width;
            packedRect.h = fontCharToPack.Height;
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
        for (const auto& packedRect : packedRects)
        {
            const auto& packedFontChar = m_fontCharsToPack[packedRect.id];

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

}