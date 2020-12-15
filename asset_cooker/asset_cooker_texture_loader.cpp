/*=============================================================================
Leo Tamminen

Texture Loader

Todo(Leo):
	- use own allocator for loading, stbi apparently lets us do just that
=============================================================================*/
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

internal TextureAssetData asset_cooker_load_texture(const char * filename)
{
	s32 width, height, channels;
	stbi_uc * stbi_pixels = stbi_load(filename, &width, &height, &channels, STBI_rgb_alpha);

	assert(stbi_pixels != nullptr);

	u64 pixelCount      = width      * height;
	u64 pixelMemorySize = pixelCount * channels;

	TextureAssetData result = {};
	result.pixelMemory = allocate<u8>(pixelMemorySize);
	memory_copy(result.pixelMemory, stbi_pixels, pixelMemorySize);

	result.width    = width;
	result.height   = height;
	result.channels = 4;

	stbi_image_free(stbi_pixels);

	return result;
}

#if 1

#define STB_TRUETYPE_IMPLEMENTATION
#include "external/stb_truetype.h"

struct FontLoadResult
{
	Font                font;
	TextureAssetData    atlasTextureAsset;
};

internal FontLoadResult asset_cooker_load_font(char const * fontFilePath)
{
	// Array2<byte> fontFile = read_binary_file(*global_transientMemory, fontFilePath);

	s64 fontFileSize;
	u8 * fontFileMemory;
	{
		HANDLE file = CreateFileA(fontFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, nullptr);
		LARGE_INTEGER fileSize;
		GetFileSizeEx(file, &fileSize);

		fontFileSize = fileSize.QuadPart;
		fontFileMemory = allocate<u8>(fontFileSize);

		ReadFile(file, fontFileMemory, (DWORD)fontFileSize, nullptr, nullptr);
	}

	stbtt_fontinfo fontInfo;
	stbtt_InitFont(&fontInfo, fontFileMemory, stbtt_GetFontOffsetForIndex(fontFileMemory, 0));
	
	s32 characterSize           = 64;
	s32 charactersPerDirection  = (s32)ceil_f32(square_root_f32((f32)Font::characterCount));

	s32 width   = characterSize * charactersPerDirection;
	s32 height  = characterSize * charactersPerDirection;

	s64 memorySize = width * height;
	u8 * monoColorBitmap = allocate<u8>(memorySize);

	f32 scale = stbtt_ScaleForPixelHeight(&fontInfo, 64.0f);

	s32 ascent, descent, lineGap;
	stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);

	Font font         = {};

	for (u8 i = 0; i < Font::characterCount; ++i)
	{
		// https://github.com/justinmeiners/stb-truetype-example/blob/master/main.c

		u8 character = i + Font::firstCharacter;

		s32 ix0, ix1, iy0, iy1;
		stbtt_GetCodepointBitmapBox(&fontInfo, character, scale, scale, &ix0, &iy0, &ix1, &iy1);

		s32 advanceWidth, leftSideBearing;
		stbtt_GetCodepointHMetrics(&fontInfo, character, &advanceWidth, &leftSideBearing);

		FontCharacterInfo info = {};
		info.advanceWidth = (advanceWidth * scale) / characterSize;
		info.leftSideBearing = (leftSideBearing * scale) / characterSize;

		s32 x = i % charactersPerDirection;
		s32 y = i / charactersPerDirection;

		f32 u = (f32)x / charactersPerDirection;
		f32 v = (f32)y / charactersPerDirection;

		info.characterWidth = (float)(ix1 - ix0) / characterSize;
		info.uvPosition     = {u, v};
		info.uvSize         = {info.characterWidth / charactersPerDirection, 1.0f / charactersPerDirection};

		font.characters[i] = info;

		s32 byteOffset = x * characterSize + y * charactersPerDirection * characterSize * characterSize;

		s32 extraRows = (ascent * scale) + iy0;
		byteOffset += extraRows * charactersPerDirection * characterSize;

		stbtt_MakeCodepointBitmap(&fontInfo, monoColorBitmap + byteOffset, ix1 - ix0, iy1 - iy0, width, scale, scale, character);
	}

	u32 * fontBitMap = allocate<u32>(width * height);

	for (s32 y = 0; y < height; ++y)
	{
		for(s32 x = 0; x < width; ++x)
		{
			s32 index = x + y * width;
			u8 value = monoColorBitmap[index];
			fontBitMap[index] = value << 24 | 255 << 16 | 255 << 8 | 255;
		}
	}

	TextureAssetData atlasAsset = {};
	atlasAsset.pixelMemory 		= fontBitMap;
	atlasAsset.width 			= width;
	atlasAsset.height 			= height;
	atlasAsset.channels 		= 4;

	// Todo(Leo): we would probably like to have clamp address mode, but it did not seem to work right
	atlasAsset.addressMode 		= TextureAddressMode_repeat;
	atlasAsset.format 			= TextureFormat_u8_srgb;

	FontLoadResult result = 
	{
		.font               = font,
		.atlasTextureAsset  = atlasAsset
	};

	free(fontFileMemory);
	free(monoColorBitmap);

	return result;
}


#endif
