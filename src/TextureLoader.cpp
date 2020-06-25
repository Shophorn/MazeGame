/*=============================================================================
Leo Tamminen

Texture Loader

Todo(Leo):
    - use own allocator for loading, stbi apparently lets us do just that
=============================================================================*/
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(x) Assert((x))
#include "external/stb_image.h"

internal TextureAsset
make_texture_asset(Array<u32> pixels, s32 width, s32 height, s32 channels)
{
    TextureAsset result = 
    {
        .pixels     = std::move(pixels),
        .width      = width,
        .height     = height,
        .channels   = channels,
    };
    return result;
}

internal TextureAsset
load_texture_asset(const char * assetPath, MemoryArena * memoryArena)
{
    s32 width, height, channels;
    stbi_uc * stbi_pixels = stbi_load(assetPath, &width, &height, &channels, STBI_rgb_alpha);

    AssertMsg(stbi_pixels != nullptr, assetPath);

    auto begin  = reinterpret_cast<u32*>(stbi_pixels);
    auto end    = begin + (width * height);
    auto pixels = allocate_array<u32>(*memoryArena, begin, end);

    auto result = make_texture_asset(std::move(pixels), width, height, 4);

    stbi_image_free(stbi_pixels);

    return result;
}

#define STB_TRUETYPE_IMPLEMENTATION
// Todo(Leo): this causes errors in some if-elses, fix.
// #define STBTT_assert(x) Assert(x)
#include "external/stb_truetype.h"

#undef assert

internal Font
load_font(char const * fontFilePath)
{
    Array<byte> fontFile = read_binary_file(*global_transientMemory, fontFilePath);

    stbtt_fontinfo fontInfo;
    stbtt_InitFont(&fontInfo, fontFile.data(), stbtt_GetFontOffsetForIndex(fontFile.data(), 0));
    
    u8 firstCharacter = 32;
    u8 lastCharacter = 125;

    s32 characterSize = 64;

    s32 characterCount = lastCharacter - firstCharacter;
    s32 charactersPerDirection = (s32)ceil_f32(square_root_f32((f32)characterCount));

    s32 width = characterSize * charactersPerDirection;
    s32 height = characterSize * charactersPerDirection;

    u8 * monoColorBitmap = (u8*)allocate(*global_transientMemory, width * height, true);

    f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, 64.0f);

    s32 ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

    Font result         = {};
    result.spaceWidth   = 0.25;

    for (u8 i = 0; i < characterCount; ++i)
    {
        // https://github.com/justinmeiners/stb-truetype-example/blob/master/main.c

        u8 character = i + firstCharacter;

        s32 ix0, ix1, iy0, iy1;
        stbtt_GetCodepointBitmapBox(&fontInfo, character, scale, scale, &ix0, &iy0, &ix1, &iy1);

        s32 advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&fontInfo, character, &advanceWidth, &leftSideBearing);

        result.advanceWidths[i] = (advanceWidth * scale) / characterSize;
        result.leftSideBearings[i] = (leftSideBearing * scale) / characterSize;

        s32 x = i % charactersPerDirection;
        s32 y = i / charactersPerDirection;

        f32 u = (f32)x / charactersPerDirection;
        f32 v = (f32)y / charactersPerDirection;

        result.characterWidths[i] = (float)(ix1 - ix0) / characterSize;
        result.uvPositionsAndSizes[i].xy = {u, v};
        result.uvPositionsAndSizes[i].zw = {result.characterWidths[i] / charactersPerDirection, 1.0f / charactersPerDirection};

        s32 byteOffset = x * characterSize + y * charactersPerDirection * characterSize * characterSize;

        s32 extraRows = (ascent * scale) + iy0;
        byteOffset += extraRows * charactersPerDirection * characterSize;

        stbtt_MakeCodepointBitmap(&fontInfo, monoColorBitmap + byteOffset, ix1 - ix0, iy1 - iy0, width, scale, scale, character);
    }

    Array<u32> fontBitMap = allocate_array<u32>(*global_transientMemory, width * height, ALLOC_FILL);

    for (s32 y = 0; y < height; ++y)
    {
        for(s32 x = 0; x < width; ++x)
        {
            s32 index = x + y * width;
            u8 value = monoColorBitmap[index];
            fontBitMap[index] = value << 24 | 255 << 16 | 255 << 8 | 255;
        }
    }

    auto atlasAsset = make_texture_asset(std::move(fontBitMap), width, height, 4);
    result.atlasTexture = platformApi->push_gui_texture(platformGraphics, &atlasAsset);

    return result;
}