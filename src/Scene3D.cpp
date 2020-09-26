/*
Leo Tamminen
shophorn @ internet

Scene description for 3d development scene

Todo(Leo):
	crystal trees 
*/
#include "CharacterMotor.cpp"
#include "PlayerController3rdPerson.cpp"
#include "FollowerController.cpp"

#include "scene3d_monuments.cpp"
#include "scene3d_trees.cpp"

#include "metaballs.cpp"
#include "experimental.cpp"

struct GetWaterFunc
{
	Waters & 	waters;
	s32 		carriedItemIndex;
	bool32 		waterIsCarried;

	f32 operator()(v3 position, f32 requestedAmount)
	{
		s32 closestIndex;
		f32 closestDistance = highest_f32;

		for (s32 i = 0; i < waters.count; ++i)
		{
			if (carriedItemIndex == i && waterIsCarried)
			{
				continue;
			}

			f32 distance = magnitude_v3(waters.transforms[i].position - position);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestIndex 	= i;
			}
		}

		f32 amount 				= 0;
		f32 distanceThreshold	= 2;

		if (closestDistance < distanceThreshold)
		{
			waters.levels[closestIndex] -= requestedAmount;
			if (waters.levels[closestIndex] < 0)
			{
				waters.count -= 1;
				waters.transforms[closestIndex] 	= waters.transforms[waters.count];
				waters.levels[closestIndex] 		= waters.levels[waters.count];
			}

			amount = requestedAmount;
		}

		return amount;
	};
};

#include "dynamic_mesh.cpp"
#include "Trees3.cpp"
#include "settings.cpp"

enum CameraMode : s32
{ 
	CAMERA_MODE_PLAYER, 
	CAMERA_MODE_ELEVATOR,
	CAMERA_MODE_MOUSE_AND_KEYBOARD,

	CAMERA_MODE_COUNT
};

enum CarryMode : s32
{ 	
	CARRY_NONE,
	CARRY_POT,
	CARRY_WATER,
	CARRY_RACCOON,
	CARRY_TREE_3,
};

enum TrainState : s32
{
	TRAIN_MOVE,
	TRAIN_WAIT,
};

enum NoblePersonMode : s32
{
	NOBLE_WANDER_AROUND,
	NOBLE_WAIT_FOR_TRAIN,
	NOBLE_AWAY,
	NOBLE_ARRIVING_IN_TRAIN,
};

enum RaccoonMode : s32
{
	RACCOON_IDLE,
	RACCOON_FLEE,
	RACCOON_CARRIED,
};

struct FallingObject
{
	s32 type;
	s32 index;
	f32 fallSpeed;
};

enum MenuView : s32
{
	MENU_OFF,
	MENU_MAIN,
	MENU_CONFIRM_EXIT,
	MENU_CONFIRM_TELEPORT,
	MENU_EDIT_SKY,
	MENU_EDIT_MESH_GENERATION,
	MENU_EDIT_TREE,
	MENU_SAVE_COMPLETE,
};

enum MeshAssetId : s32
{
	MESH_ASSET_CHARACTER,
	MESH_ASSET_RACCOON,
	MESH_ASSET_ROBOT,
	MESH_ASSET_SKYSPHERE,
	MESH_ASSET_TOTEM,
	MESH_ASSET_SMALL_POT,
	MESH_ASSET_BIG_POT,
	MESH_ASSET_SEED,
	MESH_ASSET_WATER_DROP,
	MESH_ASSET_TRAIN,

	MESH_ASSET_COUNT
};

enum TextureAssetId : s32
{
	TEXTURE_ASSET_GROUND_ALBEDO,
	TEXTURE_ASSET_TILES_ALBEDO,
	TEXTURE_ASSET_RED_TILES_ALBEDO,
	TEXTURE_ASSET_SEED_ALBEDO,
	TEXTURE_ASSET_BARK_ALBEDO,
	TEXTURE_ASSET_RACCOON_ALBEDO,
	TEXTURE_ASSET_ROBOT_ALBEDO,

	TEXTURE_ASSET_LEAVES_MASK,

	TEXTURE_ASSET_GROUND_NORMAL,
	TEXTURE_ASSET_TILES_NORMAL,
	TEXTURE_ASSET_BARK_NORMAL,
	TEXTURE_ASSET_ROBOT_NORMAL,

	TEXTURE_ASSET_WHITE,
	TEXTURE_ASSET_BLACK,
	TEXTURE_ASSET_FLAT_NORMAL,
	TEXTURE_ASSET_WATER_BLUE,

	TEXTURE_ASSET_COUNT
};

enum MaterialAssetId : s32
{
	MATERIAL_ASSET_GROUND,
	MATERIAL_ASSET_ENVIRONMENT,
	MATERIAL_ASSET_SKY,
	MATERIAL_ASSET_CHARACTER,
	MATERIAL_ASSET_SEED,
	MATERIAL_ASSET_TREE,
	MATERIAL_ASSET_WATER,
	MATERIAL_ASSET_RACCOON,
	MATERIAL_ASSET_ROBOT,
	MATERIAL_ASSET_SEA,
	MATERIAL_ASSET_LEAVES,

	MATERIAL_ASSET_COUNT	
};

enum AnimationAssetId : s32
{
	AAID_RACCOON_EMPTY,

	AAID_CHARACTER_IDLE,
	AAID_CHARACTER_CROUCH,
	AAID_CHARACTER_WALK,
	AAID_CHARACTER_RUN,
	AAID_CHARACTER_JUMP,
	AAID_CHARACTER_FALL,

	AAID_COUNT
};

enum SkeletonAssetId : s32
{
	SAID_CHARACTER,
	SAID_COUNT
};

struct MeshLoadInfo
{
	char const * filename;
	char const * gltfNodeName;
};

struct TextureLoadInfo
{
	TextureFormat 		format;
	TextureAddressMode 	addressMode;
	
	bool generateAssetData;
	
	char const * filename;
	
	v4 generatedTextureColour;	
};

struct MaterialLoadInfo
{
	static constexpr int max_texture_count = 5;

	GraphicsPipeline 	pipeline;
	TextureAssetId 		textures[max_texture_count];
};

struct AnimationLoadInfo
{
	char const * filename;
	char const * gltfNodeName;
};

struct AnimatedSkeletonLoadInfo
{
	char const * filename;
	char const * gltfNodeName;
};

struct GameAssets
{
	// GPU MEMORY
	MeshHandle 			meshes [MESH_ASSET_COUNT];
	MeshLoadInfo 		meshAssetLoadInfo [MESH_ASSET_COUNT];

	TextureHandle 		textures [TEXTURE_ASSET_COUNT];
	TextureLoadInfo 	textureLoadInfos [TEXTURE_ASSET_COUNT];	

	MaterialHandle 		materials [MATERIAL_ASSET_COUNT];
	MaterialLoadInfo 	materialLoadInfos [MATERIAL_ASSET_COUNT];

	// CPU MEMORY
	MemoryArena * allocator;

	Animation *			animations [AAID_COUNT];
	AnimationLoadInfo 	animationLoadInfos [AAID_COUNT];

	AnimatedSkeleton * 			skeletons [SAID_COUNT];
	AnimatedSkeletonLoadInfo 	skeletonLoadInfos [SAID_COUNT];
};

