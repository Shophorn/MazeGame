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

struct AssetFileAudio
{
	u64 dataOffset;
	s32 sampleCount;
};

// Note(Leo): font contains texture, so it is same.
// We just write character info after texture
using AssetFileFont = AssetFileTexture;


struct AssetFileHeader
{
	static constexpr u32 magicValue 	= 0x66736166;
	static constexpr u32 currentVersion = 1;

	u32 magic;
	u32 version;

	AssetFileTexture 	textures [TextureAssetIdCount];
	AssetFileMesh 		meshes [MeshAssetIdCount];
	AssetFileAnimation 	animations [AnimationAssetIdCount];
	AssetFileSkeleton 	skeletons [SkeletonAssetIdCount];
	AssetFileAudio		audios [AudioAssetIdCount];
	AssetFileFont 		fonts [FontAssetIdCount];
};
#pragma pack(pop)
