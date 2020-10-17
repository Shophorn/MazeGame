struct Waters
{
	s32 			capacity;
	s32 			count;
	Transform3D * 	transforms;
	f32	* 			levels;

	s32 			fullWaterLevel = 1;
};

internal void initialize_waters(Waters & waters, MemoryArena & allocator)
{
	waters = {};

	waters.capacity 	= 100000;
	waters.count 		= 0;
	waters.transforms 	= push_memory<Transform3D>(allocator, waters.capacity, ALLOC_GARBAGE);
	waters.levels 		= push_memory<f32>(allocator, waters.capacity, ALLOC_GARBAGE);
}

internal void waters_instantiate(Waters & waters, PhysicsWorld & physicsWorld, v3 position, f32 level)
{
	if (waters.count < waters.capacity)
	{
		s32 index 		= waters.count;
		waters.count 	+= 1;

		waters.transforms[index] 			= identity_transform;
		waters.transforms[index].position 	= position;
		waters.levels[index] 				= level;

		physics_world_push_entity(physicsWorld, EntityType_water, index);
	}
}

internal void draw_waters(Waters & waters, PlatformGraphics * graphics, GameAssets & assets)
{
	if (waters.count > 0)
	{
		m44 * waterTransforms = push_memory<m44>(*global_transientMemory, waters.count, ALLOC_GARBAGE);
		for (s32 i = 0; i < waters.count; ++i)
		{
			waterTransforms[i] = transform_matrix(	waters.transforms[i].position,
													waters.transforms[i].rotation,
													make_uniform_v3(waters.levels[i] / waters.fullWaterLevel));
		}

		graphics_draw_meshes(	platformGraphics, waters.count, waterTransforms, 
								assets_get_mesh(assets, MeshAssetId_water_drop),
								assets_get_material(assets, MaterialAssetId_water));
	}
}
