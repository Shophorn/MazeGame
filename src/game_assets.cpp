/*
Leo Tamminen

Game assets functions actually used in game
*/

#include "game_assets.h"
#include "fs_asset_file_format.cpp"

struct TextureLoadInfo
{
	TextureFormat 		format;
	TextureAddressMode 	addressMode;
	
	bool	generateAssetData;
	v4 		generatedTextureColour;	
};

struct MaterialLoadInfo
{
	static constexpr int max_texture_count = 5;

	GraphicsPipeline 	pipeline;
	TextureAssetId 		textures[max_texture_count];
};

struct GameAssets
{
	// GPU MEMORY
	MeshHandle 			meshes [MeshAssetIdCount];

	TextureHandle 		textures [TextureAssetIdCount];
	TextureLoadInfo 	textureLoadInfos [TextureAssetIdCount];	

	MaterialHandle 		materials [MaterialAssetIdCount];
	MaterialLoadInfo 	materialLoadInfos [MaterialAssetIdCount];

	// CPU MEMORY
	MemoryArena * allocator;

	Animation *			animations [AnimationAssetIdCount];
	AnimatedSkeleton * 	skeletons [SkeletonAssetIdCount];
	AudioAsset * 		audio [SoundAssetIdCount];
	Font * 				fonts [FontAssetIdCount];

	PlatformFileHandle 	file;
	// AssetFileHeader 	fileHeader;

	AssetFileTexture 	textureHeaders[TextureAssetIdCount];
	AssetFileMesh		meshHeaders[MeshAssetIdCount];
	AssetFileAnimation	animationHeaders[AnimationAssetIdCount];
	AssetFileSkeleton	skeletonHeaders[SkeletonAssetIdCount];
	AssetFileSound		soundHeaders[SoundAssetIdCount];
	AssetFileFont		fontHeaders[FontAssetIdCount];

	PlatformFileHandle 	meshFile;
	PlatformFileHandle 	meshFileHeader;
};

