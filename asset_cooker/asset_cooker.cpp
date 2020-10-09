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

#include <windows.h>

#include "fs_essentials.hpp"
#include "game_assets.h"
#include "platform_assets.cpp"

#include "Transform3D.cpp"

#include "animations.hpp"

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

constexpr char const * assetFileName = "../assets.fsa";

static std::ofstream 		global_outFile;
static AssetFileHeader * 	global_header;
static u64 					global_dataPosition;

static void write(u64 position, u64 size, void * memory)
{
	global_outFile.seekp(position);
	global_outFile.write((char*)memory, size);
}

static void cook_texture(TextureAssetId id, char const * filename, TextureFormat format = TextureFormat_u8_srgb)
{
	TextureAssetData data = asset_cooker_load_texture(filename);

	global_header->textures[id].dataOffset = global_dataPosition;
	global_header->textures[id].width 		= data.width;
	global_header->textures[id].height 		= data.height;
	global_header->textures[id].channels 	= data.channels;

	global_header->textures[id].format 		= format;
	global_header->textures[id].addressMode = TextureAddressMode_repeat;

	assert(data.channels == 4);

	u64 memorySize 			= data.width * data.height * data.channels;
	u64 filePosition 		= global_dataPosition;
	global_dataPosition 	+= memorySize;

	write(filePosition, memorySize, data.pixelMemory);

	std::cout << "COOK TEXTURE: " << filename << ", " << memorySize << " bytes at " << filePosition << "\n";

	free(data.pixelMemory);
}