internal GameAssets init_game_assets(MemoryArena * allocator)
{
	// Note(Leo): apparently this does indeed set MeshHandles to -1, which is what we want. I do not know why, though.
	GameAssets assets 	= {};
	assets.allocator 	= allocator;

	assets.meshAssetLoadInfo[MESH_ASSET_RACCOON] 	= {"assets/models/raccoon.glb", "raccoon"};
	assets.meshAssetLoadInfo[MESH_ASSET_ROBOT] 		= {"assets/models/Robot53.glb", "model_rigged"};
	assets.meshAssetLoadInfo[MESH_ASSET_CHARACTER] 	= {"assets/models/cube_head_v4.glb", "cube_head"};
	assets.meshAssetLoadInfo[MESH_ASSET_SKYSPHERE] 	= {"assets/models/skysphere.glb", "skysphere"};
	assets.meshAssetLoadInfo[MESH_ASSET_TOTEM] 		= {"assets/models/totem.glb", "totem"};
	assets.meshAssetLoadInfo[MESH_ASSET_SMALL_POT] 	= {"assets/models/scenery.glb", "small_pot"};
	assets.meshAssetLoadInfo[MESH_ASSET_BIG_POT] 	= {"assets/models/scenery.glb", "big_pot"};
	assets.meshAssetLoadInfo[MESH_ASSET_SEED] 		= {"assets/models/scenery.glb", "acorn"};
	assets.meshAssetLoadInfo[MESH_ASSET_WATER_DROP] = {"assets/models/scenery.glb", "water_drop"};
	assets.meshAssetLoadInfo[MESH_ASSET_TRAIN] 		= {"assets/models/train.glb", "train"};

	auto load_from_file = [](char const * filename, TextureFormat format = {}, TextureAddressMode addressMode = {}) -> TextureLoadInfo
	{
		TextureLoadInfo result 		= {};
		result.generateAssetData 	= false;
		result.filename 			= filename;
		result.format 				= format;
		result.addressMode 			= addressMode;
		return result;
	};

	auto load_generated = [](v4 colour, TextureFormat format = {}, TextureAddressMode addressMode = {}) -> TextureLoadInfo
	{
		TextureLoadInfo result 			= {};
		result.generateAssetData 		= true;
		result.generatedTextureColour 	= colour;
		result.format 					= format;
		result.addressMode 				= addressMode;
		return result;
	};

	assets.textureLoadInfos[TEXTURE_ASSET_GROUND_ALBEDO] 	= load_from_file("assets/textures/ground.png");
	assets.textureLoadInfos[TEXTURE_ASSET_TILES_ALBEDO] 	= load_from_file("assets/textures/tiles.png");
	assets.textureLoadInfos[TEXTURE_ASSET_RED_TILES_ALBEDO] = load_from_file("assets/textures/tiles_red.png");
	assets.textureLoadInfos[TEXTURE_ASSET_SEED_ALBEDO] 		= load_from_file("assets/textures/Acorn_albedo.png");
	assets.textureLoadInfos[TEXTURE_ASSET_BARK_ALBEDO]		= load_from_file("assets/textures/bark.png");
	assets.textureLoadInfos[TEXTURE_ASSET_RACCOON_ALBEDO]	= load_from_file("assets/textures/RaccoonAlbedo.png");
	assets.textureLoadInfos[TEXTURE_ASSET_ROBOT_ALBEDO] 	= load_from_file("assets/textures/Robot_53_albedo_4k.png");
	
	assets.textureLoadInfos[TEXTURE_ASSET_LEAVES_MASK]			= load_from_file("assets/textures/leaf_mask.png");

	assets.textureLoadInfos[TEXTURE_ASSET_GROUND_NORMAL] 	= load_from_file("assets/textures/ground_normal.png", TEXTURE_FORMAT_U8_LINEAR);
	assets.textureLoadInfos[TEXTURE_ASSET_TILES_NORMAL] 	= load_from_file("assets/textures/tiles_normal.png", TEXTURE_FORMAT_U8_LINEAR);
	assets.textureLoadInfos[TEXTURE_ASSET_BARK_NORMAL]		= load_from_file("assets/textures/bark_normal.png", TEXTURE_FORMAT_U8_LINEAR);
	assets.textureLoadInfos[TEXTURE_ASSET_ROBOT_NORMAL] 	= load_from_file("assets/textures/Robot_53_normal_4k.png");

	assets.textureLoadInfos[TEXTURE_ASSET_WHITE] 		= load_generated(colour_white, TEXTURE_FORMAT_U8_SRGB);
	assets.textureLoadInfos[TEXTURE_ASSET_BLACK] 		= load_generated(colour_black, TEXTURE_FORMAT_U8_SRGB);
	assets.textureLoadInfos[TEXTURE_ASSET_FLAT_NORMAL] 	= load_generated(colour_bump, TEXTURE_FORMAT_U8_LINEAR);
	assets.textureLoadInfos[TEXTURE_ASSET_WATER_BLUE] 	= load_generated(colour_rgb_alpha(colour_aqua_blue.rgb, 0.4), TEXTURE_FORMAT_U8_SRGB);

	// ----------- MATERIALS -------------------

	assets.materialLoadInfos[MATERIAL_ASSET_GROUND] 		= {GRAPHICS_PIPELINE_NORMAL, TEXTURE_ASSET_GROUND_ALBEDO, TEXTURE_ASSET_GROUND_NORMAL, TEXTURE_ASSET_BLACK};
	assets.materialLoadInfos[MATERIAL_ASSET_ENVIRONMENT] 	= {GRAPHICS_PIPELINE_NORMAL, TEXTURE_ASSET_TILES_ALBEDO, TEXTURE_ASSET_TILES_NORMAL, TEXTURE_ASSET_BLACK};
	assets.materialLoadInfos[MATERIAL_ASSET_SEED] 			= {GRAPHICS_PIPELINE_NORMAL, TEXTURE_ASSET_SEED_ALBEDO, TEXTURE_ASSET_FLAT_NORMAL, TEXTURE_ASSET_BLACK};
	assets.materialLoadInfos[MATERIAL_ASSET_RACCOON] 		= {GRAPHICS_PIPELINE_NORMAL, TEXTURE_ASSET_RACCOON_ALBEDO, TEXTURE_ASSET_FLAT_NORMAL, TEXTURE_ASSET_BLACK};
	assets.materialLoadInfos[MATERIAL_ASSET_ROBOT]			= {GRAPHICS_PIPELINE_NORMAL, TEXTURE_ASSET_ROBOT_ALBEDO, TEXTURE_ASSET_ROBOT_NORMAL, TEXTURE_ASSET_BLACK};
	
	assets.materialLoadInfos[MATERIAL_ASSET_CHARACTER] 		= {GRAPHICS_PIPELINE_ANIMATED, TEXTURE_ASSET_RED_TILES_ALBEDO, TEXTURE_ASSET_TILES_NORMAL, TEXTURE_ASSET_BLACK};
	
	assets.materialLoadInfos[MATERIAL_ASSET_WATER] 			= {GRAPHICS_PIPELINE_WATER, TEXTURE_ASSET_WATER_BLUE, TEXTURE_ASSET_FLAT_NORMAL, TEXTURE_ASSET_BLACK};
	assets.materialLoadInfos[MATERIAL_ASSET_SEA] 			= {GRAPHICS_PIPELINE_WATER, TEXTURE_ASSET_WATER_BLUE, TEXTURE_ASSET_FLAT_NORMAL, TEXTURE_ASSET_BLACK};
	
	assets.materialLoadInfos[MATERIAL_ASSET_LEAVES] 		= {GRAPHICS_PIPELINE_LEAVES, TEXTURE_ASSET_LEAVES_MASK};

	assets.materialLoadInfos[MATERIAL_ASSET_SKY] 			= {GRAPHICS_PIPELINE_SKYBOX, TEXTURE_ASSET_BLACK, TEXTURE_ASSET_BLACK};
	
	assets.materialLoadInfos[MATERIAL_ASSET_TREE]			= {GRAPHICS_PIPELINE_TRIPLANAR, TEXTURE_ASSET_TILES_ALBEDO};

	// ------------- ANIMATIONS ---------------------

	assets.animationLoadInfos[AAID_CHARACTER_IDLE]		= {"assets/models/cube_head_v3.glb", "Idle"};
	assets.animationLoadInfos[AAID_CHARACTER_WALK]		= {"assets/models/cube_head_v3.glb", "Walk"};
	assets.animationLoadInfos[AAID_CHARACTER_RUN]		= {"assets/models/cube_head_v3.glb", "Run"};
	assets.animationLoadInfos[AAID_CHARACTER_JUMP]		= {"assets/models/cube_head_v3.glb", "JumpUp"};
	assets.animationLoadInfos[AAID_CHARACTER_FALL]		= {"assets/models/cube_head_v3.glb", "JumpDown"};
	assets.animationLoadInfos[AAID_CHARACTER_CROUCH] 	= {"assets/models/cube_head_v3.glb", "Crouch"};

	assets.animationLoadInfos[AAID_RACCOON_EMPTY] = {};

	// ------------- SKELETONS ----------------------------

	assets.skeletonLoadInfos[SAID_CHARACTER] = {"assets/models/cube_head_v4.glb", "cube_head"};

	return assets;
}

internal MeshHandle assets_get_mesh(GameAssets & assets, MeshAssetId id)
{
	MeshHandle handle = assets.meshes[id];
	
	if (is_valid_handle(handle) == false)
	{
		auto file 			= read_gltf_file(*global_transientMemory, assets.meshAssetLoadInfo[id].filename);
		auto assetData 		= load_mesh_glb(*global_transientMemory, file, assets.meshAssetLoadInfo[id].gltfNodeName);
		assets.meshes[id] 	= graphics_memory_push_mesh(platformGraphics, &assetData);
		handle 				= assets.meshes[id];
	}

	return handle;
}

internal TextureHandle assets_get_texture(GameAssets & assets, TextureAssetId id)
{
	// Todo(Leo): textures are copied too many times: from file to stb, from stb to TextureAssetData, from TextureAssetData to graphics.
	TextureHandle handle = assets.textures[id];

	if (is_valid_handle(handle) == false)
	{
		TextureLoadInfo & loadInfo = assets.textureLoadInfos[id];
		TextureAssetData assetData;
		if (loadInfo.generateAssetData == false)
		{
			assetData = load_texture_asset(*global_transientMemory, loadInfo.filename);
		}
		else
		{
			u32 textureColour 	= colour_rgba_u32(loadInfo.generatedTextureColour);
			assetData 			= make_texture_asset(&textureColour, 1, 1, 4);
		}

		assetData.format 		= loadInfo.format;
		assetData.addressMode 	= loadInfo.addressMode;
		assets.textures[id] 	= graphics_memory_push_texture(platformGraphics, &assetData);
		handle 					= assets.textures[id];
	}

	return handle;
}

internal MaterialHandle assets_get_material(GameAssets & assets, MaterialAssetId id)
{
	MaterialHandle handle = assets.materials[id];

	if (is_valid_handle(handle) == false)
	{
		MaterialLoadInfo & loadInfo = assets.materialLoadInfos[id];

		s32 textureCount = 3;
		if (loadInfo.pipeline == GRAPHICS_PIPELINE_SKYBOX)
		{
			textureCount = 2;
		}
		else if (loadInfo.pipeline == GRAPHICS_PIPELINE_LEAVES || loadInfo.pipeline == GRAPHICS_PIPELINE_TRIPLANAR)
		{
			textureCount = 1;
		}

		TextureHandle textures [loadInfo.max_texture_count] = {};
		for (s32 i = 0; i < textureCount; ++i)
		{
			textures[i] = assets_get_texture(assets, loadInfo.textures[i]);
		}

		handle 					= graphics_memory_push_material(platformGraphics, loadInfo.pipeline, textureCount, textures);
		assets.materials[id] 	= handle;
	}
	return handle;
}

internal Animation * assets_get_animation(GameAssets & assets, AnimationAssetId id)
{
	Animation * animation = assets.animations[id];

	if (animation == nullptr)
	{
		auto & loadInfo = assets.animationLoadInfos[id];
		animation 		= push_memory<Animation>(*assets.allocator, 1, ALLOC_CLEAR);

		if (loadInfo.filename && loadInfo.gltfNodeName)
		{
			auto file 	= read_gltf_file(*global_transientMemory, loadInfo.filename);
			*animation 	= load_animation_glb(*assets.allocator, file, loadInfo.gltfNodeName);
		}
		else
		{
			log_debug(0, FILE_ADDRESS, "Loading empty animation");
			*animation 	= {};
		}

		assets.animations[id] = animation;
	}

	return animation;

}

internal AnimatedSkeleton * assets_get_skeleton(GameAssets & assets, SkeletonAssetId id)
{
	AnimatedSkeleton * skeleton = assets.skeletons[id];

	if (skeleton == nullptr)
	{
		auto & loadInfo = assets.skeletonLoadInfos[id];

		skeleton 		= push_memory<AnimatedSkeleton>(*assets.allocator, 1, ALLOC_CLEAR);
		auto file 		= read_gltf_file(*global_transientMemory, loadInfo.filename);
		*skeleton 		= load_skeleton_glb(*assets.allocator, file, loadInfo.gltfNodeName);
		
		assets.skeletons[id] = skeleton;
	}	

	return skeleton;
}

struct Scene3d
{
	GameAssets assets;
	AnimatedSkeleton characterAnimatedSkeleton;

	SkySettings skySettings;

	// ---------------------------------------

	// Todo(Leo): Remove these "component" arrays, they are stupidly generic solution, that hide away actual data location, at least the way they are now used
	Array<Transform3D> 			transforms;
	Array<Renderer> 			renderers;

	CollisionSystem3D 			collisionSystem;

	// ---------------------------------------

	Transform3D 		playerCharacterTransform;
	CharacterMotor 		playerCharacterMotor;
	SkeletonAnimator 	playerSkeletonAnimator;
	AnimatedRenderer 	playerAnimatedRenderer;

	PlayerInputState	playerInputState;

	Camera 						worldCamera;
	PlayerCameraController 		playerCamera;
	FreeCameraController		freeCamera;
	MouseCameraController		mouseCamera;

	// ---------------------------------------

	Transform3D 		noblePersonTransform;
	CharacterMotor 		noblePersonCharacterMotor;
	SkeletonAnimator 	noblePersonSkeletonAnimator;
	AnimatedRenderer 	noblePersonAnimatedRenderer;

	s32 	noblePersonMode;

	v3 		nobleWanderTargetPosition;
	f32 	nobleWanderWaitTimer;
	bool32 	nobleWanderIsWaiting;

	// ---------------------------------------

	static constexpr f32 fullWaterLevel = 1;
	Waters 			waters;
	MeshHandle 		waterMesh;
	MaterialHandle 	waterMaterial;

	/// POTS -----------------------------------
		s32 			potCapacity;
		s32 			potCount;
		Transform3D * 	potTransforms;
		f32 * 			potWaterLevels;

		MeshHandle 		potMesh;
		MaterialHandle 	potMaterial;
	// ------------------------------------------------------

	Monuments monuments;

	// ------------------------------------------------------

