/*
Leo Tamminen

Platform aka in this case gpu assets
*/

/* Todo(Leo): not super cool to init random -1, we would like to memset arrays of these to zero. */
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

// Todo(Leo): this should not be needed
struct ModelHandle : AssetHandle {};

static_assert(sizeof(MeshHandle) == 8);
static_assert(sizeof(TextureHandle) == 8);
static_assert(sizeof(MaterialHandle) == 8);
static_assert(sizeof(ModelHandle) == 8);

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

internal TextureAssetData
make_texture_asset(u32 * pixelMemory, s32 width, s32 height, s32 channels)
{
    TextureAssetData result = 
    {
        .pixelMemory     = pixelMemory,
        .width      = width,
        .height     = height,
        .channels   = channels,
    };
    return result;
}

constexpr u64 texture_format_component_size(TextureFormat format)
{
	switch(format)
	{	
		case TextureFormat_u8_srgb: 	return sizeof(u8);
		case TextureFormat_u8_linear: 	return sizeof(u8);
		case TextureFormat_f32: 		return sizeof(f32);
	}
	// Todo(Leo): Add this back by moving this function elsewhere
	// Assert(false && "Invalid texture formmat");
	return 0;	
}

u64 texture_asset_data_get_memory_size(TextureAssetData & asset)
{
	// Todo(Leo): Add this back by moving this function elsewhere
	// Assert(asset.channels == 4);

	u64 pixelCount 		= asset.width * asset.height;
	u64 componenCount 	= pixelCount * asset.channels;
	u64 componentSize  	= texture_format_component_size(asset.format);

	u64 memorySize 		=  componentSize * componenCount;
	return memorySize;
}

struct FontCharacterInfo
{
	// Todo(Leo): Kerning
	f32 leftSideBearing;
	f32 advanceWidth;
	f32 characterWidth;
	v2 	uvPosition;
	v2 	uvSize;
};

// Note(Leo): This is saved to asset pack file, so we must be confident about its layout
static_assert(sizeof(FontCharacterInfo) == 7 * sizeof(f32));
	// Todo(Leo): Add this back
// static_assert(std::is_standard_layout_v<FontCharacterInfo>);

struct Font
{
	static constexpr u8 firstCharacter 	= 0;
	static constexpr s32 characterCount = 128;

	MaterialHandle atlasTexture;
	FontCharacterInfo characters[characterCount];
};

// Todo(Leo): This maybe does not belong here, I just dumped it here
enum CharacterSkeletonBone : s32
{
	CharacterSkeletonBone_root,
	CharacterSkeletonBone_hip,
	CharacterSkeletonBone_back,
	CharacterSkeletonBone_neck,
	CharacterSkeletonBone_head,
	CharacterSkeletonBone_left_arm,
	CharacterSkeletonBone_left_forearm,
	CharacterSkeletonBone_left_hand,
	CharacterSkeletonBone_right_arm,
	CharacterSkeletonBone_right_forearm,
	CharacterSkeletonBone_right_hand,
	CharacterSkeletonBone_left_thigh,
	CharacterSkeletonBone_left_shin,
	CharacterSkeletonBone_left_foot,
	CharacterSkeletonBone_right_thigh,
	CharacterSkeletonBone_right_shin,
	CharacterSkeletonBone_right_foot,

	CharacterSkeletonBoneCount,
	CharacterSkeletonBone_invalid,
};
