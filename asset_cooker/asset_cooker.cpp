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
#include "game_asset_id.cpp"


#include "Assets.cpp"
#include "Transform3D.cpp"
// #include "Animator.cpp"

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

constexpr char const * assetFileName = "assets.fsa";

static std::ofstream 		global_outFile;
static AssetFileHeader * 	global_header;
static u64 					global_dataPosition;

static void write(u64 position, u64 size, void * memory)
{
	global_outFile.seekp(position);
	global_outFile.write((char*)memory, size);
}

static void cook_texture(TextureAssetId id, char const * filename, TextureFormat format = TEXTURE_FORMAT_U8_SRGB)
{
	TextureAssetData data = asset_cooker_load_texture(filename);

	global_header->textures[id].dataOffset = global_dataPosition;
	global_header->textures[id].width 		= data.width;
	global_header->textures[id].height 		= data.height;
	global_header->textures[id].channels 	= data.channels;

	global_header->textures[id].format 		= format;
	global_header->textures[id].addressMode = TEXTURE_ADDRESS_MODE_REPEAT;

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

	quaternion q = dataToWrite[9].boneSpaceDefaultTransform.rotation;
	std::cout << q.x << "\n";
	std::cout << q.y << "\n";
	std::cout << q.z << "\n";
	std::cout << q.w << "\n";




	std::cout << "COOK SKELETON: " << gltfFileName << ", " << gltfNodeName << ": " << memorySize << " bytes at " << global_dataPosition << "\n";
	global_dataPosition += memorySize;

	free(data.bones);
}

