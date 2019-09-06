/*=============================================================================
Leo Tamminen

Texture Loader
=============================================================================*/
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

internal TextureAsset
LoadTextureAsset(const char * assetPath, MemoryArena * memoryArena)
{
    TextureAsset resultTexture = {};
    stbi_uc * pixels = stbi_load(assetPath, &resultTexture.width, &resultTexture.height,
                                        &resultTexture.channels, STBI_rgb_alpha);

    if(pixels == nullptr)
    {
        // Todo[Error](Leo): Proper handling and logging
        throw std::runtime_error("Failed to load image");
    }

    // rgba channels
    resultTexture.channels = 4;

    int32 pixelCount = resultTexture.width * resultTexture.height;
    resultTexture.pixels = PushArray<uint32>(memoryArena, pixelCount);

    uint64 imageMemorySize = resultTexture.width * resultTexture.height * resultTexture.channels;

    // for (int v = 0; v < resultTexture.height; ++v)
    // {
    //     for (int u = 0; u < resultTexture.height; ++v)
    //     {
            
    //     }
    // }



    memcpy((uint8*)resultTexture.pixels.memory, pixels, imageMemorySize);

    stbi_image_free(pixels);
    return resultTexture;
}