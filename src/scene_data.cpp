struct SceneAssetHeader
{
	s64 buildingBlockCount;
	s64 buildingBlockOffset;

	s64 buildingPipeCount;
	s64 buildingPipeOffset;
};

static void scene_asset_load (Game * game)
{
	PlatformFileHandle file = platform_file_open("scene_asset.fs", FileMode_read);

	SceneAssetHeader header = {};
	platform_file_read(file, 0, sizeof(SceneAssetHeader), &header);

	{
		Scenery & buildingBlocks = game->sceneries[game->buildingBlockSceneryIndex];
		Assert(buildingBlocks.transforms.capacity >= header.buildingBlockCount);
		buildingBlocks.transforms.count = header.buildingBlockCount;
		platform_file_read(	file,
							header.buildingBlockOffset,
							sizeof(m44) * header.buildingBlockCount,
							buildingBlocks.transforms.begin());
	}

	{
		Scenery & buildingPipes = game->sceneries[game->buildingPipeSceneryIndex];
		Assert(buildingPipes.transforms.capacity >= header.buildingPipeCount);
		buildingPipes.transforms.count = header.buildingPipeCount;
		platform_file_read( file,
							header.buildingPipeOffset,
							sizeof(m44) * header.buildingPipeCount,
							buildingPipes.transforms.begin());
	}
	
	platform_file_close(file);
}

static void editor_scene_asset_write(Game const * game)
{
	PlatformFileHandle file = platform_file_open("scene_asset.fs", FileMode_write);

	SceneAssetHeader header = {};
	s64 fileOffset 			= sizeof header;

	{
		Scenery const & buildingBlocks 	= game->sceneries[game->buildingBlockSceneryIndex];
		header.buildingBlockCount 		= buildingBlocks.transforms.count;
		header.buildingBlockOffset 		= fileOffset;
		s64 buildingBlocksFileSize 		= sizeof(m44) * buildingBlocks.transforms.count;
		platform_file_write(file, fileOffset, buildingBlocksFileSize, buildingBlocks.transforms.begin());
		fileOffset 						+= buildingBlocksFileSize;
	}

	{
		Scenery const & buildingPipes 	= game->sceneries[game->buildingPipeSceneryIndex];
		header.buildingPipeCount 		= buildingPipes.transforms.count;
		header.buildingPipeOffset		= fileOffset;
		s64 buildingPipesFileSize 		= sizeof(m44) * buildingPipes.transforms.count;
		platform_file_write(file, fileOffset, buildingPipesFileSize, buildingPipes.transforms.begin());
		fileOffset 						+= buildingPipesFileSize;
	}

	platform_file_write(file, 0, sizeof(SceneAssetHeader), &header);

	platform_file_close(file);
}