	s32 				raccoonCount;
	s32 *				raccoonModes;
	Transform3D * 		raccoonTransforms;
	v3 *				raccoonTargetPositions;
	CharacterMotor * 	raccoonCharacterMotors;

	MeshHandle 		raccoonMesh;
	MaterialHandle 	raccoonMaterial;

	// ------------------------------------------------------

	s32 playerCarryState;
	s32 carriedItemIndex;

	s32 			fallingObjectCapacity;
	s32 			fallingObjectCount;
	FallingObject * fallingObjects;

	// ------------------------------------------------------

	Transform3D 	trainTransform;
	MeshHandle 		trainMesh;
	MaterialHandle 	trainMaterial;

	v3 trainStopPosition;
	v3 trainFarPositionA;
	v3 trainFarPositionB;

	s32 trainMoveState;
	s32 trainTargetReachedMoveState;

	s32 trainWayPointIndex;

	f32 trainFullSpeed;
	f32 trainStationMinSpeed;
	f32 trainAcceleration;
	f32 trainWaitTimeOnStop;
	f32 trainBrakeBeforeStationDistance;

	f32 trainCurrentWaitTime;
	f32 trainCurrentSpeed;

	v3 trainCurrentTargetPosition;
	v3 trainCurrentDirection;

	// ------------------------------------------------------

	// Note(Leo): There are multiple mesh handles here
	s32 			terrainCount;
	m44 * 			terrainTransforms;
	MeshHandle * 	terrainMeshes;
	MaterialHandle 	terrainMaterial;

	m44 			seaTransform;
	MeshHandle 		seaMesh;
	MaterialHandle 	seaMaterial;

	// ----------------------------------------------------------
	
	bool32 		drawMCStuff;
	
	f32 			metaballGridScale;
	MaterialHandle 	metaballMaterial;

	m44 		metaballTransform;

	u32 		metaballVertexCapacity;
	u32 		metaballVertexCount;
	Vertex * 	metaballVertices;

	u32 		metaballIndexCapacity;
	u32 		metaballIndexCount;
	u16 * 		metaballIndices;


	m44 metaballTransform2;
	
	u32 		metaballVertexCapacity2;
	u32 		metaballVertexCount2;
	Vertex * 	metaballVertices2;

	u32 		metaballIndexCapacity2;
	u32 		metaballIndexCount2;
	u16 *		metaballIndices2;


	// ----------------------------------------------------------

	// Sky
	ModelHandle 	skybox;

	// Random
	bool32 		getSkyColorFromTreeDistance;

	Gui 		gui;
	CameraMode 	cameraMode;
	bool32		drawDebugShadowTexture;


	MenuView menuView;

	GuiTextureHandle guiPanelImage;
	v4 guiPanelColour;

	// Todo(Leo): this is kinda too hacky
	constexpr static s32 	timeScaleCount = 3;
	s32 					timeScaleIndex;

	Array2<Tree3> 	trees;
	s32 			treeIndex;
	Tree3Settings 	treeSettings [2];
};

internal void read_settings_file(Scene3d * scene)
{
	PlatformFileHandle file = platform_file_open("settings", FILE_MODE_READ);

	s32 fileSize 	= platform_file_get_size(file);
	char * buffer 	= push_memory<char>(*global_transientMemory, fileSize, 0);

	platform_file_read(file, fileSize, buffer);
	platform_file_close(file);	

	String fileAsString = {fileSize, buffer};
	String originalString = fileAsString;

	while(fileAsString.length > 0)
	{
		String line 		= string_extract_line (fileAsString);
		String identifier 	= string_extract_until_character(line, '=');

		string_extract_until_character(fileAsString, '[');
		string_extract_line(fileAsString);

		String block = string_extract_until_character(fileAsString, ']');

		auto deserialize_if_id_matches = [&identifier, &block](char const * name, auto & serializedProperties)
		{
			if(string_equals(identifier, name))
			{
				deserialize_properties(serializedProperties, block);
			}
		};

		deserialize_if_id_matches("sky", scene->skySettings);
		deserialize_if_id_matches("tree_0", scene->treeSettings[0]);
		deserialize_if_id_matches("tree_1", scene->treeSettings[1]);

		string_eat_leading_spaces_and_lines(fileAsString);
	}
}

internal void write_settings_file(Scene3d * scene)
{
	PlatformFileHandle file = platform_file_open("settings", FILE_MODE_WRITE);

	auto serialize = [file](char const * label, auto const & serializedData)
	{
		push_memory_checkpoint(*global_transientMemory);
	
		constexpr s32 capacity 			= 2000;
		String serializedFormatString 	= push_temp_string(capacity);

		string_append_format(serializedFormatString, capacity, label, " = \n[\n");
		serialize_properties(serializedData, serializedFormatString, capacity);
		string_append_cstring(serializedFormatString, capacity, "]\n\n");

		platform_file_write(file, serializedFormatString.length, serializedFormatString.memory);

		pop_memory_checkpoint(*global_transientMemory);
	};

	serialize("sky", scene->skySettings);
	serialize("tree_0", scene->treeSettings[0]);
	serialize("tree_1", scene->treeSettings[1]);


	platform_file_close(file);
}


// struct Scene3dSaveLoad
// {
// 	s32 playerLocation;
// 	s32 watersLocation;
// 	s32 treesLocation;
// 	s32 potsLocation;
// };

template <typename T>
internal inline void file_write_struct(PlatformFileHandle file, T * value)
{
	platformApi->write_file(file, sizeof(T), value);
}

template <typename T>
internal inline void file_write_memory(PlatformFileHandle file, s32 count, T * memory)
{
	platformApi->write_file(file, sizeof(T) * count, memory);
}

template <typename T>
internal inline void file_read_struct(PlatformFileHandle file, T * value)
{
	platformApi->read_file(file, sizeof(T), value);
}

template <typename T>
internal inline void file_read_memory(PlatformFileHandle file, s32 count, T * memory)
{
	platformApi->read_file(file, sizeof(T) * count, memory);
}

internal void write_save_file(Scene3d * scene)
{
	auto file = platform_file_open("save_game.fssave", FILE_MODE_WRITE);
	/*

	// Note(Leo): as we go, set values to this struct, and write this to file last.
	Scene3dSaveLoad save;
	platformApi->set_file_position(file, sizeof(save));

	save.playerLocation = platformApi->get_file_position(file);
	platformApi->write_file(file, sizeof(Transform3D), &scene->playerCharacterTransform);

	save.watersLocation = platformApi->get_file_position(file);
	file_write_struct(file, &scene->waters.capacity);
	file_write_struct(file,	&scene->waters.count);
	file_write_memory(file, scene->waters.count, scene->waters.transforms);
	file_write_memory(file, scene->waters.count, scene->waters.levels);

	save.treesLocation = platformApi->get_file_position(file);
	file_write_struct(file, &scene->lSystemCapacity);
	file_write_struct(file, &scene->lSystemCount);
	for (s32 i = 0; i < scene->lSystemCount; ++i)
	{
		file_write_struct(file, &scene->lSystems[i].wordCapacity);
		file_write_struct(file, &scene->lSystems[i].wordCount);

		file_write_memory(file, scene->lSystems[i].wordCount, scene->lSystems[i].aWord);

		file_write_struct(file, &scene->lSystems[i].timeSinceLastUpdate);
		file_write_struct(file, &scene->lSystems[i].position);
		file_write_struct(file, &scene->lSystems[i].rotation);
		file_write_struct(file, &scene->lSystems[i].type);
		file_write_struct(file, &scene->lSystems[i].totalAge);
		file_write_struct(file, &scene->lSystems[i].maxAge);
	}
	file_write_memory(file, scene->lSystemCount, scene->lSystemsPotIndices);

	save.potsLocation = platformApi->get_file_position(file);
	file_write_struct(file, &scene->potCapacity);
	file_write_struct(file, &scene->potCount);
	file_write_memory(file, scene->potCount, scene->potTransforms);
	file_write_memory(file, scene->potCount, scene->potWaterLevels);

	platformApi->set_file_position(file, 0);
	file_write_memory(file, 1, &save);

	platformApi->close_file(file);
	*/
}

#include "scene3d_gui.cpp"

struct SnapOnGround
{
	CollisionSystem3D & collisionSystem;

	v3 operator()(v2 position)
	{
		v3 result = {position.x, position.y, get_terrain_height(collisionSystem, position)};
		return result;
	}

	v3 operator()(v3 position)
	{
		v3 result = {position.x, position.y, get_terrain_height(collisionSystem, position.xy)};
		return result;
	}
};


