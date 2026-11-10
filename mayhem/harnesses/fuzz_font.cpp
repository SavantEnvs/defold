// Fuzzes Defold's TrueType/OpenType loader: table parsing, cmap lookup, glyph outlines, SDF/bitmap
// generation and COLR/CPAL data, through the public font API.
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dmsdk/font/font.h>
#include <font_ttf.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    void* mem = malloc(size ? size : 1);
    if (size)
        memcpy(mem, data, size);

    HFont font = FontLoadFromMemory("fuzz.ttf", mem, (uint32_t)size, false);
    if (!font)
    {
        free(mem);
        return -1;
    }

    float scale = FontGetScaleFromSize(font, 24.0f);
    (void)FontGetAscent(font, scale);
    (void)FontGetDescent(font, scale);
    (void)FontGetLineGap(font, scale);
    (void)FontGetResourceSize(font);
    (void)FontGetColrTTF(font);

    static const uint32_t codepoints[] = {0x20, 0x41, 0x67, 0xC5, 0x627, 0x263A, 0x4E2D, 0x1F600};
    for (uint32_t i = 0; i < sizeof(codepoints) / sizeof(codepoints[0]); ++i)
    {
        uint32_t index = FontGetGlyphIndex(font, codepoints[i]);
        for (int variant = 0; variant < 3; ++variant)
        {
            FontGlyphOptions options;
            options.m_Scale = scale;
            options.m_GenerateImage = variant == 1;
            options.m_GenerateOutline = variant == 2;
            options.m_UseUnpaddedBounds = variant == 0;
            FontGlyph glyph;
            memset(&glyph, 0, sizeof(glyph));
            if (FontGetGlyphByIndex(font, index, &options, &glyph) == FONT_RESULT_OK)
                FontFreeGlyph(font, &glyph);
        }
    }
    for (uint32_t index = 0; index < 6; ++index)
    {
        int32_t x0, y0, x1, y1;
        (void)FontGetGlyphBoxTTF(font, index, &x0, &y0, &x1, &y1);
        FontGlyph glyph;
        memset(&glyph, 0, sizeof(glyph));
        (void)FontGetGlyphSDFMetricsTTF(font, index, scale, 4.0f, &glyph);
    }

    FontDestroy(font);
    free(mem);
    return 0;
}
