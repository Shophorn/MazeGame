/*
Leo Tamminen

Asset cooking (sometimes baking) utility.

Todo(Leo):
	in actual game we use custom memory allocation things, but here we could use 
	std things also, so maybe use std::vector here for memory management.
*/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cassert>
#include <direct.h>

#define NOMINMAX
#include <windows.h>

#include "fs_standard_types.h"
#include "fs_standard_functions.h"
#include "platform_assets.cpp"

#include "Transform3D.cpp"

#include "animations.hpp"


#define EXPORT extern "C" __declspec(dllexport)

// Todo(Leo): make and use headers, so we actually get types right, if we change something
// Note(leo): or maybe not, this is just offline data loading and it doesn't need to directly mirror runtime
struct AnimatedBone
{
	// Properties
	Transform3D boneSpaceDefaultTransform;
	m44 		inverseBindMatrix;
	s32 		parent;
};

struct AnimatedSkeleton
{
	s32 			bonesCount;
	AnimatedBone * 	bones;
};

#include "fs_asset_file_format.cpp"

// Note(Leo): this is just a typed wrapper over malloc, nothing too fancy
// we could have used new[] and delete [], but then we would have to always
// remember if pointer was allocated with new or new [], this doesn't care
template <typename T>
T * allocate(s32 count)
{
	return reinterpret_cast<T*>(malloc(sizeof(T) * count));
}

#include "asset_cooker_gltf_loader.cpp"
#include "asset_cooker_texture_loader.cpp"

#include "AudioFile.cpp"

using asset_underlying_type = s32;

static std::ofstream 	global_outFile;
static u64 				global_dataPosition;

static AssetFileHeader2		global_header_2;
static AssetFileTexture *	global_textureHeaders;
static AssetFileMesh *		global_meshHeaders;
static AssetFileAnimation *	global_animationHeaders;
static AssetFileSkeleton *	global_skeletonHeaders;
static AssetFileSound *		global_soundHeaders;
static AssetFileFont *		global_fontHeaders;

static void write(u64 position, u64 size, void * memory)
{
	global_outFile.seekp(position);
	global_outFile.write((char*)memory, size);
}

struct AssetCounts
{
	s32 texture;
	s32 mesh;
	s32 animation;
	s32 skeleton;
	s32 sound;
	s32 font;
};

EXPORT void cook_initialize(char const * filename, AssetCounts & assetCounts)
{
	std::cout << "This is asset cooker DLL\n";
	global_outFile = std::ofstream(filename, std::ios::out | std::ios::binary);

	global_header_2 		= {};
	global_header_2.magic 	= asset_file_magic_value;
	global_header_2.version = asset_file_current_version;

	u64 dataPosition = sizeof(global_header_2);

	auto init_header_info = [&dataPosition](auto & info, u64 count, u64 elementSize)
	{
		info 				= {};
		info.offsetToFirst 	= dataPosition;
		info.count 			= count;

		dataPosition += count * elementSize;
	};

	init_header_info(global_header_2.textures, 		assetCounts.texture, 	sizeof(AssetFileTexture));
	init_header_info(global_header_2.meshes, 		assetCounts.mesh, 		sizeof(AssetFileMesh));
	init_header_info(global_header_2.animations, 	assetCounts.animation, 	sizeof(AssetFileAnimation));
	init_header_info(global_header_2.skeletons, 	assetCounts.skeleton, 	sizeof(AssetFileSkeleton));
	init_header_info(global_header_2.sounds, 		assetCounts.sound, 		sizeof(AssetFileSound));
	init_header_info(global_header_2.fonts, 		assetCounts.font, 		sizeof(AssetFileFont));

	global_textureHeaders 	= allocate<AssetFileTexture>(assetCounts.texture);
	global_meshHeaders 		= allocate<AssetFileMesh>(assetCounts.mesh);
	global_animationHeaders = allocate<AssetFileAnimation>(assetCounts.animation);
	global_skeletonHeaders 	= allocate<AssetFileSkeleton>(assetCounts.skeleton);
	global_soundHeaders 	= allocate<AssetFileSound>(assetCounts.sound);
	global_fontHeaders 		= allocate<AssetFileFont>(assetCounts.font);

	global_dataPosition 	= dataPosition;
}

