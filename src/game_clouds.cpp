/*
Leo Tamminen

Cloud game objects code for Project FriendSimulator
*/

struct Cloud
{	
	Transform3D transform;
	f32 		radius;
	bool32 		hasStartedRaining;
	f32 		waterInstantiateTimer;
};

struct Clouds
{
	Array2<Cloud> clouds;

	f32 growSpeed 			= 0.1;
	f32 altitude 			= 200;
	f32 windSpeed 			= 1;
	f32 windDirectionAngle 	= 0;

	f32 rainSizeThreshold  		= 50;
	f32 rainSizeDecreaseSpeed 	= 10;
};

internal void initialize_clouds(Clouds & clouds, MemoryArena & allocator)
{
	clouds = {};

	s32 initialCloudCount 	= 50;
	s32 cloudCapacity 		= 1000;
	
	clouds.clouds 			= push_array_2<Cloud>(allocator, cloudCapacity, ALLOC_GARBAGE);
	clouds.clouds.count 	= initialCloudCount;

	for (auto & cloud : clouds.clouds)
	{
		cloud.transform = identity_transform;

		constexpr f32 range 		= 500;
		cloud.transform.position 	= random_inside_unit_square() * range - v3{range / 2, range / 2, 0};
		cloud.radius 				= random_range(5, 50);

	}
}

internal void simulate_clouds(Clouds & clouds, Waters & waters, PhysicsWorld & physicsWorld, f32 elapsedTime)
{
	v3 windDirection = rotate_v3(quaternion_axis_angle(v3_up, clouds.windDirectionAngle), v3_forward);

	// for (auto & cloud : clouds.clouds)
	for (s32 i = 0; i < clouds.clouds.count; ++i)
	{
		auto & cloud = clouds.clouds[i];

		cloud.transform.position 	+= clouds.windSpeed * windDirection * elapsedTime;
		cloud.transform.position.z 	= clouds.altitude;

		FS_DEBUG_BACKGROUND(debug_draw_circle_xy(cloud.transform.position, cloud.radius, colour_bright_red));

		if (cloud.hasStartedRaining)
		{
			cloud.waterInstantiateTimer -= elapsedTime;
			if (cloud.waterInstantiateTimer < 0)
			{
				cloud.waterInstantiateTimer = 0.1;

				f32 distance 	= random_range(0, cloud.radius);
				f32 angle 		= random_value() * 2 * π;

				v3 localPosition 	= rotate_v3(quaternion_axis_angle(v3_up, angle), v3_forward) * distance;
				v3 position 		= cloud.transform.position + localPosition;

				waters_instantiate(waters, physicsWorld, position, 1);
			}

			cloud.radius 			-= clouds.rainSizeDecreaseSpeed * elapsedTime;
			cloud.transform.scale 	= {cloud.radius, cloud.radius, cloud.radius};

			if (cloud.radius < 0)
			{
				array_2_unordered_remove(clouds.clouds, i);
				i -= 1;
			}
		}
		else
		{
			cloud.radius 			+= clouds.growSpeed * elapsedTime;
			cloud.transform.scale 	= {cloud.radius, cloud.radius, cloud.radius};

			if (cloud.radius > clouds.rainSizeThreshold)
			{
				log_debug(FILE_ADDRESS, "Cloud started raining");

				cloud.hasStartedRaining 	= true;
				cloud.waterInstantiateTimer = 0;
			}
		}
	}
}

internal void draw_clouds(Clouds & clouds, PlatformGraphics * graphics, GameAssets & assets)
{
	if (clouds.clouds.count <= 0)
	{
		return;
	}

	m44 * matrices = push_memory<m44>(*global_transientMemory, clouds.clouds.count, ALLOC_GARBAGE);
	for (s32 i = 0; i < clouds.clouds.count; ++i)
	{
		matrices[i] = transform_matrix(clouds.clouds[i].transform);
	}
	graphics_draw_meshes(	graphics,
							clouds.clouds.count, matrices,
							assets_get_mesh(assets, MeshAssetId_cloud),
							assets_get_material(assets, MaterialAssetId_clouds));
}

internal void clouds_menu(Clouds & clouds)
{
	gui_start_panel("EDIT CLOUDS", {});

	gui_float_field("Grow speed", &clouds.growSpeed);
	gui_float_field("Cloud altitude", &clouds.altitude);
	gui_float_field("Wind Speed", &clouds.windSpeed);

	f32 windDirectionAngleDegrees = to_degrees(clouds.windDirectionAngle);
	if (gui_float_field("Wind Direction Angle", &windDirectionAngleDegrees))
	{
		if (windDirectionAngleDegrees < 0)
		{
			windDirectionAngleDegrees = 360 + windDirectionAngleDegrees;	
		}
		clouds.windDirectionAngle = to_radians(mod_f32(windDirectionAngleDegrees, 360));
	}

	gui_float_field("Rain Size Threshold", &clouds.rainSizeThreshold);
	gui_float_field("Rain Size Decrease Speed", &clouds.rainSizeDecreaseSpeed);

	gui_end_panel();

}
