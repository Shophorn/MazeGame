/*
Leo Tamminen

Platform aka in this case gpu assets
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

// Todo(Leo): these 2 should not be needed
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

enum MeshIndexType : s32
{
	MeshIndexType_uint16,
	MeshIndexType_uint32,
};

struct MeshAssetData
{
	s64 				vertexCount;
	Vertex * 			vertices;
	VertexSkinData * 	skinning;

	s64 	indexCount;
	u16 * 	indices;

	MeshIndexType indexType = MeshIndexType_uint16;

	/*
	TODO(Leo): would this be a good way to deal with different index types?
	u16 * indices_u16() { Assert(indexType == IndexType::UInt16); return reinterpret_cast<u16*>(indices); }
	u32 * indices_u32() { Assert(indexType == IndexType::UInt32); return reinterpret_cast<u32*>(indices); }
	*/
};

enum TextureAddressMode : s32
{
	TextureAddressMode_repeat,
	TextureAddressMode_clamp,
};

enum TextureFormat : s32
{
	TextureFormat_u8_srgb,
	TextureFormat_u8_linear,
	TextureFormat_f32,
};

struct TextureAssetData
{
	void * 	pixelMemory;

	s32 	width;
	s32 	height;
	s32 	channels;

	TextureAddressMode 	addressMode;
	TextureFormat 		format;
};

constexpr u64 texture_format_component_size(TextureFormat format)
{
	switch(format)
	{	
		case TextureFormat_u8_srgb: 	return sizeof(u8);
		case TextureFormat_u8_linear: 	return sizeof(u8);
		case TextureFormat_f32: 		return sizeof(f32);
	}
	Assert(false && "Invalid texture formmat");
	return 0;	
}

u64 texture_asset_data_get_memory_size(TextureAssetData & asset)
{
	Assert(asset.channels == 4);

	u64 pixelCount 		= asset.width * asset.height;
	u64 componenCount 	= pixelCount * asset.channels;
	u64 componentSize  	= texture_format_component_size(asset.format);

	u64 memorySize 		=  componentSize * componenCount;
	return memorySize;
}

