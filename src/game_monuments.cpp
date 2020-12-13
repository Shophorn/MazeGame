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

		quaternion rotation = quaternion_axis_angle(v3_up, to_radians(monuments.monuments[i].rotation));

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
		Transform3D {{0, 2, -0.07}, 	quaternion_axis_angle(v3_right, π/4), 	{1.75, 0.575, 0.575}},
		Transform3D {{0, -2, -0.07}, 	quaternion_axis_angle(v3_right, π/4), 	{1.75, 0.575, 0.575}},
		Transform3D {{2, 0, -0.07}, 	quaternion_axis_angle(v3_forward, π/4), {0.575, 1.75, 0.575}},
		Transform3D {{-2, 0, -0.07}, 	quaternion_axis_angle(v3_forward, π/4), {0.575, 1.75, 0.575}},

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

	MeshHandle baseMesh 		= assets_get_mesh(assets, MeshAssetId_monument_base);
	MeshHandle archMesh 		= assets_get_mesh(assets, MeshAssetId_monument_arcs);
	MeshHandle ornamentMeshes 	= assets_get_mesh(assets, MeshAssetId_monument_top_1);
	MaterialHandle material 	= assets_get_material(assets, MaterialAssetId_environment);

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

		quaternion rotation = quaternion_axis_angle(v3_up, to_radians(monuments.monuments[i].rotation));

		monuments.transforms[i] = {position, rotation, {2,2,2}};
	}
}

static void monuments_editor(Monuments & monuments, CollisionSystem3D & collisionSystem)
{
	using namespace ImGui;

	// Todo(Leo): Use move gizmo and rotation
	for (s32 i = 0; i < monuments.count; ++i)
	{
		PushID(i);

		Text("Monument %i", i);

		float rotation = monuments.monuments[i].rotation;
		if (DragFloat("Rotation", &rotation))
		{
			if (rotation > 360)
			{
				rotation = mod_f32(rotation, 360);
			}

			if (rotation < 0)
			{
				rotation = 360 - mod_f32(rotation, 360);
			}

			monuments.monuments[i].rotation = rotation;
			monuments.transforms[i].rotation 	= quaternion_axis_angle(v3_up, to_radians(rotation));						
		}


		v2 xy = monuments.monuments[i].position;
		if (DragV2("Position XY", &xy))
		{
			v3 position = {xy.x, xy.y, get_terrain_height(collisionSystem, xy)};
			monuments.monuments[i].position = position.xy;
			monuments.transforms[i].position = position;
		}


		FS_DEBUG_ALWAYS
		(
			v3 position = (monuments.transforms[i].position + v3{0,0,30});

			v3 right = rotate_v3(monuments.transforms[i].rotation, v3_right);
			debug_draw_line(position - right * 300, position + right * 300, colour_bright_red);										

			v3 forward = rotate_v3(monuments.transforms[i].rotation, v3_forward);
			debug_draw_line(position - forward * 300, position + forward * 300, colour_bright_red);
		)

		Separator();
		PopID();
	}
}
