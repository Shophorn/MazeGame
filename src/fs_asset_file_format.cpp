#pragma pack(push, 1)
struct AssetFileTexture
{
	u64 dataOffset;

	u32 width;
	u32 height;
	u32 channels;

	TextureFormat 		format;
	TextureAddressMode 	addressMode;
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

	s32 channelCount;
	f32 duration;

	u64 totalTranslationKeyframeCount;
	u64 totalRotationKeyframeCount;
};

struct AssetFileSkeleton
{
	u64 dataOffset;
	s32 boneCount;
};

struct AssetFileSound
{
	u64 dataOffset;
	s32 sampleCount;
};

// Note(Leo): font contains texture, so it is same.
// We just write character info after texture
using AssetFileFont = AssetFileTexture;

constexpr u32 asset_file_magic_value 		= 0x66736166;
constexpr u32 asset_file_current_version 	= 1;

// struct AssetFileHeader
// {
// 	u32 magic;
// 	u32 version;

// 	AssetFileTexture 	textures [TextureAssetIdCount];
// 	AssetFileMesh 		meshes [MeshAssetIdCount];
// 	AssetFileAnimation 	animations [AnimationAssetIdCount];
// 	AssetFileSkeleton 	skeletons [SkeletonAssetIdCount];
// 	AssetFileSound		sounds [SoundAssetIdCount];
// 	AssetFileFont 		fonts [FontAssetIdCount];
// };

struct AssetTypeHeaderInfo
{
	u64 offsetToFirst;
	u64 count;
};

struct AssetFileHeader2
{
	u32 magic;
	u32 version;

	// u64 offsetToTextures;
	// u64 offsetToMeshes;
	// u64 offsetToAnimations;
	// u64 offsetToSkeletons;
	// u64 offsetToSounds;
	// u64 offsetToFonts;

	AssetTypeHeaderInfo	 textures;
	AssetTypeHeaderInfo	 meshes;
	AssetTypeHeaderInfo	 animations;
	AssetTypeHeaderInfo	 skeletons;
	AssetTypeHeaderInfo	 sounds;
	AssetTypeHeaderInfo	 fonts;
};
#pragma pack(pop)