// Todo(Leo): add this to syntax higlight, so that 'GAME UPDATE' is different color
/// ------------------ GAME UPDATE -------------------------
internal bool32 update_scene_3d(void * scenePtr, PlatformInput const & input, PlatformTime const & time)
{
	Scene3d * scene = reinterpret_cast<Scene3d*>(scenePtr);
	
	/// ****************************************************************************
	/// DEFINE UPDATE FUNCTIONS
	// Todo(Leo): Get water functions are used in one place only, inline them
	auto get_water = [	carryState 			= scene->playerCarryState,
						carriedItemIndex 	= scene->carriedItemIndex]
					(Waters & waters, v3 position, f32 distanceThreshold, f32 requestedAmount) -> f32
	{	
		s32 closestIndex;
		f32 closestDistance = highest_f32;

		for (s32 i = 0; i < waters.count; ++i)
		{
			if (carriedItemIndex == i && carryState == CARRY_WATER)
			{
				continue;
			}

			f32 distance = magnitude_v3(waters.transforms[i].position - position);
			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestIndex 	= i;
			}
		}

		f32 amount = 0;

		if (closestDistance < distanceThreshold)
		{
			waters.levels[closestIndex] -= requestedAmount;
			if (waters.levels[closestIndex] < 0)
			{
				waters.count -= 1;
				waters.transforms[closestIndex] 	= waters.transforms[waters.count];
				waters.levels[closestIndex] 		= waters.levels[waters.count];
			}

			amount = requestedAmount;
		}

		return amount;
	};

	auto get_water_from_pot = [](f32 * potWaterLevels, s32 index, f32 amount) -> f32
	{
		amount = min_f32(amount, potWaterLevels[index]);
		potWaterLevels[index] -= amount;
		return amount;
	};

	auto simple_get_water = [get_water, &waters = scene->waters] (v3 position, f32 amount) ->f32
	{
		return get_water(waters, position, 1.0f, amount);
	};

	SnapOnGround snap_on_ground = {scene->collisionSystem};

	/// ****************************************************************************
	/// TIME

	f32 const timeScales [scene->timeScaleCount] { 1.0f, 0.1f, 0.5f }; 
	f32 scaledTime 		= time.elapsedTime * timeScales[scene->timeScaleIndex];
	f32 unscaledTime 	= time.elapsedTime;

	/// ****************************************************************************

	gui_start_frame(scene->gui, input, unscaledTime);

	/* Sadly, we need to draw skybox before game logic, because otherwise
	debug lines would be hidden. This can be fixed though, just make debug pipeline similar to shadows. */ 
	graphics_draw_model(platformGraphics, scene->skybox, identity_m44, false, nullptr, 0);

	// Game Logic section
	switch (scene->cameraMode)
	{
		case CAMERA_MODE_PLAYER:
		{
			CharacterInput playerCharacterMotorInput = {};

			if (scene->menuView == MENU_OFF)
			{
				if (is_clicked(input.down))
				{
					scene->nobleWanderTargetPosition 	= scene->playerCharacterTransform.position;
					scene->nobleWanderWaitTimer 		= 0;
					scene->nobleWanderIsWaiting 		= false;
				}

				playerCharacterMotorInput = update_player_input(scene->playerInputState, scene->worldCamera, input);
			}

			update_character_motor(scene->playerCharacterMotor, playerCharacterMotorInput, scene->collisionSystem, scaledTime, DEBUG_LEVEL_PLAYER);

			update_camera_controller(&scene->playerCamera, scene->playerCharacterTransform.position, input, scaledTime);

			scene->worldCamera.position = scene->playerCamera.position;
			scene->worldCamera.direction = scene->playerCamera.direction;

			scene->worldCamera.farClipPlane = 1000;

			/// ---------------------------------------------------------------------------------------------------

			FS_DEBUG_NPC(debug_draw_circle_xy(scene->nobleWanderTargetPosition + v3{0,0,0.5}, 1.0, colour_bright_green));
			FS_DEBUG_NPC(debug_draw_circle_xy(scene->nobleWanderTargetPosition + v3{0,0,0.5}, 0.9, colour_bright_green));
		} break;

		case CAMERA_MODE_ELEVATOR:
		{
			m44 cameraMatrix = update_free_camera(scene->freeCamera, input, unscaledTime);

			scene->worldCamera.position = scene->freeCamera.position;
			scene->worldCamera.direction = scene->freeCamera.direction;

			/// Document(Leo): Teleport player
			if (scene->menuView == MENU_OFF && is_clicked(input.A))
			{
				scene->menuView = MENU_CONFIRM_TELEPORT;
				gui_ignore_input();
				gui_reset_selection();
			}

			scene->worldCamera.farClipPlane = 2000;
		} break;

		case CAMERA_MODE_MOUSE_AND_KEYBOARD:
		{
			update_mouse_camera(scene->mouseCamera, input, unscaledTime);

			scene->worldCamera.position = scene->mouseCamera.position;
			scene->worldCamera.direction = scene->mouseCamera.direction;

			/// Document(Leo): Teleport player
			if (scene->menuView == MENU_OFF && is_clicked(input.A))
			{
				scene->menuView = MENU_CONFIRM_TELEPORT;
				gui_ignore_input();
				gui_reset_selection();
			}

			scene->worldCamera.farClipPlane = 2000;
		} break;

		case CAMERA_MODE_COUNT:
			Assert(false && "Bad execution path");
			break;
	}


	/// *******************************************************************************************

	update_camera_system(&scene->worldCamera, platformGraphics, platformWindow);

	{
		v3 lightDirection = {0,0,1};
		lightDirection = rotate_v3(axis_angle_quaternion(right_v3, scene->skySettings.sunHeightAngle * π), lightDirection);
		lightDirection = rotate_v3(axis_angle_quaternion(up_v3, scene->skySettings.sunOrbitAngle * 2 * π), lightDirection);
		lightDirection = normalize_v3(-lightDirection);

		Light light = { .direction 	= lightDirection, //normalize_v3({-1, 1.2, -8}), 
						.color 		= v3{0.95, 0.95, 0.9}};
		v3 ambient 	= {0.05, 0.05, 0.3};

		light.skyColorSelection = scene->skySettings.skyColourSelection;

		light.skyGroundColor.rgb 	= scene->skySettings.skyGradientGround;
		light.skyBottomColor.rgb 	= scene->skySettings.skyGradientBottom;
		light.skyTopColor.rgb 		= scene->skySettings.skyGradientTop;

		light.horizonHaloColorAndFalloff.rgb 	= scene->skySettings.horizonHaloColour;
		light.horizonHaloColorAndFalloff.a 		= scene->skySettings.horizonHaloFalloff;

		light.sunHaloColorAndFalloff.rgb 	= scene->skySettings.sunHaloColour;
		light.sunHaloColorAndFalloff.a 		= scene->skySettings.sunHaloFalloff;

		light.sunDiscColor.rgb 			= scene->skySettings.sunDiscColour;
		light.sunDiscSizeAndFalloff.xy 	= {	scene->skySettings.sunDiscSize,
											scene->skySettings.sunDiscFalloff };

		graphics_drawing_update_lighting(platformGraphics, &light, &scene->worldCamera, ambient);
		HdrSettings hdrSettings = 
		{
			scene->skySettings.hdrExposure,
			scene->skySettings.hdrContrast,
		};
		graphics_drawing_update_hdr_settings(platformGraphics, &hdrSettings);
	}

	/// *******************************************************************************************
	

	/// SPAWN WATER
	bool playerInputAvailable = scene->cameraMode == CAMERA_MODE_PLAYER
								&& scene->menuView == MENU_OFF;

	if (playerInputAvailable && is_clicked(input.Y))
	{
		Waters & waters = scene->waters;

		v2 center = scene->playerCharacterTransform.position.xy;

		s32 spawnCount = 10;
		spawnCount = min_f32(spawnCount, waters.capacity - waters.count);

		for (s32 i = 0; i < spawnCount; ++i)
		{
			f32 distance 	= random_range(1, 5);
			f32 angle 		= random_range(0, 2 * π);

			f32 x = cosine(angle) * distance;
			f32 y = sine(angle) * distance;

			v3 position = { x + center.x, y + center.y, 0 };
			position.z = get_terrain_height(scene->collisionSystem, position.xy);

			waters.transforms[waters.count].position = position;
			waters.transforms[waters.count].rotation = identity_quaternion;
			waters.transforms[waters.count].scale = {1,1,1};

			waters.levels[waters.count] = scene->fullWaterLevel;

			waters.count += 1;
		}
	}

	/// PICKUP OR DROP
	if (playerInputAvailable && is_clicked(input.A))
	{
		auto push_falling_object = [fallingObjects 			= scene->fallingObjects,
									fallingObjectCapacity 	= scene->fallingObjectCapacity,
									&fallingObjectCount 	= scene->fallingObjectCount]
									(s32 type, s32 index)
		{
			Assert(fallingObjectCount < fallingObjectCapacity);

			fallingObjects[fallingObjectCount] 	= {type, index, 0};
			fallingObjectCount 					+= 1;
		};

		v3 playerPosition = scene->playerCharacterTransform.position;
		f32 grabDistance = 1.0f;

		switch(scene->playerCarryState)
		{
			case CARRY_NONE: {
				/* Todo(Leo): Do this properly, taking into account player facing direction and distances etc. */
				auto check_pickup = [	playerPosition 		= scene->playerCharacterTransform.position,
										&playerCarryState 	= scene->playerCarryState,
										&carriedItemIndex 	= scene->carriedItemIndex,
										grabDistance]
									(s32 count, Transform3D const * transforms, s32 carryState)
				{
					for (s32 i = 0; i < count; ++i)
					{
						if (magnitude_v3(playerPosition - transforms[i].position) < grabDistance)
						{
							playerCarryState = carryState;
							carriedItemIndex = i;
						}
					}
				};

				check_pickup(scene->potCount, scene->potTransforms, CARRY_POT);
				check_pickup(scene->waters.count, scene->waters.transforms, CARRY_WATER);
				check_pickup(scene->raccoonCount, scene->raccoonTransforms, CARRY_RACCOON);

				{
					for (auto & tree : scene->trees)
					{
						if (magnitude_v3(playerPosition - tree.position) < grabDistance)
						{
							// f32 maxGrabbableLength 	= 1;
							// f32 maxGrabbableRadius 	= maxGrabbableLength / tree.settings->maxHeightToWidthRatio;
							// bool isGrabbable 		= tree.nodes[tree.branches[0].startNodeIndex].radius > maxGrabbableRadius;

							// if (isGrabbable)
							{
								scene->playerCarryState = CARRY_TREE_3;
								scene->carriedItemIndex = array_2_get_index_of(scene->trees, tree);
								tree.isCarried = true;
							}
						}
					}
				}

			} break;

			case CARRY_POT:
				push_falling_object(CARRY_POT, scene->carriedItemIndex);
				scene->carriedItemIndex = -1;
				scene->playerCarryState = CARRY_NONE;

				break;
	
			case CARRY_WATER:
			{
				Transform3D & waterTransform = scene->waters.transforms[scene->carriedItemIndex];

				push_falling_object(CARRY_WATER, scene->carriedItemIndex);
				scene->carriedItemIndex = -1;
				scene->playerCarryState = CARRY_NONE;

				constexpr f32 waterSnapDistance 	= 0.5f;
				constexpr f32 smallPotMaxWaterLevel = 1.0f;

				f32 droppedWaterLevel = scene->waters.levels[scene->carriedItemIndex];

				for (s32 i = 0; i < scene->potCount; ++i)
				{
					f32 distance = magnitude_v3(scene->potTransforms[i].position - waterTransform.position);
					if (distance < waterSnapDistance)
					{
						scene->potWaterLevels[i] += droppedWaterLevel;
						scene->potWaterLevels[i] = min_f32(scene->potWaterLevels[i] + droppedWaterLevel, smallPotMaxWaterLevel);

						scene->waters.count -= 1;
						scene->waters.transforms[scene->carriedItemIndex] 	= scene->waters.transforms[scene->waters.count];
						scene->waters.levels[scene->carriedItemIndex] 		= scene->waters.levels[scene->waters.count];

						break;	
					}
				}


			} break;

			case CARRY_RACCOON:
			{
				scene->raccoonTransforms[scene->carriedItemIndex].rotation = identity_quaternion;
				push_falling_object(CARRY_RACCOON, scene->carriedItemIndex);
				scene->carriedItemIndex = -1;
				scene->playerCarryState = CARRY_NONE;

			} break;

			case CARRY_TREE_3:
			{	
				push_falling_object(CARRY_TREE_3, scene->carriedItemIndex);
				scene->carriedItemIndex = -1;
				scene->playerCarryState = CARRY_NONE;
			} break;
		}
	} // endif input


	v3 carriedPosition 			= multiply_point(transform_matrix(scene->playerCharacterTransform), {0, 0.7, 0.7});
	quaternion carriedRotation 	= scene->playerCharacterTransform.rotation;
	switch(scene->playerCarryState)
	{
		case CARRY_POT:
			scene->potTransforms[scene->carriedItemIndex].position = carriedPosition;
			scene->potTransforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_WATER:
			scene->waters.transforms[scene->carriedItemIndex].position = carriedPosition;
			scene->waters.transforms[scene->carriedItemIndex].rotation = carriedRotation;
			break;

		case CARRY_RACCOON:
		{
			scene->raccoonTransforms[scene->carriedItemIndex].position 	= carriedPosition + v3{0,0,0.2};

			v3 right = rotate_v3(carriedRotation, right_v3);
			scene->raccoonTransforms[scene->carriedItemIndex].rotation 	= carriedRotation * axis_angle_quaternion(right, 1.4f);


		} break;

		case CARRY_TREE_3:
			scene->trees[scene->carriedItemIndex].position = carriedPosition;
			break;

		case CARRY_NONE:
			break;


		default:
			Assert(false && "That cannot be carried!");
			break;
	}

	/// UPDATE TREES IN POTS POSITIONS
	// Todo(Leo):...

	// UPDATE falling objects
	{
		for (s32 i = 0; i < scene->fallingObjectCount; ++i)
		{
			s32 index 		= scene->fallingObjects[i].index;
			bool cancelFall = index == scene->carriedItemIndex;

			f32 & fallSpeed = scene->fallingObjects[i].fallSpeed;

			v3 * position;

			switch (scene->fallingObjects[i].type)
			{
				case CARRY_RACCOON:	position = &scene->raccoonTransforms[index].position; 	break;
				case CARRY_WATER: 	position = &scene->waters.transforms[index].position; 	break;
				case CARRY_POT:		position = &scene->potTransforms[index].position;		break;
				case CARRY_TREE_3: 	position = &scene->trees[index].position; 				break;
			}
		
			f32 targetHeight 	= get_terrain_height(scene->collisionSystem, position->xy);

			if (position->z < targetHeight or cancelFall)
			{
				if (scene->fallingObjects[i].type == CARRY_TREE_3)
				{
					scene->trees[index].isCarried = false;
				}

				if(!cancelFall)
				{
					position->z = targetHeight;
				}

				/// POP falling object
				scene->fallingObjects[i] = scene->fallingObjects[scene->fallingObjectCount];
				scene->fallingObjectCount -= 1;
				i--;
			}
			else
			{
				fallSpeed 			+= scaledTime * physics_gravity_acceleration;
				position->z 		+= scaledTime * fallSpeed;
			}
		}
	}
	
	// -----------------------------------------------------------------------------------------------------------
	/// TRAIN
	{
		v3 trainWayPoints [] = 
		{
			scene->trainStopPosition, 
			scene->trainFarPositionA,
			scene->trainStopPosition, 
			scene->trainFarPositionB,
		};

		if (scene->trainMoveState == TRAIN_MOVE)
		{

			v3 trainMoveVector 	= scene->trainCurrentTargetPosition - scene->trainTransform.position;
			f32 directionDot 	= dot_v3(trainMoveVector, scene->trainCurrentDirection);
			f32 moveDistance	= magnitude_v3(trainMoveVector);

			if (moveDistance > scene->trainBrakeBeforeStationDistance)
			{
				scene->trainCurrentSpeed += scaledTime * scene->trainAcceleration;
				scene->trainCurrentSpeed = min_f32(scene->trainCurrentSpeed, scene->trainFullSpeed);
			}
			else
			{
				scene->trainCurrentSpeed -= scaledTime * scene->trainAcceleration;
				scene->trainCurrentSpeed = max_f32(scene->trainCurrentSpeed, scene->trainStationMinSpeed);
			}

			if (directionDot > 0)
			{
				scene->trainTransform.position += scene->trainCurrentDirection * scene->trainCurrentSpeed * scaledTime;
			}
			else
			{
				scene->trainMoveState 		= TRAIN_WAIT;
				scene->trainCurrentWaitTime = 0;
			}

		}
		else
		{
			scene->trainCurrentWaitTime += scaledTime;
			if (scene->trainCurrentWaitTime > scene->trainWaitTimeOnStop)
			{
				scene->trainMoveState 		= TRAIN_MOVE;
				scene->trainCurrentSpeed 	= 0;

				v3 start 	= trainWayPoints[scene->trainWayPointIndex];
				v3 end 		= trainWayPoints[(scene->trainWayPointIndex + 1) % array_count(trainWayPoints)];

				scene->trainWayPointIndex += 1;
				scene->trainWayPointIndex %= array_count(trainWayPoints);

				scene->trainCurrentTargetPosition 	= end;
				scene->trainCurrentDirection 		= normalize_v3(end - start);
			}
		}
	}

	// -----------------------------------------------------------------------------------------------------------
	/// NOBLE PERSON CHARACTER
	{
		CharacterInput nobleCharacterMotorInput = {};

		switch(scene->noblePersonMode)
		{
			case NOBLE_WANDER_AROUND:
			{
				if (scene->nobleWanderIsWaiting)
				{
					scene->nobleWanderWaitTimer -= scaledTime;
					if (scene->nobleWanderWaitTimer < 0)
					{
						scene->nobleWanderTargetPosition = { random_range(-99, 99), random_range(-99, 99)};
						scene->nobleWanderIsWaiting = false;
					}


					m44 gizmoTransform = make_transform_matrix(	scene->noblePersonTransform.position + up_v3 * scene->noblePersonTransform.scale.z * 2.0f, 
																scene->noblePersonTransform.rotation,
																scene->nobleWanderWaitTimer);
					FS_DEBUG_NPC(debug_draw_diamond_xz(gizmoTransform, colour_muted_red));
				}
				
				f32 distance = magnitude_v2(scene->noblePersonTransform.position.xy - scene->nobleWanderTargetPosition.xy);
				if (distance < 1.0f && scene->nobleWanderIsWaiting == false)
				{
					scene->nobleWanderWaitTimer = 10;
					scene->nobleWanderIsWaiting = true;
				}


				v3 input 			= {};
				input.xy	 		= scene->nobleWanderTargetPosition.xy - scene->noblePersonTransform.position.xy;
				f32 inputMagnitude 	= magnitude_v3(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= clamp_f32(inputMagnitude, 0.0f, 1.0f);
				input 				= input * inputMagnitude;

				nobleCharacterMotorInput = {input, false, false};

			} break;
		}
	
		update_character_motor(	scene->noblePersonCharacterMotor,
								nobleCharacterMotorInput,
								scene->collisionSystem,
								scaledTime,
								DEBUG_LEVEL_NPC);
	}

	// -----------------------------------------------------------------------------------------------------------
	/// Update RACCOONS
	{

		for(s32 i = 0; i < scene->raccoonCount; ++i)
		{
			CharacterInput raccoonCharacterMotorInput = {};
	
			v3 toTarget 			= scene->raccoonTargetPositions[i] - scene->raccoonTransforms[i].position;
			f32 distanceToTarget 	= magnitude_v3(toTarget);

			v3 input = {};

			if (distanceToTarget < 1.0f)
			{
				scene->raccoonTargetPositions[i] 	= snap_on_ground(random_inside_unit_square() * 100 - v3{50,50,0});
				toTarget 							= scene->raccoonTargetPositions[i] - scene->raccoonTransforms[i].position;
			}
			else
			{
				input.xy	 		= scene->raccoonTargetPositions[i].xy - scene->raccoonTransforms[i].position.xy;
				f32 inputMagnitude 	= magnitude_v3(input);
				input 				= input / inputMagnitude;
				inputMagnitude 		= clamp_f32(inputMagnitude, 0.0f, 1.0f);
				input 				= input * inputMagnitude;
			}

			raccoonCharacterMotorInput = {input, false, false};

			bool isCarried = (scene->playerCarryState == CARRY_RACCOON) && (scene->carriedItemIndex == i);

			if (isCarried == false)
			{
				update_character_motor( scene->raccoonCharacterMotors[i],
										raccoonCharacterMotorInput,
										scene->collisionSystem,
										scaledTime,
										DEBUG_LEVEL_NPC);
			}

			// debug_draw_circle_xy(snap_on_ground(scene->raccoonTargetPositions[i].xy) + v3{0,0,1}, 1, colour_bright_red, DEBUG_LEVEL_ALWAYS);
		}


	}

	// -----------------------------------------------------------------------------------------------------------

	// Todo(Leo): Rather use something like submit_collider() with what every collider can decide themselves, if they want to
	// contribute to collisions this frame or not.
	precompute_colliders(scene->collisionSystem);

	/// SUBMIT COLLIDERS
	{
		// Todo(Leo): Maybe make something like that these would have predetermined range, that is only updated when
		// tree has grown a certain amount or somthing. These are kinda semi-permanent by nature.

		auto submit_cylinder_colliders = [&collisionSystem = scene->collisionSystem](f32 radius, f32 halfHeight, s32 count, Transform3D * transforms)
		{
			for (s32 i = 0; i < count; ++i)
			{
				submit_cylinder_collider(collisionSystem, radius, halfHeight, transforms[i].position + v3{0,0,halfHeight});
			}
		};

		f32 smallPotColliderRadius = 0.3;
		f32 smallPotColliderHeight = 0.58;
		f32 smallPotHalfHeight = smallPotColliderHeight / 2;
		submit_cylinder_colliders(smallPotColliderRadius, smallPotHalfHeight, scene->potCount, scene->potTransforms);

		constexpr f32 baseRadius = 0.12;
		constexpr f32 baseHeight = 2;

		// NEW TREES 3
		for (auto tree : scene->trees)
		{
			constexpr f32 colliderHalfHeight = 1.0f;
			constexpr v3 colliderOffset = {0, 0, colliderHalfHeight};
			submit_cylinder_collider(	scene->collisionSystem,
										tree.nodes[tree.branches[0].startNodeIndex].radius,
										colliderHalfHeight,
										tree.position + colliderOffset);
		}
	}


	update_skeleton_animator(scene->playerSkeletonAnimator, scaledTime);
	update_skeleton_animator(scene->noblePersonSkeletonAnimator, scaledTime);
	
	
	/// DRAW POTS
	{
		// Todo(Leo): store these as matrices, we can easily retrieve position (that is needed somwhere) from that too.
		m44 * potTransformMatrices = push_memory<m44>(*global_transientMemory, scene->potCount, ALLOC_NO_CLEAR);
		for(s32 i = 0; i < scene->potCount; ++i)
		{
			potTransformMatrices[i] = transform_matrix(scene->potTransforms[i]);
		}
		graphics_draw_meshes(platformGraphics, scene->potCount, potTransformMatrices, scene->potMesh, scene->potMaterial);
	}

	/// DRAW STATIC SCENERY
	{
		for(s32 i = 0; i < scene->terrainCount; ++i)
		{
			graphics_draw_meshes(platformGraphics, 1, scene->terrainTransforms + i, scene->terrainMeshes[i], scene->terrainMaterial);
		}

		graphics_draw_meshes(platformGraphics, 1, &scene->seaTransform, scene->seaMesh, scene->seaMaterial);

		scene3d_draw_monuments(scene->monuments);
	}


	/// DEBUG DRAW COLLIDERS
	{
		for (auto const & collider : scene->collisionSystem.precomputedBoxColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_box(collider.transform, colour_muted_green));
		}

		for (auto const & collider : scene->collisionSystem.staticBoxColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_box(collider.transform, colour_dark_green));
		}

		for (auto const & collider : scene->collisionSystem.precomputedCylinderColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
		}

		for (auto const & collider : scene->collisionSystem.submittedCylinderColliders)
		{
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center - v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
			FS_DEBUG_BACKGROUND(debug_draw_circle_xy(collider.center + v3{0, 0, collider.halfHeight}, collider.radius, colour_bright_green));
		}

		FS_DEBUG_PLAYER(debug_draw_circle_xy(scene->playerCharacterTransform.position + v3{0,0,0.7}, 0.25f, colour_bright_green));
	}

	  //////////////////////////////
	 /// 	RENDERING 			///
	//////////////////////////////
	/*
		TODO(LEO): THIS COMMENT PIECE IS NOT ACCURATE ANYMORE

		1. Update camera and lights
		2. Render normal models
		3. Render animated models
	*/

	if (scene->drawMCStuff)
	{
		v3 position = multiply_point(scene->metaballTransform, {0,0,0});
		// debug_draw_circle_xy(position, 4, {1,0,1,1}, DEBUG_LEVEL_ALWAYS);
		// graphics_draw_meshes(platformGraphics, 1, &scene->metaballTransform, scene->metaballMesh, scene->metaballMaterial);

		local_persist f32 testX = 0;
		local_persist f32 testY = 0;
		local_persist f32 testZ = 0;
		local_persist f32 testW = 0;

		testX += scaledTime;
		testY += scaledTime * 1.2;
		testZ += scaledTime * 0.9;
		testW += scaledTime * 1.7;

		v4 positions[] =
		{
			{mathfun_pingpong_f32(testX, 5),2,2,										2},
			{1,mathfun_pingpong_f32(testY, 3),1,										1},
			{4,3,mathfun_pingpong_f32(testZ, 3) + 1,									1.5},
			{mathfun_pingpong_f32(testW, 4) + 1, mathfun_pingpong_f32(testY, 3), 3,		1.2},
		};

		generate_mesh_marching_cubes(	scene->metaballVertexCapacity, scene->metaballVertices, &scene->metaballVertexCount,
										scene->metaballIndexCapacity, scene->metaballIndices, &scene->metaballIndexCount,
										sample_four_sdf_balls, positions, scene->metaballGridScale);

		graphics_draw_procedural_mesh(	platformGraphics,
											scene->metaballVertexCount, scene->metaballVertices,
											scene->metaballIndexCount, scene->metaballIndices,
											scene->metaballTransform,
											scene->metaballMaterial);

		auto sample_sdf_2 = [](v3 position, void const * data)
		{
			v3 a = {2,2,2};
			v3 b = {5,3,5};

			f32 rA = 1;
			f32 rB = 1;

			// f32 d = min_f32(1, max_f32(0, dot_v3()))

			f32 t = min_f32(1, max_f32(0, dot_v3(position - a, b - a) / square_magnitude_v3(b-a)));
			f32 d = magnitude_v3(position - a  - t * (b -a));

			return d - lerp_f32(0.5,0.1,t);

			// return min_f32(magnitude_v3(a - position) - rA, magnitude_v3(b - position) - rB);
		};

		f32 fieldMemory [] =
		{
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			-5,-5,-5,-5,-5,
			
			5,5,5,5,5,
			5,-2,-2,5,5,
			5,-1,-1,-1,5,
			5,5,5,5,5,
			
			5,5,5,5,5,
			5,5,5,5,5,
			5,5,5,5,5,
			5,5,5,5,5,
		};

		// v3 fieldSize = {5,4,4};

		VoxelField field = {5, 4, 4, fieldMemory};

		generate_mesh_marching_cubes(	scene->metaballVertexCapacity2, scene->metaballVertices2, &scene->metaballVertexCount2,
										scene->metaballIndexCapacity2, scene->metaballIndices2, &scene->metaballIndexCount2,
										sample_heightmap_for_mc, &scene->collisionSystem.terrainCollider, scene->metaballGridScale);

		FS_DEBUG_ALWAYS(debug_draw_circle_xy(multiply_point(scene->metaballTransform2, scene->metaballVertices2[0].position), 5.0f, colour_bright_green));
		// log_debug(0) << multiply_point(scene->metaballTransform2, scene->metaballVertices2[0].position);

		if (scene->metaballVertexCount2 > 0 && scene->metaballIndexCount2 > 0)
		{
			graphics_draw_procedural_mesh(	platformGraphics,
												scene->metaballVertexCount2, scene->metaballVertices2,
												scene->metaballIndexCount2, scene->metaballIndices2,
												scene->metaballTransform2,
												scene->metaballMaterial);
		}
	}

	{
		m44 trainTransformMatrix = transform_matrix(scene->trainTransform);
		graphics_draw_meshes(platformGraphics, 1, &trainTransformMatrix, scene->trainMesh, scene->trainMaterial);
	}

	/// DRAW RACCOONS
	{
		m44 * raccoonTransformMatrices = push_memory<m44>(*global_transientMemory, scene->raccoonCount, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < scene->raccoonCount; ++i)
		{
			raccoonTransformMatrices[i] = transform_matrix(scene->raccoonTransforms[i]);
		}
		graphics_draw_meshes(platformGraphics, scene->raccoonCount, raccoonTransformMatrices, scene->raccoonMesh, scene->raccoonMaterial);
	}

	for (auto & renderer : scene->renderers)
	{
		graphics_draw_model(platformGraphics, renderer.model,
								transform_matrix(*renderer.transform),
								renderer.castShadows,
								nullptr, 0);
	}


	/// PLAYER
	/// CHARACTER 2
	{

		m44 boneTransformMatrices [32];
		
		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, scene->playerSkeletonAnimator);

		graphics_draw_model(platformGraphics, 	scene->playerAnimatedRenderer.model,
												transform_matrix(scene->playerCharacterTransform),
												true,
												boneTransformMatrices, array_count(boneTransformMatrices));

		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, scene->noblePersonSkeletonAnimator);

		graphics_draw_model(platformGraphics, 	scene->playerAnimatedRenderer.model,
												transform_matrix(scene->noblePersonTransform),
												true,
												boneTransformMatrices, array_count(boneTransformMatrices));
	}


	/// DRAW UNUSED WATER
	if (scene->waters.count > 0)
	{
		m44 * waterTransforms = push_memory<m44>(*global_transientMemory, scene->waters.count, ALLOC_NO_CLEAR);
		for (s32 i = 0; i < scene->waters.count; ++i)
		{
			waterTransforms[i] = transform_matrix(	scene->waters.transforms[i].position,
													scene->waters.transforms[i].rotation,
													make_uniform_v3(scene->waters.levels[i] / scene->fullWaterLevel));
		}
		graphics_draw_meshes(platformGraphics, scene->waters.count, waterTransforms, scene->waterMesh, scene->waterMaterial);
	}

	for (auto & tree : scene->trees)
	{
		GetWaterFunc get_water = {scene->waters, scene->carriedItemIndex, scene->playerCarryState == CARRY_WATER };
		update_tree_3(tree, scaledTime, get_water);
		
		tree.leaves.position = tree.position;
		tree.leaves.rotation = identity_quaternion;


		m44 transform = translation_matrix(tree.position);
		// Todo(Leo): make some kind of SLOPPY_ASSERT macro thing
		if (tree.mesh.vertices.count > 0 || tree.mesh.indices.count > 0)
		{
			graphics_draw_procedural_mesh(	platformGraphics,
											tree.mesh.vertices.count, tree.mesh.vertices.memory,
											tree.mesh.indices.count, tree.mesh.indices.memory,
											transform, assets_get_material(scene->assets, MATERIAL_ASSET_TREE));
		}

		if (tree.drawSeed)
		{
			graphics_draw_meshes(platformGraphics, 1, &transform, tree.seedMesh, tree.seedMaterial);
		}

		FS_DEBUG_BACKGROUND(debug_draw_circle_xy(tree.position, 2, colour_bright_red));
	}

	/// DRAW LEAVES FOR BOTH LSYSTEM ARRAYS IN THE END BECAUSE OF LEAF SHDADOW INCONVENIENCE
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): leaves are drawn last, so we can bind their shadow pipeline once, and not rebind the normal shadow thing. Fix this inconvenience!
	// Todo(Leo): This works ok currently, but do fix this, maybe do a proper command buffer for these
	{
		for (auto & tree : scene->trees)
		{
			draw_leaves(tree.leaves, scaledTime, tree.settings->leafSize, tree.settings->leafColour);
		}
	}

	// ------------------------------------------------------------------------

	auto gui_result = do_gui(scene, input);

	// if (scene->settings.dirty)
	// {
	// 	write_settings_file(scene->settings, scene);
	// 	scene->settings.dirty = false;
	// }

	return gui_result;
}

