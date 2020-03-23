/*=============================================================================
Leo Tamminen

Texture Loader

Todo(Leo):
    - use own allocator for loading
=============================================================================*/
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

internal TextureAsset
make_texture_asset(Array<u32> pixels, s32 width, s32 height, s32 channels = 4)
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

    DEBUG_ASSERT(stbi_pixels != nullptr, assetPath);

    auto begin  = reinterpret_cast<u32*>(stbi_pixels);
    auto end    = begin + (width * height);
    auto pixels = allocate_array<u32>(*memoryArena, begin, end);

    auto result = make_texture_asset(std::move(pixels), width, height);

    stbi_image_free(stbi_pixels);

    return result;
}

