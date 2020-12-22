struct Ground
{
	// Note(Leo): There are multiple mesh handles here
	s64 			chunkCount;
	m44 * 			chunkTransforms;
	MeshHandle * 	chunkMeshes;
	MaterialAssetId material;
};

static Ground ground_initialize(MemoryArena & persistentMemory, GameAssets & assets, CollisionSystem3D & collisionSystem)
{
	constexpr f32 mapSize 				= 1200;
	constexpr f32 minTerrainElevation 	= -5;
	constexpr f32 maxTerrainElevation 	= 100;

	// Note(Leo): this is maximum size we support with u16 mesh vertex indices
	s32 chunkResolution			= 128;
	
	s32 chunkCountPerDirection 	= 10;
	f32 chunkSize 				= 1.0f / chunkCountPerDirection;
	f32 chunkWorldSize 			= chunkSize * mapSize;

	HeightMap heightmap;
	{
		auto checkpoint = memory_push_checkpoint(*global_transientMemory);
		
		s32 heightmapResolution = 1024;

		// todo(Leo): put to asset system thing
		TextureAssetData heightmapData 	= assets_get_texture_data(assets, TextureAssetId_heightmap);
		heightmap 						= make_heightmap(&persistentMemory, &heightmapData, heightmapResolution, mapSize, minTerrainElevation, maxTerrainElevation);

		memory_pop_checkpoint(*global_transientMemory, checkpoint);
	}

	Ground ground = {};

	s64 chunkCount 			= chunkCountPerDirection * chunkCountPerDirection;
	
	ground.chunkCount 		= chunkCount;
	ground.chunkTransforms 	= push_memory<m44>(persistentMemory, chunkCount, ALLOC_GARBAGE);
	ground.chunkMeshes 		= push_memory<MeshHandle>(persistentMemory, chunkCount, ALLOC_GARBAGE);
	ground.material 		= MaterialAssetId_ground;

	/// GENERATE GROUND MESH
	{
		auto checkpoint = memory_push_checkpoint(*global_transientMemory);
	
		for (s64 i = 0; i < chunkCount; ++i)
		{
			s64 x = i % chunkCountPerDirection;
			s64 y = i / chunkCountPerDirection;

			v2 position = { x * chunkSize, y * chunkSize };
			v2 size 	= { chunkSize, chunkSize };

			auto groundMeshAsset 	= generate_terrain(*global_transientMemory, heightmap, position, size, chunkResolution, 20);
			ground.chunkMeshes[i] = graphics_memory_push_mesh(platformGraphics, &groundMeshAsset);
		}
	
		memory_pop_checkpoint(*global_transientMemory, checkpoint);
	}

	f32 halfMapSize = mapSize / 2;
	v3 terrainOrigin = {-halfMapSize, -halfMapSize, 0};

	for (s64 i = 0; i < chunkCount; ++i)
	{
		s64 x = i % chunkCountPerDirection;
		s64 y = i / chunkCountPerDirection;

		v3 leftBackCornerPosition = {x * chunkWorldSize - halfMapSize, y * chunkWorldSize - halfMapSize, 0};
		ground.chunkTransforms[i] = translation_matrix(leftBackCornerPosition);
	}

	collisionSystem.terrainCollider 	= heightmap;
	collisionSystem.terrainOffset 		= {{-mapSize / 2, -mapSize / 2, 0}};

	return ground;
}

static void ground_render(Ground & ground, PlatformGraphics * graphics, GameAssets & assets)
{
	for(s32 i = 0; i < ground.chunkCount; ++i)
	{
		graphics_draw_meshes(	graphics,
								1, ground.chunkTransforms + i,
								ground.chunkMeshes[i],
								assets_get_material(assets,ground.material));
	}
}