internal GameAssets init_game_assets(MemoryArena * allocator)
{
	// Note(Leo): apparently this does indeed set MeshHandles to -1, which is what we want. I do not know why, though.
	GameAssets assets 	= {};
	assets.file 		= platform_file_open("assets.fsa", FileMode_read);
	// platform_file_read(assets.file, 0, sizeof(AssetFileHeader), &assets.fileHeader);



	AssetFileHeader2 header2 = {};
	platform_file_read(assets.file, 0, sizeof(AssetFileHeader2), &header2);
	Assert(header2.magic == asset_file_magic_value);
	Assert(header2.version == asset_file_current_version);

	Assert(header2.textures.count == TextureAssetIdCount && "Invalid number of textures in asset file");
	Assert(header2.meshes.count == MeshAssetIdCount && "Invalid number of meshes in asset file");
	Assert(header2.animations.count == AnimationAssetIdCount && "Invalid number of animations in asset file");
	Assert(header2.skeletons.count == SkeletonAssetIdCount && "Invalid number of skeletons in asset file");
	Assert(header2.sounds.count == SoundAssetIdCount && "Invalid number of sounds in asset file");
	Assert(header2.fonts.count == FontAssetIdCount && "Invalid number of fonts in asset file");

	platform_file_read(assets.file, header2.textures.offsetToFirst, sizeof(assets.textureHeaders), assets.textureHeaders);
	platform_file_read(assets.file, header2.meshes.offsetToFirst, sizeof(assets.meshHeaders), assets.meshHeaders);
	platform_file_read(assets.file, header2.animations.offsetToFirst, sizeof(assets.animationHeaders), assets.animationHeaders);
	platform_file_read(assets.file, header2.skeletons.offsetToFirst, sizeof(assets.skeletonHeaders), assets.skeletonHeaders);
	platform_file_read(assets.file, header2.sounds.offsetToFirst, sizeof(assets.soundHeaders), assets.soundHeaders);
	platform_file_read(assets.file, header2.fonts.offsetToFirst, sizeof(assets.fontHeaders), assets.fontHeaders);


	assets.allocator 		= allocator;

	// auto load_from_file = [](char const * filename, TextureFormat format = {}, TextureAddressMode addressMode = {}) -> TextureLoadInfo
	// {
	// 	TextureLoadInfo result 		= {};
	// 	result.generateAssetData 	= false;
	// 	// result.filename 			= filename;
	// 	result.format 				= format;
	// 	result.addressMode 			= addressMode;
	// 	return result;
	// };

	auto load_generated = [](v4 colour, TextureFormat format = {}, TextureAddressMode addressMode = {}) -> TextureLoadInfo
	{
		TextureLoadInfo result 			= {};
		result.generateAssetData 		= true;
		result.generatedTextureColour 	= colour;
		result.format 					= format;
		result.addressMode 				= addressMode;
		return result;
	};

	// assets.textureLoadInfos[TextureAssetId_ground_albedo] 	= load_from_file("assets/textures/ground.png");
	// assets.textureLoadInfos[TextureAssetId_tiles_albedo] 	= load_from_file("assets/textures/tiles.png");
	// assets.textureLoadInfos[TextureAssetId_red_tiles_albedo] = load_from_file("assets/textures/tiles_red.png");
	// assets.textureLoadInfos[TextureAssetId_seed_albedo] 		= load_from_file("assets/textures/Acorn_albedo.png");
	// assets.textureLoadInfos[TextureAssetId_bark_albedo]		= load_from_file("assets/textures/bark.png");
	// assets.textureLoadInfos[TextureAssetId_raccoon_albedo]	= load_from_file("assets/textures/RaccoonAlbedo.png");
	// assets.textureLoadInfos[TextureAssetId_robot_albedo] 	= load_from_file("assets/textures/Robot_53_albedo_4k.png");
	
	// assets.textureLoadInfos[TextureAssetId_leaves_mask]			= load_from_file("assets/textures/leaf_mask.png");

	// assets.textureLoadInfos[TextureAssetId_ground_normal] 	= load_from_file("assets/textures/ground_normal.png", TextureFormat_u8_linear);
	// assets.textureLoadInfos[TextureAssetId_tiles_normal] 	= load_from_file("assets/textures/tiles_normal.png", TextureFormat_u8_linear);
	// assets.textureLoadInfos[TextureAssetId_bark_normal]		= load_from_file("assets/textures/bark_normal.png", TextureFormat_u8_linear);
	// assets.textureLoadInfos[TextureAssetId_robot_normal] 	= load_from_file("assets/textures/Robot_53_normal_4k.png");

	assets.textureLoadInfos[TextureAssetId_white] 		= load_generated(colour_white, TextureFormat_u8_srgb);
	assets.textureLoadInfos[TextureAssetId_black] 		= load_generated(colour_black, TextureFormat_u8_srgb);
	assets.textureLoadInfos[TextureAssetId_flat_normal] = load_generated(colour_bump, TextureFormat_u8_linear);
	assets.textureLoadInfos[TextureAssetId_water_blue] 	= load_generated(colour_rgb_alpha(colour_aqua_blue.rgb, 0.4), TextureFormat_u8_srgb);
 
	// ----------- MATERIALS -------------------

	assets.materialLoadInfos[MaterialAssetId_ground] 			= {GraphicsPipeline_normal, TextureAssetId_ground_albedo, TextureAssetId_ground_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_environment] 		= {GraphicsPipeline_normal, TextureAssetId_tiles_albedo, TextureAssetId_tiles_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_seed] 				= {GraphicsPipeline_normal, TextureAssetId_seed_albedo, TextureAssetId_flat_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_raccoon] 			= {GraphicsPipeline_normal, TextureAssetId_raccoon_albedo, TextureAssetId_flat_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_robot]				= {GraphicsPipeline_normal, TextureAssetId_robot_albedo, TextureAssetId_robot_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_box]				= {GraphicsPipeline_normal, TextureAssetId_bark_albedo, TextureAssetId_bark_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_clouds] 			= {GraphicsPipeline_normal, TextureAssetId_white, TextureAssetId_flat_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_character] 		= {GraphicsPipeline_animated, TextureAssetId_red_tiles_albedo, TextureAssetId_tiles_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_water] 			= {GraphicsPipeline_water, TextureAssetId_water_blue, TextureAssetId_flat_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_sea] 				= {GraphicsPipeline_water, TextureAssetId_water_blue, TextureAssetId_flat_normal, TextureAssetId_white};
	assets.materialLoadInfos[MaterialAssetId_leaves] 			= {GraphicsPipeline_leaves, TextureAssetId_leaves_mask};
	assets.materialLoadInfos[MaterialAssetId_sky] 				= {GraphicsPipeline_skybox, TextureAssetId_black, TextureAssetId_black};
	assets.materialLoadInfos[MaterialAssetId_tree]				= {GraphicsPipeline_triplanar, TextureAssetId_tiles_albedo};
	assets.materialLoadInfos[MaterialAssetId_building_block]	= {GraphicsPipeline_normal, TextureAssetId_tiles_2_albedo, TextureAssetId_tiles_2_normal, TextureAssetId_tiles_2_ao};

	assets.materialLoadInfos[MaterialAssetId_menu_background] 	= {GraphicsPipeline_screen_gui, TextureAssetId_menu_background};

	return assets;
}

