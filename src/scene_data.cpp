struct SceneAssetHeader
{
	s64 buildingBlockCount;
	s64 buildingBlockOffset;	
};

struct SceneAsset
{
	s64 	buildingBlockCount;
	v3 * 	buildingBlockPositions;
};

struct Scene
{
	Array<m44> buildingBlocks;
};

static void scene_asset_load_2 (Scene & scene)
{
	PlatformFileHandle file = platform_file_open("scene_asset.fs", FileMode_read);

	SceneAssetHeader header = {};
	platform_file_read(file, 0, sizeof(SceneAssetHeader), &header);

	Assert(scene.buildingBlocks.capacity >= header.buildingBlockCount);
	scene.buildingBlocks.count = header.buildingBlockCount;
	platform_file_read(	file,
						header.buildingBlockOffset,
						sizeof(m44) * header.buildingBlockCount,
						scene.buildingBlocks.begin());

	platform_file_close(file);
}

/*
static SceneAsset scene_asset_load(MemoryArena & allocator, s64 buildingBlockCapacity)
{
	SceneAsset result = {};

	PlatformFileHandle file = platform_file_open("scene_asset.fs", FileMode_read);

	SceneAssetHeader header = {};

	platform_file_read(file, 0, sizeof(SceneAssetHeader), &header);

	result.buildingBlockCount 		= header.buildingBlockCount;
	result.buildingBlockPositions 	= push_memory<v3>(allocator, buildingBlockCapacity, ALLOC_GARBAGE);

	platform_file_read(	file,
						header.buildingBlockOffset,
						header.buildingBlockCount * sizeof(v3),
						result.buildingBlockPositions);

	platform_file_close(file);

	return result;
};
*/

// static SceneAsset editor_scene_asset_load(MemoryArena & allocator, EditorSceneAssetDescription const & editorDescription)
// {
// 	SceneAsset result = {};

// 	PlatformFileHandle file = platform_file_open("scene_asset.fs", FileMode_read);

// 	SceneAssetHeader header = {};

// 	platform_file_read(file, 0, sizeof(SceneAssetHeader), &header);

// 	result.buildingBlockCount 		= header.buildingBlockCount;
// 	result.buildingBlockPositions 	= push_memory<v3>(allocator, editorDescription.buildingBlockCapacity, ALLOC_Z);
// }

static void editor_scene_asset_write(Scene const & scene)
{
	PlatformFileHandle file = platform_file_open("scene_asset.fs", FileMode_write);

	SceneAssetHeader header = {};
	s64 fileOffset 			= sizeof header;

	header.buildingBlockCount 	= scene.buildingBlocks.count;
	header.buildingBlockOffset 	= fileOffset;
	s64 buildingBlocksFileSize 	= sizeof(m44) * scene.buildingBlocks.count;

	platform_file_write(file, fileOffset, buildingBlocksFileSize, scene.buildingBlocks.memory);

	fileOffset += buildingBlocksFileSize;

	platform_file_write(file, 0, sizeof(SceneAssetHeader), &header);

	platform_file_close(file);
}

