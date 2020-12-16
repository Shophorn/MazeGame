struct SceneAssetHeader
{
	s64 buildingBlockCount;
	s64 buildingBlockOffset;

	s64 buildingPipeCount;
	s64 buildingPipeOffset;
};

struct Scene
{
	Array<m44> buildingBlocks;
	Array<m44> buildingPipes;
};

static void scene_asset_load (Scene & scene)
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

	Assert(scene.buildingBlocks.capacity >= header.buildingPipeCount);
	scene.buildingPipes.count = header.buildingPipeCount;
	platform_file_read( file,
						header.buildingPipeOffset,
						sizeof(m44) * header.buildingPipeCount,
						scene.buildingPipes.begin());
	
	platform_file_close(file);
}

static void editor_scene_asset_write(Scene const & scene)
{
	PlatformFileHandle file = platform_file_open("scene_asset.fs", FileMode_write);

	SceneAssetHeader header = {};
	s64 fileOffset 			= sizeof header;

	header.buildingBlockCount 	= scene.buildingBlocks.count;
	header.buildingBlockOffset 	= fileOffset;
	s64 buildingBlocksFileSize 	= sizeof(m44) * scene.buildingBlocks.count;
	platform_file_write(file, fileOffset, buildingBlocksFileSize, scene.buildingBlocks.begin());
	fileOffset 					+= buildingBlocksFileSize;

	header.buildingPipeCount 	= scene.buildingPipes.count;
	header.buildingPipeOffset	= fileOffset;
	s64 buildingPipesFileSize 	= sizeof(m44) * scene.buildingPipes.count;
	platform_file_write(file, fileOffset, buildingPipesFileSize, scene.buildingPipes.begin());
	fileOffset 					+= buildingPipesFileSize;

	platform_file_write(file, 0, sizeof(SceneAssetHeader), &header);

	platform_file_close(file);
}

