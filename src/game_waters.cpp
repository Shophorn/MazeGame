struct Waters
{
	s32 			capacity;
	s32 			count;

	v3 * 			positions;
	quaternion * 	rotations;
	f32	* 			levels;

	s32 fullWaterLevel 			= 1;
	f32 evaporateLevelPerSecond = 0.05;
};

internal void initialize_waters(Waters & waters, MemoryArena & allocator)
{
	waters = {};

	waters.capacity 	= 100'000;
	waters.count 		= 0;
	waters.positions 	= push_memory<v3>(allocator, waters.capacity, ALLOC_GARBAGE);
	waters.rotations 	= push_memory<quaternion>(allocator, waters.capacity, ALLOC_GARBAGE);
	waters.levels 		= push_memory<f32>(allocator, waters.capacity, ALLOC_GARBAGE);
}

// Todo(Leo): testing these here, move to memory
template<typename T>
internal void memory_unordered_pop(T * memory, s32 index, s32 count)
{
	memory[index] = memory[count - 1];
}

internal void update_waters(Waters & waters, f32 elapsedTime)
{
	for (s32 i = 0; i < waters.count; ++i)
	{
		waters.levels[i] -= waters.evaporateLevelPerSecond * elapsedTime;

		if (waters.levels[i] < 0)
		{
			memory_unordered_pop(waters.positions, i, waters.count);
			memory_unordered_pop(waters.rotations, i, waters.count);
			memory_unordered_pop(waters.levels, i, waters.count);

			waters.count -= 1;
			i -= 1;
		}
	}
}

internal void waters_instantiate(Waters & waters, PhysicsWorld & physicsWorld, v3 position, f32 level)
{
	if (waters.count < waters.capacity)
	{
		s32 index 		= waters.count;
		waters.count 	+= 1;

		waters.positions[index] = position;
		waters.rotations[index] = identity_quaternion;
		waters.levels[index] 	= level;

		// physics_world_push_entity(physicsWorld, {EntityType_water, index});
	}
}

internal void draw_waters(Waters & waters, PlatformGraphics * graphics, GameAssets & assets)
{
	if (waters.count > 0)
	{
		m44 * waterTransforms = push_memory<m44>(*global_transientMemory, waters.count, ALLOC_GARBAGE);
		for (s32 i = 0; i < waters.count; ++i)
		{
			waterTransforms[i] = transform_matrix(	waters.positions[i],
													waters.rotations[i],
													make_uniform_v3(waters.levels[i] / waters.fullWaterLevel));
		}

		graphics_draw_meshes(	platformGraphics, waters.count, waterTransforms, 
								assets_get_mesh(assets, MeshAssetId_water_drop),
								assets_get_material(assets, MaterialAssetId_water));
	}
}