internal MeshHandle assets_get_mesh(GameAssets & assets, MeshAssetId id)
{
	MeshHandle handle = assets.meshes[id];
	
	if (is_valid_handle(handle) == false)
	{
		AssetFileMesh assetMesh = assets.meshHeaders[id];//fileHeader.meshes[id];

		u64 vertexMemorySize 	= assetMesh.vertexCount * sizeof(Vertex);
		u64 skinMemorySize 		= assetMesh.hasSkinning ? assetMesh.vertexCount * sizeof(VertexSkinData) : 0;
		u64 indexMemorySize 	= assetMesh.indexCount * sizeof(u16);
		u64 totalMemorySize 	= vertexMemorySize + skinMemorySize + indexMemorySize;

		MeshAssetData data;
		u8 * dataMemory = push_memory<u8>(*global_transientMemory, totalMemorySize, ALLOC_GARBAGE);

		platform_file_read(assets.file, assetMesh.dataOffset, totalMemorySize, dataMemory);


		data.vertices 	= reinterpret_cast<Vertex*>(dataMemory);
		data.skinning 	= assetMesh.hasSkinning ? reinterpret_cast<VertexSkinData*>(dataMemory + vertexMemorySize) : nullptr;
		data.indices 	= reinterpret_cast<u16*>(dataMemory + vertexMemorySize + skinMemorySize);

		data.vertexCount = assetMesh.vertexCount;
		data.indexCount = assetMesh.indexCount;

		// Todo(Leo): this is sad, somehow tangents dont come all the way to here
		mesh_generate_tangents(data);

		assets.meshes[id] 	= graphics_memory_push_mesh(platformGraphics, &data);
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
			AssetFileTexture assetTexture = assets.textureHeaders[id];

			u64 memorySize = assetTexture.width * assetTexture.height * assetTexture.channels;

			assetData = {};
			assetData.pixelMemory = push_memory<u8>(*global_transientMemory, memorySize, ALLOC_GARBAGE);
			platform_file_read(assets.file, assetTexture.dataOffset, memorySize, assetData.pixelMemory);

			assetData.width 		= assetTexture.width;
			assetData.height 		= assetTexture.height;
			assetData.channels 		= assetTexture.channels;
			assetData.format 		= assetTexture.format;
			assetData.addressMode 	= assetTexture.addressMode;

			Assert(assetData.channels == 4);
	
			assets.textures[id] 	= graphics_memory_push_texture(platformGraphics, &assetData);
			handle 					= assets.textures[id];
		}
		else
		{
			// Note(Leo): we use pointer to this u32 to define colour context, but it shouldnt be referenced after the scope ends
			u32 textureColour 		= colour_rgba_u32(loadInfo.generatedTextureColour);
			assetData 				= make_texture_asset(&textureColour, 1, 1, 4);
			assetData.format 		= loadInfo.format;
			assetData.addressMode 	= loadInfo.addressMode;
			assets.textures[id] 	= graphics_memory_push_texture(platformGraphics, &assetData);
			handle 					= assets.textures[id];
		}

	}

	return handle;
}