int main()
{
	using std::cout;
	AssetFileHeader header = {};

	auto inheader = std::ifstream(assetFileName, std::ios::in | std::ios::binary);
	inheader.read((char*)&header, sizeof(header));

	inheader.close();

	cout << "This is asset cooker\n";
	global_outFile = std::ofstream(assetFileName, std::ios::out | std::ios::binary);

	header = {};
	header.magic 	= AssetFileHeader::magicValue;
	header.version 	= AssetFileHeader::currentVersion;

	global_header 		= &header;
	global_dataPosition = sizeof(AssetFileHeader);

	// -------------------------------------------------------------

	_chdir("../assets/textures/");

	cook_texture(TEXTURE_ASSET_GROUND_ALBEDO, 		"ground.png");
	cook_texture(TEXTURE_ASSET_GROUND_ALBEDO, 		"ground.png");
	cook_texture(TEXTURE_ASSET_TILES_ALBEDO, 		"tiles.png");
	cook_texture(TEXTURE_ASSET_RED_TILES_ALBEDO, 	"tiles_red.png");
	cook_texture(TEXTURE_ASSET_SEED_ALBEDO, 		"Acorn_albedo.png");
	cook_texture(TEXTURE_ASSET_BARK_ALBEDO, 		"bark.png");
	cook_texture(TEXTURE_ASSET_RACCOON_ALBEDO, 		"RaccoonAlbedo.png");
	cook_texture(TEXTURE_ASSET_ROBOT_ALBEDO, 		"Robot_53_albedo_4k.png");
	
	cook_texture(TEXTURE_ASSET_LEAVES_MASK, 		"leaf_mask.png");

	cook_texture(TEXTURE_ASSET_GROUND_NORMAL, 		"ground_normal.png", TEXTURE_FORMAT_U8_LINEAR);
	cook_texture(TEXTURE_ASSET_TILES_NORMAL, 		"tiles_normal.png", TEXTURE_FORMAT_U8_LINEAR);
	cook_texture(TEXTURE_ASSET_BARK_NORMAL, 		"bark_normal.png", TEXTURE_FORMAT_U8_LINEAR);
	cook_texture(TEXTURE_ASSET_ROBOT_NORMAL, 		"Robot_53_normal_4k.png", TEXTURE_FORMAT_U8_LINEAR);

	cook_texture(TEXTURE_ASSET_HEIGHTMAP, 			"heightmap_island.png");

	cout << "Textures cooked!\n";

	// -------------------------------------------------------------

	_chdir("../models/");

	cook_mesh(MESH_ASSET_RACCOON, 		"raccoon.glb", "raccoon");
	cook_mesh(MESH_ASSET_ROBOT, 		"Robot53.glb", "model_rigged");
	cook_mesh(MESH_ASSET_CHARACTER, 	"cube_head_v4.glb", "cube_head");
	cook_mesh(MESH_ASSET_SKYSPHERE, 	"skysphere.glb", "skysphere");
	cook_mesh(MESH_ASSET_TOTEM, 		"totem.glb", "totem");
	cook_mesh(MESH_ASSET_SMALL_POT, 	"scenery.glb", "small_pot");
	cook_mesh(MESH_ASSET_BIG_POT, 		"scenery.glb", "big_pot");
	cook_mesh(MESH_ASSET_SEED, 			"scenery.glb", "acorn");
	cook_mesh(MESH_ASSET_WATER_DROP, 	"scenery.glb", "water_drop");
	cook_mesh(MESH_ASSET_TRAIN, 		"train.glb", "train");

	cook_mesh(MESH_ASSET_MONUMENT_ARCS, 	"monuments.glb", "monument_arches");
	cook_mesh(MESH_ASSET_MONUMENT_BASE,		"monuments.glb", "monument_base");
	cook_mesh(MESH_ASSET_MONUMENT_TOP_1,	"monuments.glb", "monument_ornament_01");
	cook_mesh(MESH_ASSET_MONUMENT_TOP_2, 	"monuments.glb", "monument_ornament_02");

	cook_mesh(MESH_ASSET_BOX, 		"box.glb", "box");
	cook_mesh(MESH_ASSET_BOX_COVER, "box.glb", "cover");

	cout << "Meshes cooked!\n";

	cook_skeleton(SAID_CHARACTER, "cube_head_v4.glb", "cube_head");

	cout << "Skeletons cooked!\n";

	cook_animation(AAID_CHARACTER_IDLE,		 "cube_head_v3.glb", "Idle");
	cook_animation(AAID_CHARACTER_WALK,		 "cube_head_v3.glb", "Walk");
	cook_animation(AAID_CHARACTER_RUN,		 "cube_head_v3.glb", "Run");
	cook_animation(AAID_CHARACTER_JUMP,		 "cube_head_v3.glb", "JumpUp");
	cook_animation(AAID_CHARACTER_FALL,		 "cube_head_v3.glb", "JumpDown");
	cook_animation(AAID_CHARACTER_CROUCH, 	 "cube_head_v3.glb", "Crouch");

	cout << "Animations cooked!\n";

	// -------------------------------------------------------------

	write(0, sizeof(AssetFileHeader), &header);
	cout << "global_header written to file \n";


	global_outFile.close();



	AssetFileHeader readedHeader = {};

	HANDLE testReadFile = CreateFile(assetFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	ReadFile(testReadFile, &readedHeader, sizeof(AssetFileHeader), nullptr, nullptr);

	AnimatedBone readedBones[30];
	SetFilePointer(testReadFile, readedHeader.skeletons[SAID_CHARACTER].dataOffset, 0, FILE_BEGIN);
	ReadFile(testReadFile, readedBones, sizeof(AnimatedBone) * readedHeader.skeletons[SAID_CHARACTER].boneCount, nullptr, nullptr);
	// auto testReadFile = std::ifstream(assetFileName, std::ios::in | std::ios::binary);

	// testReadFile.seekg(readedHeader.skeletons[SAID_CHARACTER].dataOffset);
	// testReadFile.read((char*)readedBones, sizeof(AnimatedBone) * readedHeader.skeletons[SAID_CHARACTER].boneCount);

	CloseHandle(testReadFile);

	cout << "All done, all good.\n";
}