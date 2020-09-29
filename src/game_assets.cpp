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

	MESH_ASSET_MONUMENT_ARCS,
	MESH_ASSET_MONUMENT_BASE,
	MESH_ASSET_MONUMENT_TOP_1,
	MESH_ASSET_MONUMENT_TOP_2,

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

	assets.meshAssetLoadInfo[MESH_ASSET_MONUMENT_ARCS] 	= {"assets/models/monuments.glb", "monument_arches"};
	assets.meshAssetLoadInfo[MESH_ASSET_MONUMENT_BASE] 	= {"assets/models/monuments.glb", "monument_base"};
	assets.meshAssetLoadInfo[MESH_ASSET_MONUMENT_TOP_1] = {"assets/models/monuments.glb", "monument_ornament_01"};
	assets.meshAssetLoadInfo[MESH_ASSET_MONUMENT_TOP_2] = {"assets/models/monuments.glb", "monument_ornament_02"};



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
		animation 		= push_memory<Animation>(*assets.allocator, 1, ALLOC_ZERO_MEMORY);

		if (loadInfo.filename && loadInfo.gltfNodeName)
		{
			auto file 	= read_gltf_file(*global_transientMemory, loadInfo.filename);
			*animation 	= load_animation_glb(*assets.allocator, file, loadInfo.gltfNodeName);
		}
		else
		{
			log_debug(FILE_ADDRESS, "Loading empty animation");
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

		skeleton 		= push_memory<AnimatedSkeleton>(*assets.allocator, 1, ALLOC_ZERO_MEMORY);
		auto file 		= read_gltf_file(*global_transientMemory, loadInfo.filename);
		*skeleton 		= load_skeleton_glb(*assets.allocator, file, loadInfo.gltfNodeName);
		
		assets.skeletons[id] = skeleton;
	}	

	return skeleton;
}