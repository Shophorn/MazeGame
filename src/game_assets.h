/*
Leo Tamminen

Enumeration definitions that are used both in game and asset cooking pipelines
*/


// Todo(Leo): maybe do this :)
// enum AssetId : s32
// {
// 	AssetId_cloud_mesh,

// 	AssetId_character_mesh,
// 	AssetId_raccoon_mesh,
// 	AssetId_robot_mesh,
// 	AssetId_skysphere_mesh,
// 	AssetId_totem_mesh,
// 	AssetId_small_pot_mesh,
// 	AssetId_big_pot_mesh,
// 	AssetId_seed_mesh,
// 	AssetId_water_drop_mesh,
// 	AssetId_train_mesh,

// 	AssetId_monument_arcs_mesh,
// 	AssetId_monument_base_mesh,
// 	AssetId_monument_top_1_mesh,
// 	AssetId_monument_top_2_mesh,

// 	AssetId_box_mesh,
// 	AssetId_box_cover_mesh,	

// 	AssetIdCount
// };

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

	TextureAssetId_leaves_mask,

	TextureAssetId_ground_normal,
	TextureAssetId_tiles_normal,
	TextureAssetId_bark_normal,
	TextureAssetId_robot_normal,

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

// enum SkeletonType : s32
// {
// 	SKELETON_TYPE_CHARACTER,
// 	SKELETON_TYPE_RACCOON,

// 	SKELETON_TYPE_COUNT
// };

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

enum AudioAssetId : s32
{
	AudioAssetId_background,
	AudioAssetId_step_1,
	AudioAssetId_step_2,
	AudioAssetId_birds,

	AudioAssetId_electric_1,
	AudioAssetId_electric_2,

	AudioAssetIdCount
};

enum FontAssetId : s32
{
	FontAssetId_game,
	// FontAssetId_menu,

	FontAssetIdCount,

	// FontAssetId_menu = FontAssetId_game
};