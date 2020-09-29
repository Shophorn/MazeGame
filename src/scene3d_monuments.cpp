/*
Leo Tamminen

Monuments
*/

struct Monument
{
	v2 	position;
	f32 rotation;

	static constexpr auto serializedProperties = make_property_list
	(
		serialize_property("position", &Monument::position),
		serialize_property("rotation", &Monument::rotation)
	);
};

struct Monuments
{
	s32 			count;
	Transform3D *	transforms;
	Monument 		monuments[5];
};


internal auto monuments_get_serialized_objects(Monuments & monuments)
{
	auto serializedObjects = make_property_list
	(
		serialize_object("monument_0", monuments.monuments[0]),
		serialize_object("monument_1", monuments.monuments[1]),
		serialize_object("monument_2", monuments.monuments[2]),
		serialize_object("monument_3", monuments.monuments[3]),
		serialize_object("monument_4", monuments.monuments[4])
	);

	return serializedObjects;
}

internal Monuments init_monuments(MemoryArena & persistentMemory, GameAssets & assets, CollisionSystem3D & collisionSystem)
{
	auto checkpoint = memory_push_checkpoint(*global_transientMemory);

	auto on_ground = [&collisionSystem](v2 xy) -> v3
	{
		v3 position = {xy.x, xy.y, get_terrain_height(collisionSystem, xy)};
		return position;
	};


	Monuments monuments = {};
	read_settings_file(monuments_get_serialized_objects(monuments));

	monuments.count 		= array_count(monuments.monuments);
	monuments.transforms 	= push_memory<Transform3D>(persistentMemory, monuments.count, ALLOC_GARBAGE);

	for (s32 i = 0; i < monuments.count; ++i)
	{
		v3 position;
		position.xy = monuments.monuments[i].position;
		position.z	= get_terrain_height(collisionSystem, position.xy);

		quaternion rotation = axis_angle_quaternion(v3_up, to_radians(monuments.monuments[i].rotation));

		monuments.transforms[i] = {position, rotation, {2,2,2}};
	}

	memory_pop_checkpoint(*global_transientMemory, checkpoint);

	return monuments;
}

internal void monuments_submit_colliders(Monuments const & monuments, CollisionSystem3D & collisionSystem)
{
	local_persist Transform3D colliderTransforms [] =
	{
		Transform3D {{0, 0, -0.2356}, 	identity_quaternion,					{2, 2, 0.99}},
		Transform3D {{0, 2, -0.07}, 	axis_angle_quaternion(v3_right, π/4), 	{1.75, 0.575, 0.575}},
		Transform3D {{0, -2, -0.07}, 	axis_angle_quaternion(v3_right, π/4), 	{1.75, 0.575, 0.575}},
		Transform3D {{2, 0, -0.07}, 	axis_angle_quaternion(v3_forward, π/4), {0.575, 1.75, 0.575}},
		Transform3D {{-2, 0, -0.07}, 	axis_angle_quaternion(v3_forward, π/4), {0.575, 1.75, 0.575}},

		Transform3D {{1.75, 1.86, 2}, 	identity_quaternion, {0.67, 0.56, 5}},
		Transform3D {{-1.75, 1.86, 2}, 	identity_quaternion, {0.67, 0.56, 5}},
		Transform3D {{1.75, -1.86, 2}, 	identity_quaternion, {0.67, 0.56, 5}},
		Transform3D {{-1.75, -1.86, 2}, identity_quaternion, {0.67, 0.56, 5}},
	};

	for (s32 i = 0; i < monuments.count; ++i)
	{
		m44 transformMatrix = transform_matrix(monuments.transforms[i]);
		m44 inverseMatrix 	= inverse_transform_matrix(monuments.transforms[i]);	

		for (auto collider : colliderTransforms)
		{
			m44 colliderTransform 			= transform_matrix(collider);
			m44 colliderInverseTransform 	= inverse_transform_matrix(collider);

			collisionSystem.submittedBoxColliders.push({transformMatrix * colliderTransform,
													colliderInverseTransform * inverseMatrix});
		}
	}	
}

internal void monuments_draw(Monuments const & monuments, GameAssets & assets)
{
	m44 * transformMatrices = push_memory<m44>(*global_transientMemory, monuments.count, ALLOC_GARBAGE);

	for (s32 i = 0; i < monuments.count; ++i)
	{
		transformMatrices[i] = transform_matrix(monuments.transforms[i]);
	}

	MeshHandle baseMesh 		= assets_get_mesh(assets, MESH_ASSET_MONUMENT_BASE);
	MeshHandle archMesh 		= assets_get_mesh(assets, MESH_ASSET_MONUMENT_ARCS);
	MeshHandle ornamentMeshes 	= assets_get_mesh(assets, MESH_ASSET_MONUMENT_TOP_1);
	MaterialHandle material 	= assets_get_material(assets, MATERIAL_ASSET_ENVIRONMENT);

	graphics_draw_meshes(platformGraphics, monuments.count, transformMatrices, baseMesh, material);
	graphics_draw_meshes(platformGraphics, monuments.count, transformMatrices, archMesh, material);
	graphics_draw_meshes(platformGraphics, monuments.count, transformMatrices, ornamentMeshes, material);
}


internal void monuments_refresh_transforms(Monuments & monuments, CollisionSystem3D const & collisionSystem)
{
	for (s32 i = 0; i < monuments.count; ++i)
	{
		v3 position;
		position.xy = monuments.monuments[i].position;
		position.z	= get_terrain_height(collisionSystem, position.xy);

		quaternion rotation = axis_angle_quaternion(v3_up, to_radians(monuments.monuments[i].rotation));

		monuments.transforms[i] = {position, rotation, {2,2,2}};
	}
}