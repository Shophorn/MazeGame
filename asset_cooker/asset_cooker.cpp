#include <iostream>
#include <fstream>
#include <cassert>
#include <direct.h>

#include "fs_essentials.hpp"
#include "game_asset_id.cpp"
#include "fs_asset_file_format.cpp"

#include "Assets.cpp"

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

static void cook_texture(TextureAssetId id, char const * filename)
{
	TextureAssetData data = asset_cooker_load_texture(filename);

	global_header->textures[id].dataOffset = global_dataPosition;
	global_header->textures[id].width 		= data.width;
	global_header->textures[id].height 	= data.height;
	global_header->textures[id].channels 	= data.channels;

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

	std::cout << "COOK MESH: " << filename << ", " << (filePosition - global_dataPosition) << " bytes at " << global_dataPosition << "\n";

	global_dataPosition = filePosition;

	free(data.vertices);
	if (data.skinning)
		free(data.skinning);
	free(data.indices);
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
	header.magic 			= header.magicValue;
	header.version 		= 0;

	global_header = &header;
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

	cook_texture(TEXTURE_ASSET_GROUND_NORMAL, 		"ground_normal.png");
	cook_texture(TEXTURE_ASSET_TILES_NORMAL, 		"tiles_normal.png");
	cook_texture(TEXTURE_ASSET_BARK_NORMAL, 		"bark_normal.png");
	cook_texture(TEXTURE_ASSET_ROBOT_NORMAL, 		"Robot_53_normal_4k.png");

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
	
	// -------------------------------------------------------------

	write(0, sizeof(AssetFileHeader), &header);
	cout << "global_header written to file \n";


	global_outFile.close();

	header = {};

	auto testReadFile = std::ifstream(assetFileName, std::ios::in | std::ios::binary);
	testReadFile.read((char*)&header, sizeof(header));


	cout << "All done, all good.\n";
}