internal TextureAssetData assets_get_texture_data(GameAssets & assets, TextureAssetId id)
{
	TextureLoadInfo & loadInfo = assets.textureLoadInfos[id];

	TextureAssetData assetData;
	if (loadInfo.generateAssetData == false)
	{
		AssetFileTexture assetTexture = assets.textureHeaders[id]; //fileHeader.textures[id];

		u64 memorySize = assetTexture.width * assetTexture.height * assetTexture.channels;

		assetData = {};
		assetData.pixelMemory = push_memory<u8>(*global_transientMemory, memorySize, ALLOC_GARBAGE);
		platform_file_read(assets.file, assetTexture.dataOffset, memorySize, assetData.pixelMemory);

		assetData.width 	= assetTexture.width;
		assetData.height 	= assetTexture.height;
		assetData.channels 	= assetTexture.channels;

		Assert(assetData.channels == 4);
	}
	else
	{
		u32 textureColour 	= colour_rgba_u32(loadInfo.generatedTextureColour);
		assetData 			= make_texture_asset(&textureColour, 1, 1, 4);
	}

	assetData.format 		= loadInfo.format;
	assetData.addressMode 	= loadInfo.addressMode;

	return assetData;
}


internal MaterialHandle assets_get_material(GameAssets & assets, MaterialAssetId id)
{
	MaterialHandle handle = assets.materials[id];

	if (is_valid_handle(handle) == false)
	{
		MaterialLoadInfo & loadInfo = assets.materialLoadInfos[id];

		s32 textureCount;
		switch (loadInfo.pipeline)
		{
			case GraphicsPipeline_leaves:
			case GraphicsPipeline_triplanar:
			case GraphicsPipeline_screen_gui:
				textureCount = 1;
				break;

			case GraphicsPipeline_skybox:
				textureCount = 2;
				break;

			case GraphicsPipeline_normal:
			case GraphicsPipeline_animated:
			case GraphicsPipeline_water:
				textureCount = 3;
				break;

			default:
				Assert(false && "Invalid choice");
				break;
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
		if (id == AnimationAssetId_raccoon_empty)
		{
			animation 	= push_memory<Animation>(*assets.allocator, 1, ALLOC_ZERO_MEMORY);
			*animation 	= {};
		}
		else
		{
			AssetFileAnimation asset = assets.animationHeaders[id];//fileHeader.animations[id];

			animation = push_memory<Animation>(*assets.allocator, 1, ALLOC_ZERO_MEMORY);

			animation->channelCount = asset.channelCount;
			animation->duration 	= asset.duration;


			s64 translationInfoSize 	= asset.channelCount * sizeof(AnimationChannelInfo);
			s64 rotationInfoSize 		= asset.channelCount * sizeof(AnimationChannelInfo);

			s64 translationTimesSize 	= asset.totalTranslationKeyframeCount * sizeof(f32);
			s64 rotationTimesSize 		= asset.totalRotationKeyframeCount * sizeof(f32);

			s64 translationValuesSize 	= asset.totalTranslationKeyframeCount * sizeof(v3);
			s64 rotationValuesSize 		= asset.totalRotationKeyframeCount * sizeof(quaternion);

			s64 totalMemorySize = translationInfoSize
								+ rotationInfoSize
								+ translationTimesSize
								+ rotationTimesSize
								+ translationValuesSize
								+ rotationValuesSize;

			u8 * memory = push_memory<u8>(*assets.allocator, totalMemorySize, ALLOC_GARBAGE);

			platform_file_read(assets.file, asset.dataOffset, totalMemorySize, memory);

			// Note(Leo): We should be cool, these should all align on 4 bytes
			animation->translationChannelInfos 	= reinterpret_cast<AnimationChannelInfo*>(memory);	memory += translationInfoSize;
			animation->rotationChannelInfos 	= reinterpret_cast<AnimationChannelInfo*>(memory);	memory += rotationInfoSize;
			animation->translationTimes 		= reinterpret_cast<f32*>(memory); 					memory += translationTimesSize;
			animation->rotationTimes 			= reinterpret_cast<f32*>(memory);					memory += rotationTimesSize;
			animation->translations 			= reinterpret_cast<v3*>(memory);					memory += translationValuesSize;
			animation->rotations 				= reinterpret_cast<quaternion*>(memory);			memory += rotationValuesSize;
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
		skeleton 		= push_memory<AnimatedSkeleton>(*assets.allocator, 1, ALLOC_GARBAGE);

		AssetFileSkeleton assetSkeleton = assets.skeletonHeaders[id];//fileHeader.skeletons[id];

		s32 boneCount 				= assetSkeleton.boneCount;
		AnimatedBone * bonesMemory 	= push_memory<AnimatedBone>(*assets.allocator, boneCount, ALLOC_GARBAGE);
		u64 memorySize 				= boneCount * sizeof(AnimatedBone);
		
		platform_file_read(assets.file, assetSkeleton.dataOffset, memorySize, bonesMemory);

		skeleton->bones 		= bonesMemory;
		skeleton->boneCount 	= boneCount;
		
		assets.skeletons[id] = skeleton;
	}	

	return skeleton;
}

internal AudioAsset * assets_get_audio(GameAssets & assets, SoundAssetId id)
{
	AudioAsset * audio = assets.audio[id];

	if (audio == nullptr)
	{
		audio = push_memory<AudioAsset>(*assets.allocator, 1, ALLOC_GARBAGE);

		AssetFileSound asset 	= assets.soundHeaders[id]; //fileHeader.sounds[id];
		audio->sampleCount 		= asset.sampleCount;
		audio->leftChannel 		= push_memory<f32>(*assets.allocator, asset.sampleCount, ALLOC_GARBAGE);
		audio->rightChannel 	= push_memory<f32>(*assets.allocator, asset.sampleCount, ALLOC_GARBAGE);

		s64 fileSizePerChannel = sizeof(f32) * asset.sampleCount;
		platform_file_read(assets.file, asset.dataOffset, fileSizePerChannel, audio->leftChannel);
		platform_file_read(assets.file, asset.dataOffset + fileSizePerChannel, fileSizePerChannel, audio->rightChannel);


		assets.audio[id] 	= audio;
	}

	return audio;
}

internal Font * assets_get_font(GameAssets & assets, FontAssetId id)
{
	Font * font = assets.fonts[id];

	if (font == nullptr)
	{
		#if 1
		AssetFileFont assetFont = assets.fontHeaders[id];//fileHeader.fonts[id];

		u64 textureMemorySize 	= assetFont.width * assetFont.height * assetFont.channels;
		u64 textureMemoryOffset = assetFont.dataOffset;

		TextureAssetData assetData = {};
		assetData.pixelMemory = push_memory<u8>(*global_transientMemory, textureMemorySize, ALLOC_GARBAGE);
		platform_file_read(assets.file, textureMemoryOffset, textureMemorySize, assetData.pixelMemory);

		assetData.width 		= assetFont.width;
		assetData.height 		= assetFont.height;
		assetData.channels 		= assetFont.channels;
		assetData.format 		= assetFont.format;
		assetData.addressMode 	= assetFont.addressMode;

		Assert(assetData.channels == 4);

		log_debug(FILE_ADDRESS, "Load font: ", assetData.width, ", ", assetData.height, ", ", assetData.channels);

		u64 characterMemorySize 	= sizeof(FontCharacterInfo) * Font::characterCount;
		u64 characterMemoryOffset 	= textureMemoryOffset + textureMemorySize;
	

		font = push_memory<Font>(*assets.allocator, 1, ALLOC_GARBAGE);


		auto textureHandle = graphics_memory_push_texture(platformGraphics, &assetData);
		font->atlasTexture = graphics_memory_push_material(platformGraphics, GraphicsPipeline_screen_gui, 1, &textureHandle);
		platform_file_read(assets.file, characterMemoryOffset, characterMemorySize, font->characters);

		log_application(0, "Font loaded from asset pack file");

		#else

		FontLoadResult loadResult;

		if (id == FontAssetId_menu)
		{
			// *font = load_font("c:/windows/fonts/arial.ttf");
			loadResult =  load_font("assets/fonts/TheStrangerBrush.ttf");
		}
		else if (id == FontAssetId_game)
		{
			loadResult =  load_font("assets/fonts/SourceCodePro-Regular.ttf");
		}

		font = push_memory<Font>(*assets.allocator, 1, ALLOC_GARBAGE);
		*font = loadResult.font;

	    auto atlasTexture   = graphics_memory_push_texture(platformGraphics, &loadResult.atlasTextureAsset);
    	font->atlasTexture   = graphics_memory_push_material(platformGraphics, GraphicsPipeline_screen_gui, 1, &atlasTexture);

		for(s32 i = 0; i < 128; ++i)
		{
			log_debug(FILE_ADDRESS, (char)i, ": ", font->characters[i].uvPosition, ", ", font->characters[i].uvSize);
		}


		#endif

		assets.fonts[id] = font;

	}

	return font;
}