EXPORT void cook_complete()
{
	write(0, sizeof(AssetFileHeader2), &global_header_2);

	write(global_header_2.textures.offsetToFirst, 	sizeof(AssetFileTexture) * global_header_2.textures.count, 		global_textureHeaders);	
	write(global_header_2.meshes.offsetToFirst, 	sizeof(AssetFileMesh) * global_header_2.meshes.count, 			global_meshHeaders);	
	write(global_header_2.animations.offsetToFirst, sizeof(AssetFileAnimation) * global_header_2.animations.count, 	global_animationHeaders);	
	write(global_header_2.skeletons.offsetToFirst, 	sizeof(AssetFileSkeleton) * global_header_2.skeletons.count, 	global_skeletonHeaders);	
	write(global_header_2.sounds.offsetToFirst, 	sizeof(AssetFileSound) * global_header_2.sounds.count, 			global_soundHeaders);	
	write(global_header_2.fonts.offsetToFirst, 		sizeof(AssetFileFont) * global_header_2.fonts.count, 			global_fontHeaders);	

	std::cout << "global_header written to file \n";
	global_outFile.close();

	free (global_textureHeaders);
	free (global_meshHeaders);
	free (global_animationHeaders);
	free (global_skeletonHeaders);
	free (global_soundHeaders);
	free (global_fontHeaders);

}


EXPORT void cook_texture(asset_underlying_type id, char const * filename, TextureFormat format)
{
	TextureAssetData data 		= asset_cooker_load_texture(filename);
	AssetFileTexture & header 	= global_textureHeaders[id];

	header.dataOffset 	= global_dataPosition;
	header.width 		= data.width;
	header.height 		= data.height;
	header.channels 	= data.channels;

	header.format 		= format;
	header.addressMode 	= TextureAddressMode_repeat;

	assert(header.channels == 4);

	u64 memorySize 			= data.width * data.height * data.channels;
	u64 filePosition 		= global_dataPosition;
	global_dataPosition 	+= memorySize;

	write(filePosition, memorySize, data.pixelMemory);

	std::cout << "COOK TEXTURE: " << filename << ", " << memorySize << " bytes at " << filePosition << "\n";

	free(data.pixelMemory);
}

EXPORT void cook_mesh(asset_underlying_type id, char const * filename, char const * gltfNodeName)
{
	MeshAssetData data = asset_cooker_load_mesh_glb(filename, gltfNodeName);

	global_meshHeaders[id].dataOffset 	= global_dataPosition;
	global_meshHeaders[id].vertexCount 	= data.vertexCount;
	global_meshHeaders[id].indexCount 	= data.indexCount;
	
	bool32 hasSkinning 						= data.skinning != nullptr;
	global_meshHeaders[id].hasSkinning 	= hasSkinning;

	u64 filePosition = global_dataPosition;

	u64 vertexMemorySize = sizeof(Vertex) * data.vertexCount;
	write(filePosition, vertexMemorySize, data.vertices);
	filePosition += vertexMemorySize;

	u64 skinMemorySize = hasSkinning ? sizeof(VertexSkinData) * data.vertexCount : 0;
	write(filePosition, skinMemorySize, data.skinning);
	filePosition += skinMemorySize;

	u64 indexMemorySize = sizeof(u16) * data.indexCount;
	write(filePosition, indexMemorySize, data.indices);
	filePosition += indexMemorySize;

	std::cout << "COOK MESH: " << filename << ", " << gltfNodeName << ": " << (filePosition - global_dataPosition) << " bytes at " << global_dataPosition << "\n";

	global_dataPosition = filePosition;

	free(data.vertices);
	if (data.skinning)
		free(data.skinning);
	free(data.indices);
}

