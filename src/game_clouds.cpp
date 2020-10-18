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
	Array<Cloud> clouds;

	f32 growSpeed 			= 0.1;
	f32 altitude 			= 200;
	f32 windSpeed 			= 1;
	f32 windDirectionAngle 	= 0;

	f32 rainSizeThreshold  		= 50;
	f32 rainSizeDecreaseSpeed 	= 1;

	f32 rainWaterDropTotalGrowSpeedPerAreaPerSecond	= 0.001;
	f32 rainMaxGrowSpeedForSingleDropPerSecond 		= 0.5;

	MeshHandle 		rainMesh;
	MaterialHandle 	rainMaterial;
};

internal void initialize_clouds(Clouds & clouds, MemoryArena & allocator)
{
	clouds = {};

	s32 initialCloudCount 	= 2;
	// s32 initialCloudCount 	= 50;
	s32 cloudCapacity 		= 1000;
	
	clouds.clouds 			= push_array<Cloud>(allocator, cloudCapacity, ALLOC_GARBAGE);
	clouds.clouds.count 	= initialCloudCount;

	for (auto & cloud : clouds.clouds)
	{
		cloud.transform = identity_transform;

		// constexpr f32 range 		= 500;
		// cloud.transform.position 	= random_inside_unit_square() * range - v3{range / 2, range / 2, 0};
		// cloud.radius 				= random_range(5, 50);

		constexpr f32 range 		= 50;
		cloud.transform.position 	= random_inside_unit_square() * range - v3{range / 2, range / 2, 0};
		cloud.radius 				= random_range(45, 49);

	}
}

internal void update_clouds(Clouds & 			clouds,
							Waters & 			waters,
							PhysicsWorld & 		physicsWorld,
							CollisionSystem3D & collisionSystem,
							f32 				elapsedTime)
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
			{
				/*
				1. Find all drops inside rain and that are small enough
				2. if threre is not enough drops, spawn more
				3. grow all selected drops by amount
				*/

				s32 maxWaterDropsInRain = 1000;
				Array<s32> selectedWaterDropIndices = push_array<s32>(*global_transientMemory, maxWaterDropsInRain, ALLOC_GARBAGE);

				for (s32 waterIndex = 0; waterIndex < waters.count; ++waterIndex)
				{
					f32 distanceToWaterDrop = v2_length(cloud.transform.position.xy - waters.positions[waterIndex].xy);
					if (distanceToWaterDrop < cloud.radius && waters.levels[waterIndex] < 1)
					{
						selectedWaterDropIndices.push(waterIndex);
					}
				}

				f32 area 					= π * cloud.radius * cloud.radius;
				f32 totalWaterGrowthAmount 	= area * elapsedTime * clouds.rainWaterDropTotalGrowSpeedPerAreaPerSecond;

				s32 requiredWaterDropCount 	= totalWaterGrowthAmount / (clouds.rainMaxGrowSpeedForSingleDropPerSecond * elapsedTime);
				s32 spawnCount 				= requiredWaterDropCount - selectedWaterDropIndices.count;

				while (spawnCount > 0)
				{
					spawnCount -= 1;

					f32 distance 	= random_range(0, cloud.radius);
					f32 angle 		= random_value() * 2 * π;

					v3 localPosition 	= rotate_v3(quaternion_axis_angle(v3_up, angle), v3_forward) * distance;
					v3 position 		= cloud.transform.position + localPosition;
					position.z 			= get_terrain_height(collisionSystem, position.xy);

					selectedWaterDropIndices.push(waters.count);

					waters_instantiate(waters, physicsWorld, position, 0);	
				}

				f32 actualWaterDropGrowth = totalWaterGrowthAmount / selectedWaterDropIndices.count;

				for (s32 index : selectedWaterDropIndices)
				{
					waters.levels[index] += actualWaterDropGrowth;
					waters.levels[index] = min_f32(waters.levels[index], waters.fullWaterLevel);
				}
			}






			cloud.radius 			-= clouds.rainSizeDecreaseSpeed * elapsedTime;
			cloud.transform.scale 	= {cloud.radius, cloud.radius, cloud.radius};

			if (cloud.radius < 0)
			{
				array_unordered_remove(clouds.clouds, i);
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



	for (s32 i = 0; i < clouds.clouds.count; ++i)
	{
		if (clouds.clouds[i].hasStartedRaining)
		{
			v3 rainPosition = clouds.clouds[i].transform.position;
			
			f32 rainRadius 	= clouds.clouds[i].radius;
			f32 rainLength 	= clouds.altitude;
			v3 rainScale 	= {rainRadius, rainRadius, rainLength};

			m44 matrix = transform_matrix(rainPosition, identity_quaternion, rainScale);
			graphics_draw_meshes(	graphics,
									1, &matrix,
									assets_get_mesh(assets, MeshAssetId_rain),
									assets_get_material(assets, MaterialAssetId_water));
		}
	}
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
