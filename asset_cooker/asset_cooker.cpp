/*
Leo Tamminen

Asset cooking (sometimes called baking) utility.
*/

#include <direct.h>

#include "game_assets.h"
#include "platform_assets.cpp"

// Todo(Leo):
struct AssetCounts
{
	s32 texture;
	s32 mesh;
	s32 animation;
	s32 skeleton;
	s32 sound;
	s32 font;
}; 

extern "C" void cook_initialize(char const * filename, AssetCounts & assetCounts);
extern "C" void cook_complete();

extern "C" void cook_texture(TextureAssetId id, char const * filename, TextureFormat format);
extern "C" void cook_mesh(MeshAssetId id, char const * filename, char const * gltfNodeName);
extern "C" void cook_animation(AnimationAssetId id, char const * gltfFileName, char const * animationName);
extern "C" void cook_skeleton(SkeletonAssetId id, char const * gltfFileName, char const * gltfNodeName);
extern "C" void cook_sound(SoundAssetId id, char const * filename);
extern "C" void cook_font(FontAssetId id, char const * filename);

int main()
{
	AssetCounts counts = {};
	counts.texture 		= TextureAssetIdCount;
	counts.mesh 		= MeshAssetIdCount;
	counts.animation 	= AnimationAssetIdCount;
	counts.skeleton 	= SkeletonAssetIdCount;
	counts.sound 		= SoundAssetIdCount;
	counts.font 		= FontAssetIdCount;


	cook_initialize("../development/assets.fsa", counts);

	// -------------------------------------------------------------

	_chdir("textures/");

	// Todo(Leo): do this maybe
	// enum TextureAssetType
	// {
	// 	TextureAssetType_albedo,
	// 	TextureAssetType_normal,
	// };

	struct TextureAssetInfo
	{
		TextureAssetId 	id;
		char const * 	filename;
		TextureFormat 	format;
	};

	TextureAssetInfo textures [] =
	{
		{ TextureAssetId_ground_albedo, 	"ground.png" },
		{ TextureAssetId_ground_albedo, 	"ground.png" },
		{ TextureAssetId_tiles_albedo, 		"tiles.png" },
		{ TextureAssetId_red_tiles_albedo, 	"tiles_red.png" },
		{ TextureAssetId_seed_albedo, 		"Acorn_albedo.png" },
		{ TextureAssetId_bark_albedo, 		"bark.png" },
		{ TextureAssetId_raccoon_albedo, 	"RaccoonAlbedo.png" },
		{ TextureAssetId_robot_albedo, 		"Robot_53_albedo_4k.png" },
		
		{ TextureAssetId_leaves_mask, 		"leaf_mask.png" },

		{ TextureAssetId_ground_normal, 	"ground_normal.png", 		TextureFormat_u8_linear },
		{ TextureAssetId_tiles_normal, 		"tiles_normal.png", 		TextureFormat_u8_linear },
		{ TextureAssetId_bark_normal, 		"bark_normal.png", 			TextureFormat_u8_linear },
		{ TextureAssetId_robot_normal, 		"Robot_53_normal_4k.png", 	TextureFormat_u8_linear },

		{ TextureAssetId_heightmap, 		"heightmap_island.png" },

		{ TextureAssetId_menu_background, 	"keyart.png" },

		{ TextureAssetId_tiles_2_albedo,  	"DungeonTiles_albedo.png" },
		{ TextureAssetId_tiles_2_normal,  	"DungeonTiles_normal.png",	TextureFormat_u8_linear },
		{ TextureAssetId_tiles_2_ao,  		"DungeonTiles_AO.png" },
	};

	for (auto & texture : textures)
	{
		cook_texture(texture.id, texture.filename, texture.format);
	}

	// -------------------------------------------------------------

	_chdir("../models/");

	struct MeshAssetInfo
	{
		MeshAssetId 	id;
		char const * 	filename;
		char const * 	name;
	};

	MeshAssetInfo meshes [] = 
	{
		{ MeshAssetId_raccoon, 		"raccoon.glb", 		"raccoon" },
		{ MeshAssetId_robot, 		"Robot53.glb", 		"model_rigged" },
		{ MeshAssetId_character, 	"cube_head_v4.glb", "cube_head" },
		{ MeshAssetId_skysphere, 	"skysphere.glb", 	"skysphere" },
		{ MeshAssetId_totem, 		"totem.glb", 		"totem" },
		{ MeshAssetId_small_pot, 	"scenery.glb", 		"small_pot" },
		{ MeshAssetId_big_pot, 		"scenery.glb", 		"big_pot" },
		{ MeshAssetId_seed, 		"scenery.glb", 		"acorn" },
		{ MeshAssetId_water_drop, 	"scenery.glb", 		"water_drop" },
		{ MeshAssetId_train, 		"train.glb", 		"train" },

		{ MeshAssetId_monument_arcs, 	"monuments.glb", "monument_arches" },
		{ MeshAssetId_monument_base,	"monuments.glb", "monument_base" },
		{ MeshAssetId_monument_top_1,	"monuments.glb", "monument_ornament_01" },
		{ MeshAssetId_monument_top_2, 	"monuments.glb", "monument_ornament_02" },

		{ MeshAssetId_box, 			"box.glb", 		"box" },
		{ MeshAssetId_box_cover, 	"box.glb",		"cover" },
		{ MeshAssetId_cloud, 		"cloud.glb", 	"cloud" },
		{ MeshAssetId_rain,			"cloud.glb", 	"rain" },

		{ MeshAssetId_cube,			"cube_2m.glb", 	"Cube" },
	};

	for (auto & mesh : meshes)
	{
		cook_mesh(mesh.id, mesh.filename, mesh.name);
	}

	// -------------------------------------------------------------

	cook_skeleton(SkeletonAssetId_character, "cube_head_v4.glb", "cube_head");

	// -------------------------------------------------------------

	struct AnimationAssetInfo
	{
		AnimationAssetId 	id;
		char const * 		filename;
		char const * 		name;
	};

	AnimationAssetInfo animations [] =
	{
		{ AnimationAssetId_character_idle,		 "cube_head_v3.glb", "Idle" },
		{ AnimationAssetId_character_walk,		 "cube_head_v3.glb", "Walk" },
		// Todo(Leo): lol these are super bad...
		{ AnimationAssetId_character_run,		 "cube_head_v3_backup_before_adding_animations.glb", "Run" },
		{ AnimationAssetId_character_jump,		 "cube_head_v3.glb", "JumpUp" },
		{ AnimationAssetId_character_fall,		 "cube_head_v3.glb", "JumpDown" },
		{ AnimationAssetId_character_crouch, 	 "cube_head_v3.glb", "Crouch" },
		{ AnimationAssetId_character_climb, 	 "cube_head_v3.glb", "Climb" },
	};

	for (auto & animation : animations)
	{
		cook_animation(animation.id, animation.filename, animation.name);
	}

	// -------------------------------------------------------------

	_chdir("../sounds/");

	struct SoundAssetInfo
	{
		SoundAssetId id;
		char const * filename;
	};

	SoundAssetInfo sounds [] =
	{
		{ SoundAssetId_background, 	"Wind-Mark_DiAngelo-1940285615.wav" },
		{ SoundAssetId_step_1, 		"step_9.wav" },
		{ SoundAssetId_step_2, 		"step_10.wav" },
		{ SoundAssetId_birds, 		"Falcon-Mark_Mattingly-169493032.wav" },
		// { SoundAssetId_electric_1, 	"arc1_freesoundeffects_com_NO_LICENSE.wav" },
		// { SoundAssetId_electric_2, 	"earcing_freesoundeffects_com_NO_LICENSE.wav" },
	};

	for (auto & sound : sounds)
	{
		cook_sound(sound.id, sound.filename);
	}


	// -------------------------------------------------------------

	_chdir("../fonts");

	// Todo(Leo): There is some fuckery going around here >:(
	cook_font(FontAssetId_game, "SourceCodePro-Regular.ttf");
	// cook_font(FontAssetId_menu, "SourceCodePro-Regular.ttf");
	// cook_font(FontAssetId_menu, "TheStrangerBrush.ttf");

	// -------------------------------------------------------------

	cook_complete();
}