internal void * load_scene_3d(MemoryArena & persistentMemory, PlatformFileHandle saveFile)
{
	Scene3d * scene = push_memory<Scene3d>(persistentMemory, 1, ALLOC_CLEAR);
	*scene 			= {};

	// Note(Leo): We currently only have statically allocated stuff (or rather allocated with scene),
	// this can be read here, at the top. If we need to allocate some stuff, we need to reconsider.
	read_settings_file(scene);

	// ----------------------------------------------------------------------------------

	scene_3d_initialize_gui(scene);

	// ----------------------------------------------------------------------------------

	// Todo(Leo): Think more about implications of storing pointer to persistent memory here
	// Note(Leo): We have not previously used persistent allocation elsewhere than in this load function
	scene->assets = init_game_assets(&persistentMemory);

	// TODO(Leo): these should probably all go away
	// Note(Leo): amounts are SWAG, rethink.
	scene->transforms 	= allocate_array<Transform3D>(persistentMemory, 1200);
	scene->renderers 	= allocate_array<Renderer>(persistentMemory, 600);

	scene->collisionSystem.boxColliders 		= allocate_array<BoxCollider>(persistentMemory, 600);
	scene->collisionSystem.cylinderColliders 	= allocate_array<CylinderCollider>(persistentMemory, 600);
	scene->collisionSystem.staticBoxColliders 	= allocate_array<StaticBoxCollider>(persistentMemory, 2000);

	scene->fallingObjectCapacity = 100;
	scene->fallingObjectCount = 0;
	scene->fallingObjects = push_memory<FallingObject>(persistentMemory, scene->fallingObjectCapacity, ALLOC_CLEAR);

	log_application(1, "Allocations succesful! :)");

	// ----------------------------------------------------------------------------------

	{
		// Todo(Leo): gui assets to systemtic asset things also
		TextureAssetData testGuiAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/tiles.png");
		scene->guiPanelImage 		= graphics_memory_push_gui_texture(platformGraphics, &testGuiAsset);
		scene->guiPanelColour 		= colour_rgb_alpha(colour_aqua_blue.rgb, 0.5);
	}


	// Skysphere
	scene->skybox = graphics_memory_push_model(	platformGraphics,
												assets_get_mesh(scene->assets, MESH_ASSET_SKYSPHERE),
												assets_get_material(scene->assets, MATERIAL_ASSET_SKY));

	// Characters
	{
		scene->playerCarryState 		= CARRY_NONE;
		scene->playerCharacterTransform = {.position = {10, 0, 5}};

		auto & motor 	= scene->playerCharacterMotor;
		motor.transform = &scene->playerCharacterTransform;

		{
			using namespace CharacterAnimations;			

			motor.animations[WALK] 		= assets_get_animation(scene->assets, AAID_CHARACTER_WALK);
			motor.animations[RUN] 		= assets_get_animation(scene->assets, AAID_CHARACTER_RUN);
			motor.animations[IDLE] 		= assets_get_animation(scene->assets, AAID_CHARACTER_IDLE);
			motor.animations[JUMP]		= assets_get_animation(scene->assets, AAID_CHARACTER_JUMP);
			motor.animations[FALL]		= assets_get_animation(scene->assets, AAID_CHARACTER_FALL);
			motor.animations[CROUCH] 	= assets_get_animation(scene->assets, AAID_CHARACTER_CROUCH);
		}

		scene->playerSkeletonAnimator = 
		{
			.skeleton 		= assets_get_skeleton(scene->assets, SAID_CHARACTER),
			.animations 	= motor.animations,
			.weights 		= motor.animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		};

		scene->playerSkeletonAnimator.boneBoneSpaceTransforms = push_array_2<Transform3D>(	persistentMemory,
																							scene->playerSkeletonAnimator.skeleton->bones.count,
																							ALLOC_CLEAR);
		array_2_fill_with_value(scene->playerSkeletonAnimator.boneBoneSpaceTransforms, identity_transform);

		auto model = graphics_memory_push_model(platformGraphics,
												assets_get_mesh(scene->assets, MESH_ASSET_CHARACTER),
												assets_get_material(scene->assets, MATERIAL_ASSET_CHARACTER));
		scene->playerAnimatedRenderer = make_animated_renderer(&scene->playerCharacterTransform, scene->playerSkeletonAnimator.skeleton, model);

	}

	// NOBLE PERSON GUY
	{
		v3 position = {random_range(-99, 99), random_range(-99, 99), 0};
		v3 scale 	= make_uniform_v3(random_range(0.8f, 1.5f));

		scene->noblePersonTransform.position 	= position;
		scene->noblePersonTransform.scale 		= scale;

		scene->noblePersonCharacterMotor = {};
		scene->noblePersonCharacterMotor.transform = &scene->noblePersonTransform;

		{
			using namespace CharacterAnimations;
			
			scene->noblePersonCharacterMotor.animations[WALK] 	= assets_get_animation(scene->assets, AAID_CHARACTER_WALK);
			scene->noblePersonCharacterMotor.animations[RUN] 	= assets_get_animation(scene->assets, AAID_CHARACTER_RUN);
			scene->noblePersonCharacterMotor.animations[IDLE]	= assets_get_animation(scene->assets, AAID_CHARACTER_IDLE);
			scene->noblePersonCharacterMotor.animations[JUMP] 	= assets_get_animation(scene->assets, AAID_CHARACTER_JUMP);
			scene->noblePersonCharacterMotor.animations[FALL] 	= assets_get_animation(scene->assets, AAID_CHARACTER_FALL);
			scene->noblePersonCharacterMotor.animations[CROUCH] = assets_get_animation(scene->assets, AAID_CHARACTER_CROUCH);
		}

		scene->noblePersonSkeletonAnimator = 
		{
			.skeleton 		= assets_get_skeleton(scene->assets, SAID_CHARACTER),
			.animations 	= scene->noblePersonCharacterMotor.animations,
			.weights 		= scene->noblePersonCharacterMotor.animationWeights,
			.animationCount = CharacterAnimations::ANIMATION_COUNT
		};
		scene->noblePersonSkeletonAnimator.boneBoneSpaceTransforms = push_array_2<Transform3D>(	persistentMemory,
																								scene->noblePersonSkeletonAnimator.skeleton->bones.count,
																								ALLOC_CLEAR);
		array_2_fill_with_value(scene->noblePersonSkeletonAnimator.boneBoneSpaceTransforms, identity_transform);

		auto model = graphics_memory_push_model(platformGraphics,
												assets_get_mesh(scene->assets, MESH_ASSET_CHARACTER), 
												assets_get_material(scene->assets, MATERIAL_ASSET_CHARACTER)); 

		scene->noblePersonAnimatedRenderer = make_animated_renderer(&scene->noblePersonTransform, scene->noblePersonSkeletonAnimator.skeleton, model);

	}

	scene->worldCamera 				= make_camera(60, 0.1f, 1000.0f);
	scene->playerCamera 			= {};
	scene->playerCamera.baseOffset 	= {0, 0, 2};
	scene->playerCamera.distance 	= 5;

	// Environment
	{
		/// GROUND AND TERRAIN
		{
			constexpr f32 mapSize 		= 1200;
			constexpr f32 minTerrainElevation = -5;
			constexpr f32 maxTerrainElevation = 50;

			// Note(Leo): this is maximum size we support with u16 mesh vertex indices
			s32 chunkResolution			= 128;
			
			s32 chunkCountPerDirection 	= 10;
			f32 chunkSize 				= 1.0f / chunkCountPerDirection;
			f32 chunkWorldSize 			= chunkSize * mapSize;

			push_memory_checkpoint(*global_transientMemory);
			s32 heightmapResolution = 1024;

			// todo(Leo): put to asset system thing
			auto heightmapTexture 	= load_texture_asset(*global_transientMemory, "assets/textures/heightmap_island.png");

			// auto heightmapTexture	= assets_get_texture_data(scene->assets, *global_transientMemory, TEXTURE_ASSET_ISLAND_HEIGHTMAP);
			auto heightmap 			= make_heightmap(&persistentMemory, &heightmapTexture, heightmapResolution, mapSize, minTerrainElevation, maxTerrainElevation);

			pop_memory_checkpoint(*global_transientMemory);

			s32 terrainCount 			= chunkCountPerDirection * chunkCountPerDirection;
			scene->terrainCount 		= terrainCount;
			scene->terrainTransforms 	= push_memory<m44>(persistentMemory, terrainCount, ALLOC_NO_CLEAR);
			scene->terrainMeshes 		= push_memory<MeshHandle>(persistentMemory, terrainCount, ALLOC_NO_CLEAR);
			scene->terrainMaterial 		= assets_get_material(scene->assets, MATERIAL_ASSET_GROUND);

			/// GENERATE GROUND MESH
			{
				push_memory_checkpoint(*global_transientMemory);
			
				for (s32 i = 0; i < terrainCount; ++i)
				{
					s32 x = i % chunkCountPerDirection;
					s32 y = i / chunkCountPerDirection;

					v2 position = { x * chunkSize, y * chunkSize };
					v2 size 	= { chunkSize, chunkSize };

					auto groundMeshAsset 	= generate_terrain(*global_transientMemory, heightmap, position, size, chunkResolution, 20);
					scene->terrainMeshes[i] = graphics_memory_push_mesh(platformGraphics, &groundMeshAsset);
				}
			
				pop_memory_checkpoint(*global_transientMemory);
			}

			f32 halfMapSize = mapSize / 2;
			v3 terrainOrigin = {-halfMapSize, -halfMapSize, 0};

			for (s32 i = 0; i < terrainCount; ++i)
			{
				s32 x = i % chunkCountPerDirection;
				s32 y = i / chunkCountPerDirection;

				v3 leftBackCornerPosition = {x * chunkWorldSize - halfMapSize, y * chunkWorldSize - halfMapSize, 0};
				scene->terrainTransforms[i] = translation_matrix(leftBackCornerPosition);
			}

			auto transform = scene->transforms.push_return_pointer({{-mapSize / 2, -mapSize / 2, 0}});

			// Todo(Leo): use better(dumber but smarter) thing to get rid of std move
			scene->collisionSystem.terrainCollider 	= std::move(heightmap);
			scene->collisionSystem.terrainTransform = transform;

			MeshAssetData seaMeshAsset = {};
			{
				Vertex vertices []
				{
					{-0.5, -0.5, 0, 0, 0, 1, 1,1,1, 0, 0},
					{ 0.5, -0.5, 0, 0, 0, 1, 1,1,1, 1, 0},
					{-0.5,  0.5, 0, 0, 0, 1, 1,1,1, 0, 1},
					{ 0.5,  0.5, 0, 0, 0, 1, 1,1,1, 1, 1},
				};
				seaMeshAsset.vertices = push_memory<Vertex>(*global_transientMemory, array_count(vertices), 0); 
				copy_memory(seaMeshAsset.vertices, vertices, sizeof(Vertex) * array_count(vertices));

				u16 indices [] 			= {0,1,2,2,1,3};
				seaMeshAsset.indexCount = array_count(indices);
				seaMeshAsset.indices 	= push_and_copy_memory(*global_transientMemory, array_count(indices), indices, 0);

				seaMeshAsset.indexType = IndexType::UInt16;
			}
			mesh_generate_tangents(seaMeshAsset);
			scene->seaMesh 		= graphics_memory_push_mesh(platformGraphics, &seaMeshAsset);
			scene->seaMaterial 	= assets_get_material(scene->assets, MATERIAL_ASSET_SEA);
			scene->seaTransform = transform_matrix({0,0,0}, identity_quaternion, {mapSize, mapSize, 1});
		}

		/// TOTEMS
		{
			auto totemMesh 	= assets_get_mesh(scene->assets, MESH_ASSET_TOTEM);
			auto model 		= graphics_memory_push_model(platformGraphics, totemMesh, assets_get_material(scene->assets, MATERIAL_ASSET_ENVIRONMENT));

			Transform3D * transform = scene->transforms.push_return_pointer({});
			transform->position.z 	= get_terrain_height(scene->collisionSystem, transform->position.xy) - 0.5f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3 {1.0, 1.0, 5.0},
								v3 {0,0,2},
								transform);

			transform = scene->transforms.push_return_pointer({.position = {0, 5, 0} , .scale =  {0.5, 0.5, 0.5}});
			transform->position.z = get_terrain_height(scene->collisionSystem, transform->position.xy) - 0.25f;
			scene->renderers.push({transform, model});
			push_box_collider(	scene->collisionSystem,
								v3{0.5, 0.5, 2.5},
								v3{0,0,1},
								transform);
		}

		/// RACCOONS
		{
			scene->raccoonCount = 4;
			push_multiple_memories(	persistentMemory,
									scene->raccoonCount,
									ALLOC_CLEAR,
									
									&scene->raccoonModes,
									&scene->raccoonTransforms,
									&scene->raccoonTargetPositions,
									&scene->raccoonCharacterMotors);

			for (s32 i = 0; i < scene->raccoonCount; ++i)
			{
				scene->raccoonModes[i]					= RACCOON_IDLE;

				scene->raccoonTransforms[i] 			= identity_transform;
				scene->raccoonTransforms[i].position 	= random_inside_unit_square() * 100 - v3{50, 50, 0};
				scene->raccoonTransforms[i].position.z  = get_terrain_height(scene->collisionSystem, scene->raccoonTransforms[i].position.xy);

				scene->raccoonTargetPositions[i] 		= random_inside_unit_square() * 100 - v3{50,50,0};
				scene->raccoonTargetPositions[i].z  	= get_terrain_height(scene->collisionSystem, scene->raccoonTargetPositions[i].xy);

				// ------------------------------------------------------------------------------------------------
		
				scene->raccoonCharacterMotors[i] = {};
				scene->raccoonCharacterMotors[i].transform = &scene->raccoonTransforms[i];

				{
					using namespace CharacterAnimations;

					scene->raccoonCharacterMotors[i].animations[WALK] 		= assets_get_animation(scene->assets, AAID_RACCOON_EMPTY);
					scene->raccoonCharacterMotors[i].animations[RUN] 		= assets_get_animation(scene->assets, AAID_RACCOON_EMPTY);
					scene->raccoonCharacterMotors[i].animations[IDLE]		= assets_get_animation(scene->assets, AAID_RACCOON_EMPTY);
					scene->raccoonCharacterMotors[i].animations[JUMP] 		= assets_get_animation(scene->assets, AAID_RACCOON_EMPTY);
					scene->raccoonCharacterMotors[i].animations[FALL] 		= assets_get_animation(scene->assets, AAID_RACCOON_EMPTY);
					scene->raccoonCharacterMotors[i].animations[CROUCH] 	= assets_get_animation(scene->assets, AAID_RACCOON_EMPTY);
				}
			}

			scene->raccoonMesh 		= assets_get_mesh(scene->assets, MESH_ASSET_RACCOON);
			scene->raccoonMaterial	= assets_get_material(scene->assets, MATERIAL_ASSET_RACCOON);
		}

		/// INVISIBLE TEST COLLIDER
		{
			Transform3D * transform = scene->transforms.push_return_pointer({});
			transform->position = {21, 15};
			transform->position.z = get_terrain_height(scene->collisionSystem, {20, 20});

			push_box_collider( 	scene->collisionSystem,
								v3{2.0f, 0.05f,5.0f},
								v3{0,0, 2.0f},
								transform);
		}

		scene->monuments = scene3d_load_monuments(persistentMemory, assets_get_material(scene->assets, MATERIAL_ASSET_ENVIRONMENT), scene->collisionSystem);

		// TEST ROBOT
		{
			v3 position 		= {21, 10, 0};
			position.z 			= get_terrain_height(scene->collisionSystem, position.xy);
			auto * transform 	= scene->transforms.push_return_pointer({.position = position});

			auto mesh 		= assets_get_mesh(scene->assets, MESH_ASSET_ROBOT);
			auto material 	= assets_get_material(scene->assets, MATERIAL_ASSET_ROBOT);
			auto model 		= graphics_memory_push_model(platformGraphics, mesh, material);

			scene->renderers.push({transform, model});
		}

		/// SMALL SCENERY OBJECTS
		{			
			scene->potMesh 		= assets_get_mesh(scene->assets, MESH_ASSET_SMALL_POT);
			scene->potMaterial 	= assets_get_material(scene->assets, MATERIAL_ASSET_ENVIRONMENT);

			{
				scene->potCapacity 			= 10;
				scene->potCount 		= scene->potCapacity;
				scene->potTransforms 	= push_memory<Transform3D>(persistentMemory, scene->potCapacity, ALLOC_NO_CLEAR);
				scene->potWaterLevels 	= push_memory<f32>(persistentMemory, scene->potCapacity, 0);

				for(s32 i = 0; i < scene->potCapacity; ++i)
				{
					v3 position 			= {15, i * 5.0f, 0};
					position.z 				= get_terrain_height(scene->collisionSystem, position.xy);
					scene->potTransforms[i]	= { .position = position };
				}
			}

			// ----------------------------------------------------------------	

			auto bigPotMesh = assets_get_mesh(scene->assets, MESH_ASSET_BIG_POT);

			s32 bigPotCount = 5;
			for (s32 i = 0; i < bigPotCount; ++i)
			{
				ModelHandle model 		= graphics_memory_push_model(platformGraphics, bigPotMesh, assets_get_material(scene->assets, MATERIAL_ASSET_ENVIRONMENT));
				Transform3D * transform = scene->transforms.push_return_pointer({13, 2.0f + i * 4.0f, 0});

				transform->position.z 	= get_terrain_height(scene->collisionSystem, transform->position.xy);

				scene->renderers.push({transform, model});
				f32 colliderHeight = 1.16;
				push_cylinder_collider(scene->collisionSystem, 0.6, colliderHeight, v3{0,0,colliderHeight / 2}, transform);
			}


			// ----------------------------------------------------------------	

			/// WATERS
			{
				scene->waterMesh 		= assets_get_mesh(scene->assets, MESH_ASSET_WATER_DROP);
				scene->waterMaterial 	= assets_get_material(scene->assets, MATERIAL_ASSET_WATER);

				{				
					scene->waters.capacity 		= 200;
					scene->waters.count 		= 20;
					scene->waters.transforms 	= push_memory<Transform3D>(persistentMemory, scene->waters.capacity, 0);
					scene->waters.levels 		= push_memory<f32>(persistentMemory, scene->waters.capacity, 0);

					for (s32 i = 0; i < scene->waters.count; ++i)
					{
						v3 position = {random_range(-50, 50), random_range(-50, 50)};
						position.z 	= get_terrain_height(scene->collisionSystem, position.xy);

						scene->waters.transforms[i] 	= { .position = position };
						scene->waters.levels[i] 		= scene->fullWaterLevel;
					}
				}
			}


		}

		/// TRAIN
		{
			scene->trainMesh 		= assets_get_mesh(scene->assets, MESH_ASSET_TRAIN);
			scene->trainMaterial 	= assets_get_material(scene->assets, MATERIAL_ASSET_ENVIRONMENT);

			// ----------------------------------------------------------------------------------

			scene->trainStopPosition 	= {50, 0, 0};
			scene->trainStopPosition.z 	= get_terrain_height(scene->collisionSystem, scene->trainStopPosition.xy);
			scene->trainStopPosition.z 	= max_f32(0, scene->trainStopPosition.z);

			f32 trainFarDistance 		= 2000;

			scene->trainFarPositionA 	= {50, trainFarDistance, 0};
			scene->trainFarPositionA.z 	= get_terrain_height(scene->collisionSystem, scene->trainFarPositionA.xy);
			scene->trainFarPositionA.z 	= max_f32(0, scene->trainFarPositionA.z);

			scene->trainFarPositionB 	= {50, -trainFarDistance, 0};
			scene->trainFarPositionB.z 	= get_terrain_height(scene->collisionSystem, scene->trainFarPositionB.xy);
			scene->trainFarPositionB.z 	= max_f32(0, scene->trainFarPositionB.z);

			scene->trainTransform.position 			= scene->trainStopPosition;

			scene->trainMoveState 					= TRAIN_WAIT;

			scene->trainFullSpeed 					= 200;
			scene->trainStationMinSpeed 			= 1;
			scene->trainAcceleration 				= 20;
			scene->trainWaitTimeOnStop 				= 5;

			/*
			v = v0 + a*t
			-> t = (v - v0) / a
			d = d0 + v0*t + 1/2*a*t^2
			d - d0 = v0*t + 1/2*a*t^2
			*/

			f32 timeToBrakeBeforeStation 			= (scene->trainFullSpeed - scene->trainStationMinSpeed) / scene->trainAcceleration;
			scene->trainBrakeBeforeStationDistance 	= scene->trainFullSpeed * timeToBrakeBeforeStation
													// Note(Leo): we brake, so acceleration term is negative
													- 0.5f * scene->trainAcceleration * timeToBrakeBeforeStation * timeToBrakeBeforeStation;

			log_debug(0, "brake time: ", timeToBrakeBeforeStation, ", distance: ", scene->trainBrakeBeforeStationDistance);

			scene->trainCurrentWaitTime = 0;
			scene->trainCurrentSpeed 	= 0;

		}
	}

	// ----------------------------------------------------------------------------------

	/// MARCHING CUBES AND METABALLS TESTING
	{
		v3 position = {-10, -30, 0};
		position.z = get_terrain_height(scene->collisionSystem, position.xy) + 3;

		s32 vertexCountPerCube 	= 14;
		s32 indexCountPerCube 	= 36;
		s32 cubeCapacity 		= 100000;
		s32 vertexCapacity 		= vertexCountPerCube * cubeCapacity;
		s32 indexCapacity 		= indexCountPerCube * cubeCapacity;

		Vertex * vertices 	= push_memory<Vertex>(persistentMemory, vertexCapacity, ALLOC_NO_CLEAR);
		u16 * indices 		= push_memory<u16>(persistentMemory, indexCapacity, ALLOC_NO_CLEAR);

		u32 vertexCount;
		u32 indexCount;

		scene->metaballGridScale = 0.3;

		scene->metaballVertexCount 	= vertexCount;
		scene->metaballVertices 	= vertices;

		scene->metaballIndexCount 	= indexCount;
		scene->metaballIndices 		= indices;

		scene->metaballVertexCapacity 	= vertexCapacity;
		scene->metaballIndexCapacity 	= indexCapacity;

		scene->metaballTransform 	= translation_matrix(position);

		scene->metaballMaterial = assets_get_material(scene->assets, MATERIAL_ASSET_TREE);

		// ----------------------------------------------------------------------------------

		scene->metaballVertexCapacity2 	= vertexCapacity;
		scene->metaballVertexCount2 	= 0;
		scene->metaballVertices2 		= push_memory<Vertex>(persistentMemory, vertexCapacity, ALLOC_NO_CLEAR);

		scene->metaballIndexCapacity2 	= vertexCapacity;
		scene->metaballIndexCount2 		= 0;
		scene->metaballIndices2 		= push_memory<u16>(persistentMemory, indexCapacity, ALLOC_NO_CLEAR);

		v3 position2 				= {0, -30, 0};
		position2.z 				= get_terrain_height(scene->collisionSystem, position2.xy) + 3;
		scene->metaballTransform2 	= translation_matrix(position2);

	}

	// ----------------------------------------------------------------------------------
	
	{	
		s32 treeCapacity 	= 50;	
		scene->trees 		= push_array_2<Tree3>(persistentMemory, treeCapacity, ALLOC_CLEAR);
		scene->trees.count 	= scene->trees.capacity;

		scene->treeIndex 	= 0;

		s32 halfCapacity 	= scene->trees.capacity / 2;
		for (auto & tree : experimental::array_2_range(scene->trees, 0, halfCapacity))
		{
			v3 position = random_on_unit_circle_xy() * random_value() * 50;
			position.z = get_terrain_height(scene->collisionSystem, position.xy);

			initialize_test_tree_3(persistentMemory, tree, position, &scene->treeSettings[0]);
			build_tree_3_mesh(tree);

			tree.leaves.material 	= assets_get_material(scene->assets, MATERIAL_ASSET_LEAVES);
			tree.leaves.colourIndex = tree.settings->leafColourIndex;
			tree.seedMesh 			= assets_get_mesh(scene->assets, MESH_ASSET_SEED);
			tree.seedMaterial 		= assets_get_material(scene->assets, MATERIAL_ASSET_SEED);
		}

		// ----------------------------------------------------------------------------------

		for (auto & tree : experimental::array_2_range(scene->trees, halfCapacity, scene->trees.capacity))
		{
			v3 position = random_on_unit_circle_xy() * random_value() * 50;
			position.z = get_terrain_height(scene->collisionSystem, position.xy);
			
			initialize_test_tree_3(persistentMemory, tree, position, &scene->treeSettings[1]);
			build_tree_3_mesh(tree);

			tree.leaves.material 	= assets_get_material(scene->assets, MATERIAL_ASSET_LEAVES);
			tree.leaves.colourIndex = tree.settings->leafColourIndex;
			tree.seedMesh 			= assets_get_mesh(scene->assets, MESH_ASSET_SEED);
			tree.seedMaterial 		= assets_get_material(scene->assets, MATERIAL_ASSET_SEED);
		}
	}

	// ----------------------------------------------------------------------------------

	log_application(0, "Scene3d loaded, ", used_percent(*global_transientMemory) * 100, "% of transient memory used.");
	log_application(0, "Scene3d loaded, ", used_percent(persistentMemory) * 100, "% of persistent memory used.");

	return scene;
}