static void cook_mesh(MeshAssetId id, char const * filename, char const * gltfNodeName)
{
	MeshAssetData data = asset_cooker_load_mesh_glb(filename, gltfNodeName);

	global_header->meshes[id].dataOffset 	= global_dataPosition;
	global_header->meshes[id].vertexCount 	= data.vertexCount;
	global_header->meshes[id].indexCount 	= data.indexCount;
	
	bool32 hasSkinning 						= data.skinning != nullptr;
	global_header->meshes[id].hasSkinning 	= hasSkinning;

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

static void cook_animation(AnimationAssetId id, char const * gltfFileName, char const * animationName)
{
	auto data = asset_cooker_load_animation_glb(gltfFileName, animationName);

	global_header->animations[id].dataOffset 	= global_dataPosition;

	global_header->animations[id].channelCount 					= data.animation.channelCount;
	global_header->animations[id].duration 						= data.animation.duration;
	global_header->animations[id].totalTranslationKeyframeCount = data.totalTranslationKeyframeCount;
	global_header->animations[id].totalRotationKeyframeCount 	= data.totalRotationKeyframeCount;

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

static void cook_skeleton(SkeletonAssetId id, char const * gltfFileName, char const * gltfNodeName)
{
	AnimatedSkeleton data = asset_cooker_load_skeleton_glb(gltfFileName, gltfNodeName);

	global_header->skeletons[id].dataOffset = global_dataPosition;
	global_header->skeletons[id].boneCount 	= data.bonesCount;

	u64 memorySize = sizeof(AnimatedBone) * data.bonesCount;

	AnimatedBone * dataToWrite = data.bones;

	write(global_dataPosition, memorySize, dataToWrite);

	std::cout << "COOK SKELETON: " << gltfFileName << ", " << gltfNodeName << ": " << memorySize << " bytes at " << global_dataPosition << "\n";
	global_dataPosition += memorySize;

	free(data.bones);
}

static void cook_audio(AudioAssetId id, char const * filename)
{
	AudioFile<f32> file;
	file.load(filename);
	s32 sampleCount = file.getNumSamplesPerChannel();

	global_header->audios[id].dataOffset 	= global_dataPosition;
	global_header->audios[id].sampleCount 	= sampleCount;

	u64 memorySizePerChannel = sampleCount * sizeof(f32);

	write(global_dataPosition, memorySizePerChannel, file.samples[0].data());
	global_dataPosition += memorySizePerChannel;

	write(global_dataPosition, memorySizePerChannel, file.samples[1].data());
	global_dataPosition += memorySizePerChannel;

	std::cout << "COOK AUDIO: " << filename << "\n";

}

static void cook_font(FontAssetId id, char const * filename)
{
	FontLoadResult loadResult = asset_cooker_load_font(filename);

	TextureAssetData & data = loadResult.atlasTextureAsset;

	global_header->fonts[id].dataOffset 	= global_dataPosition;
	global_header->fonts[id].width 			= data.width;
	global_header->fonts[id].height 		= data.height;
	global_header->fonts[id].channels 		= data.channels;
	global_header->fonts[id].format 		= data.format;
	global_header->fonts[id].addressMode 	= data.addressMode;

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

int main()
{
	using std::cout;
	AssetFileHeader header = {};

	cout << "This is asset cooker\n";
	global_outFile = std::ofstream(assetFileName, std::ios::out | std::ios::binary);

	header = {};
	header.magic 	= AssetFileHeader::magicValue;
	header.version 	= AssetFileHeader::currentVersion;

	global_header 		= &header;
	global_dataPosition = sizeof(AssetFileHeader);

	// -------------------------------------------------------------

	_chdir("textures/");

	cook_texture(TextureAssetId_ground_albedo, 		"ground.png");
	cook_texture(TextureAssetId_ground_albedo, 		"ground.png");
	cook_texture(TextureAssetId_tiles_albedo, 		"tiles.png");
	cook_texture(TextureAssetId_red_tiles_albedo, 	"tiles_red.png");
	cook_texture(TextureAssetId_seed_albedo, 		"Acorn_albedo.png");
	cook_texture(TextureAssetId_bark_albedo, 		"bark.png");
	cook_texture(TextureAssetId_raccoon_albedo, 	"RaccoonAlbedo.png");
	cook_texture(TextureAssetId_robot_albedo, 		"Robot_53_albedo_4k.png");
	
	cook_texture(TextureAssetId_leaves_mask, 		"leaf_mask.png");

	cook_texture(TextureAssetId_ground_normal, 		"ground_normal.png", TextureFormat_u8_linear);
	cook_texture(TextureAssetId_tiles_normal, 		"tiles_normal.png", TextureFormat_u8_linear);
	cook_texture(TextureAssetId_bark_normal, 		"bark_normal.png", TextureFormat_u8_linear);
	cook_texture(TextureAssetId_robot_normal, 		"Robot_53_normal_4k.png", TextureFormat_u8_linear);

	cook_texture(TextureAssetId_heightmap, 			"heightmap_island.png");

	cook_texture(TextureAssetId_menu_background, 	"keyart.png");

	cout << "Textures cooked!\n";

	// -------------------------------------------------------------

	_chdir("../models/");

	cook_mesh(MeshAssetId_raccoon, 		"raccoon.glb", "raccoon");
	cook_mesh(MeshAssetId_robot, 		"Robot53.glb", "model_rigged");
	cook_mesh(MeshAssetId_character, 	"cube_head_v4.glb", "cube_head");
	cook_mesh(MeshAssetId_skysphere, 	"skysphere.glb", "skysphere");
	cook_mesh(MeshAssetId_totem, 		"totem.glb", "totem");
	cook_mesh(MeshAssetId_small_pot, 	"scenery.glb", "small_pot");
	cook_mesh(MeshAssetId_big_pot, 		"scenery.glb", "big_pot");
	cook_mesh(MeshAssetId_seed, 			"scenery.glb", "acorn");
	cook_mesh(MeshAssetId_water_drop, 	"scenery.glb", "water_drop");
	cook_mesh(MeshAssetId_train, 		"train.glb", "train");

	cook_mesh(MeshAssetId_monument_arcs, 	"monuments.glb", "monument_arches");
	cook_mesh(MeshAssetId_monument_base,		"monuments.glb", "monument_base");
	cook_mesh(MeshAssetId_monument_top_1,	"monuments.glb", "monument_ornament_01");
	cook_mesh(MeshAssetId_monument_top_2, 	"monuments.glb", "monument_ornament_02");

	cook_mesh(MeshAssetId_box, 		"box.glb", "box");
	cook_mesh(MeshAssetId_box_cover, "box.glb", "cover");

	cout << "Meshes cooked!\n";

	cook_skeleton(SkeletonAssetId_character, "cube_head_v4.glb", "cube_head");

	cout << "Skeletons cooked!\n";

	cook_animation(AnimationAssetId_character_idle,		 "cube_head_v3.glb", "Idle");
	cook_animation(AnimationAssetId_character_walk,		 "cube_head_v3.glb", "Walk");
	cook_animation(AnimationAssetId_character_run,		 "cube_head_v3.glb", "Run");
	cook_animation(AnimationAssetId_character_jump,		 "cube_head_v3.glb", "JumpUp");
	cook_animation(AnimationAssetId_character_fall,		 "cube_head_v3.glb", "JumpDown");
	cook_animation(AnimationAssetId_character_crouch, 	 "cube_head_v3.glb", "Crouch");

	cout << "Animations cooked!\n";

	// -------------------------------------------------------------

	_chdir("../sounds/");

	cook_audio(AudioAssetId_background, "Wind-Mark_DiAngelo-1940285615.wav");
	cook_audio(AudioAssetId_step_1, "step_9.wav");
	cook_audio(AudioAssetId_step_2, "step_10.wav");
	cook_audio(AudioAssetId_birds, "Falcon-Mark_Mattingly-169493032.wav");

	// -------------------------------------------------------------

	_chdir("../fonts");

	// Todo(Leo): There is some fuckery going around here >:(
	cook_font(FontAssetId_game, "SourceCodePro-Regular.ttf");
	// cook_font(FontAssetId_menu, "SourceCodePro-Regular.ttf");
	// cook_font(FontAssetId_menu, "TheStrangerBrush.ttf");

	// -------------------------------------------------------------

	write(0, sizeof(AssetFileHeader), &header);
	cout << "global_header written to file \n";


	global_outFile.close();

	cout << "All done, all good.\n";
}