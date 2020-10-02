#pragma pack(push, 1)
struct AssetFileTexture
{
	u64 dataOffset;

	u32 width;
	u32 height;
	u32 channels;
};

struct AssetFileMesh
{
	u64 dataOffset;

	bool32 hasSkinning;
	
	u32 vertexCount;
	u32 indexCount;
};

struct AssetFileAnimation
{
	u64 dataOffset;

	
};

struct AssetFileHeader
{
	static constexpr u32 magicValue 	= 0x66736166;
	static constexpr u32 currentVersion = 0;

	u32 magic;
	u32 version;

	AssetFileTexture textures [TEXTURE_ASSET_COUNT];
	AssetFileMesh meshes [MESH_ASSET_COUNT];
};
#pragma pack(pop)
