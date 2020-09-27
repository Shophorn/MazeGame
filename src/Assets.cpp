/*
Leo Tamminen

Asset things for Friendsimulator
*/

struct AssetHandle
{
	s64 index =  -1;

	operator s64() const
	{
		return index;
	}	
};

struct MeshHandle : AssetHandle {};
struct TextureHandle : AssetHandle {};
struct MaterialHandle : AssetHandle {};
struct ModelHandle : AssetHandle {};
struct GuiTextureHandle : AssetHandle {};

static_assert(sizeof(MeshHandle) == 8);
static_assert(sizeof(TextureHandle) == 8);
static_assert(sizeof(MaterialHandle) == 8);
static_assert(sizeof(ModelHandle) == 8);
static_assert(sizeof(GuiTextureHandle) == 8);

bool32 is_valid_handle(AssetHandle handle)
{
	return handle >= 0;
}

struct Vertex
{
	v3 position;
	v3 normal;
	v3 tangent;
	v2 texCoord;
};

struct VertexSkinData
{
	u32 boneIndices[4];
	f32 boneWeights[4];	
};

enum struct IndexType : u32 { UInt16, UInt32 };

struct MeshAssetData
{
	s64 				vertexCount;
	Vertex * 			vertices;
	VertexSkinData * 	skinning;

	s64 	indexCount;
	u16 * 	indices;

	IndexType indexType = IndexType::UInt16;

	/*
	TODO(Leo): would this be a good way to deal with different index types?
	u16 * indices_u16() { Assert(indexType == IndexType::UInt16); return reinterpret_cast<u16*>(indices); }
	u32 * indices_u32() { Assert(indexType == IndexType::UInt32); return reinterpret_cast<u32*>(indices); }
	*/
};

enum TextureAddressMode : s32
{
	TEXTURE_ADDRESS_MODE_REPEAT,
	TEXTURE_ADDRESS_MODE_CLAMP,
};

enum TextureFormat : s32
{
	TEXTURE_FORMAT_U8_SRGB,
	TEXTURE_FORMAT_U8_LINEAR,
	TEXTURE_FORMAT_F32,
};

constexpr u64 get_texture_component_size(TextureFormat format)
{
	switch(format)
	{	
		case TEXTURE_FORMAT_U8_SRGB: 	return sizeof(u8);
		case TEXTURE_FORMAT_U8_LINEAR: 	return sizeof(u8);
		case TEXTURE_FORMAT_F32: 		return sizeof(f32);
	}
	Assert(false && "Invalid texture formmat");
	return 0;	
}

struct TextureAssetData
{
	void * 	pixelMemory;

	s32 	width;
	s32 	height;
	s32 	channels;

	TextureAddressMode 	addressMode;
	TextureFormat 		format;
};

u32 * get_u32_pixel_memory(TextureAssetData & asset)
{
	Assert(asset.format == TEXTURE_FORMAT_U8_SRGB || asset.format == TEXTURE_FORMAT_U8_LINEAR);
	return (u32*)asset.pixelMemory;
}

u64 get_texture_asset_memory_size(TextureAssetData & asset)
{
	Assert(asset.channels == 4);

	u64 pixelCount 		= asset.width * asset.height;
	u64 componenCount 	= pixelCount * asset.channels;
	u64 componentSize  	= get_texture_component_size(asset.format);

	u64 memorySize 		=  componentSize * componenCount;
	return memorySize;
}

inline u32 get_pixel(TextureAssetData * texture, u32 x, u32 y)
{
	AssertMsg(x < texture->width && y < texture->height, "Invalid pixel coordinates!");

	s64 index = x + y * texture->width;
	return get_u32_pixel_memory(*texture)[index];
}

inline u8
value_to_byte(float value)
{
	return (u8)(value * 255);
}

internal u32 colour_rgba_u32(v4 color)
{
	u32 pixel = ((u32)(color.r * 255))
				| (((u32)(color.g * 255)) << 8)
				| (((u32)(color.b * 255)) << 16)
				| (((u32)(color.a * 255)) << 24);
	return pixel;
}

internal v4 colour_rgba_v4(u32 pixel)
{
	v4 colour =
	{
		(f32)((u8)pixel) / 255.0f,
		(f32)((u8)(pixel >> 8)) / 255.0f,
		(f32)((u8)(pixel >> 16)) / 255.0f,
		(f32)((u8)(pixel >> 24)) / 255.0f,
	};


	// log_debug(0) << "colour_rgba_v4(): " << pixel << ", " << colour;
	// log_debug(0, "colour_rgba_v4(): ", pixel, ", ", colour);
	return colour;
}

internal constexpr v4 colour_v4_from_rgb_255(u8 r, u8 g, u8 b)
{
	v4 colour = {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
	return colour;
}

internal constexpr v4 colour_rgb_alpha(v3 rgb, f32 alpha)
{
	v4 colour = {rgb.x, rgb.y, rgb.z, alpha};
	return colour;
}

internal u32 colour_rgb_alpha_32(v3 colour, f32 alpha)
{
	return colour_rgba_u32(colour_rgb_alpha(colour, alpha));
}

internal v4 colour_multiply(v4 a, v4 b)
{
	a = {	a.r * b.r,
			a.g * b.g,
			a.b * b.b,
			a.a * b.a};
	return a;
}