EXPORT void cook_animation(asset_underlying_type id, char const * gltfFileName, char const * animationName)
{
	auto data = asset_cooker_load_animation_glb(gltfFileName, animationName);

	global_animationHeaders[id].dataOffset 	= global_dataPosition;

	global_animationHeaders[id].channelCount 					= data.animation.channelCount;
	global_animationHeaders[id].duration 						= data.animation.duration;
	global_animationHeaders[id].totalTranslationKeyframeCount = data.totalTranslationKeyframeCount;
	global_animationHeaders[id].totalRotationKeyframeCount 	= data.totalRotationKeyframeCount;

	s64 translationInfoSize = data.animation.channelCount * sizeof(AnimationChannelInfo);
	s64 rotationInfoSize = data.animation.channelCount * sizeof(AnimationChannelInfo);

	s64 translationTimesSize = data.totalTranslationKeyframeCount * sizeof(f32);
	s64 rotationTimesSize = data.totalRotationKeyframeCount * sizeof(f32);

	s64 translationValuesSize = data.totalTranslationKeyframeCount * sizeof(v3);
	s64 rotationValuesSize = data.totalRotationKeyframeCount * sizeof(quaternion);

	u64 & position = global_dataPosition;
	
	write(position, translationInfoSize, data.animation.translationChannelInfos);
	position += translationInfoSize;

	write(position, rotationInfoSize, data.animation.rotationChannelInfos);
	position += rotationInfoSize;

	write(position, translationTimesSize, data.animation.translationTimes);
	position += translationTimesSize;

	write(position, rotationTimesSize, data.animation.rotationTimes);
	position += rotationTimesSize;

	write(position, translationValuesSize, data.animation.translations);
	position += translationValuesSize;

	write(position, rotationValuesSize, data.animation.rotations);
	position += rotationValuesSize;

	std::cout << "COOK ANIMATION: " << gltfFileName << ", " << animationName << "\n";

}

EXPORT void cook_skeleton(asset_underlying_type id, char const * gltfFileName, char const * gltfNodeName)
{
	AnimatedSkeleton data = asset_cooker_load_skeleton_glb(gltfFileName, gltfNodeName);

	global_skeletonHeaders[id].dataOffset = global_dataPosition;
	global_skeletonHeaders[id].boneCount 	= data.bonesCount;

	u64 memorySize = sizeof(AnimatedBone) * data.bonesCount;

	AnimatedBone * dataToWrite = data.bones;

	write(global_dataPosition, memorySize, dataToWrite);

	std::cout << "COOK SKELETON: " << gltfFileName << ", " << gltfNodeName << ": " << memorySize << " bytes at " << global_dataPosition << "\n";
	global_dataPosition += memorySize;

	free(data.bones);
}

EXPORT void cook_sound(asset_underlying_type id, char const * filename)
{
	AudioFile<f32> file;
	file.load(filename);
	s32 sampleCount = file.getNumSamplesPerChannel();

	global_soundHeaders[id].dataOffset 	= global_dataPosition;
	global_soundHeaders[id].sampleCount 	= sampleCount;

	u64 memorySizePerChannel = sampleCount * sizeof(f32);

	write(global_dataPosition, memorySizePerChannel, file.samples[0].data());
	global_dataPosition += memorySizePerChannel;

	write(global_dataPosition, memorySizePerChannel, file.samples[1].data());
	global_dataPosition += memorySizePerChannel;

	std::cout << "COOK AUDIO: " << filename << "\n";

}

EXPORT void cook_font(asset_underlying_type id, char const * filename)
{
	FontLoadResult loadResult = asset_cooker_load_font(filename);

	TextureAssetData & data = loadResult.atlasTextureAsset;

	global_fontHeaders[id].dataOffset 	= global_dataPosition;
	global_fontHeaders[id].width 			= data.width;
	global_fontHeaders[id].height 		= data.height;
	global_fontHeaders[id].channels 		= data.channels;
	global_fontHeaders[id].format 		= data.format;
	global_fontHeaders[id].addressMode 	= data.addressMode;

	assert(data.channels == 4);

	u64 textureMemorySize 		= data.width * data.height * data.channels;
	u64 characterInfoMemorySize = sizeof(FontCharacterInfo) * Font::characterCount;

	u64 filePosition 			= global_dataPosition;

	write(filePosition, textureMemorySize, data.pixelMemory);					
	filePosition += textureMemorySize;
	
	write(filePosition, characterInfoMemorySize, loadResult.font.characters); 	
	filePosition += characterInfoMemorySize;

	global_dataPosition = filePosition;

	std::cout << "COOK FONT: " << filename << ", " << data.width << ", " << data.height << ", " << data.channels << "\n";

	free(data.pixelMemory);
}
