/*
Leo Tamminen

Enumeration definitions that are used both in game and asset cooking pipelines

NOTICE(LEO): This file is also parsed manually by us, in asset cooker, so avoid comments,
especially ones that contain things like "MeshAssetId _ NNNNNN". This even broke it :)
*/

#include "fs_standard_types.h"

enum MeshAssetId : s32
{
	MeshAssetId_character,
	MeshAssetId_raccoon,
	MeshAssetId_robot,
	MeshAssetId_skysphere,
	MeshAssetId_totem,
	MeshAssetId_small_pot,
	MeshAssetId_big_pot,
	MeshAssetId_seed,
	MeshAssetId_water_drop,
	MeshAssetId_train,

	MeshAssetId_monument_arcs,
	MeshAssetId_monument_base,
	MeshAssetId_monument_top_1,
	MeshAssetId_monument_top_2,

	MeshAssetId_box,
	MeshAssetId_box_cover,

	MeshAssetId_cloud,
	MeshAssetId_rain,

	MeshAssetId_cube,

	MeshAssetIdCount
};

enum TextureAssetId : s32
{
	TextureAssetId_ground_albedo,
	TextureAssetId_tiles_albedo,
	TextureAssetId_red_tiles_albedo,
	TextureAssetId_seed_albedo,
	TextureAssetId_bark_albedo,
	TextureAssetId_raccoon_albedo,
	TextureAssetId_robot_albedo,
	TextureAssetId_tiles_2_albedo,

	TextureAssetId_leaves_mask,

	TextureAssetId_ground_normal,
	TextureAssetId_tiles_normal,
	TextureAssetId_bark_normal,
	TextureAssetId_robot_normal,
	TextureAssetId_tiles_2_normal,

	TextureAssetId_white,
	TextureAssetId_black,
	TextureAssetId_flat_normal,
	TextureAssetId_water_blue,

	TextureAssetId_heightmap,

	TextureAssetId_menu_background,

	TextureAssetIdCount
};

enum MaterialAssetId : s32
{
	MaterialAssetId_ground,
	MaterialAssetId_environment,
	MaterialAssetId_sky,
	MaterialAssetId_character,
	MaterialAssetId_seed,
	MaterialAssetId_tree,
	MaterialAssetId_water,
	MaterialAssetId_raccoon,
	MaterialAssetId_robot,
	MaterialAssetId_sea,
	MaterialAssetId_leaves,
	MaterialAssetId_box,
	MaterialAssetId_clouds,
	MaterialAssetId_building_block,

	MaterialAssetId_menu_background,

	MaterialAssetIdCount	
};

enum AnimationAssetId : s32
{
	AnimationAssetId_raccoon_empty,

	AnimationAssetId_character_idle,
	AnimationAssetId_character_crouch,
	AnimationAssetId_character_walk,
	AnimationAssetId_character_run,
	AnimationAssetId_character_jump,
	AnimationAssetId_character_fall,

	AnimationAssetIdCount
};

enum SkeletonAssetId : s32
{
	SkeletonAssetId_character,
	
	SkeletonAssetIdCount
};

enum SoundAssetId : s32
{
	SoundAssetId_background,
	SoundAssetId_step_1,
	SoundAssetId_step_2,
	SoundAssetId_birds,

	SoundAssetId_electric_1,
	SoundAssetId_electric_2,

	SoundAssetIdCount
};

enum FontAssetId : s32
{
	FontAssetId_game,
	FontAssetIdCount,
};