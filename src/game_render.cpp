
internal void game_render(Game * game)
{
	auto * graphics = platformGraphics;


	/// UPDATE GLOBAL UNIFORMS, Camera. lights etc
	{
		update_camera_system(&game->worldCamera, graphics, platformWindow);
	
		v3 lightDirection = {0,0,1};
		lightDirection = quaternion_rotate_v3(quaternion_axis_angle(v3_right, game->skySettings.sunHeightAngle * π), lightDirection);
		lightDirection = quaternion_rotate_v3(quaternion_axis_angle(v3_up, game->skySettings.sunOrbitAngle * 2 * π), lightDirection);
		lightDirection = v3_normalize(-lightDirection);

		Light light = { .direction 	= lightDirection, //v3_normalize({-1, 1.2, -8}), 
						.color 		= v3{0.95, 0.95, 0.9}};
		v3 ambient 	= {0.05, 0.05, 0.3};

		light.skyColorSelection = game->skySettings.skyColourSelection;

		light.skyGroundColor.rgb 	= game->skySettings.skyGradientGround;
		light.skyBottomColor.rgb 	= game->skySettings.skyGradientBottom;
		light.skyTopColor.rgb 		= game->skySettings.skyGradientTop;

		light.horizonHaloColorAndFalloff.rgb 	= game->skySettings.horizonHaloColour;
		light.horizonHaloColorAndFalloff.a 		= game->skySettings.horizonHaloFalloff;

		light.sunHaloColorAndFalloff.rgb 	= game->skySettings.sunHaloColour;
		light.sunHaloColorAndFalloff.a 		= game->skySettings.sunHaloFalloff;

		light.sunDiscColor.rgb 			= game->skySettings.sunDiscColour;
		light.sunDiscSizeAndFalloff.xy 	= {	game->skySettings.sunDiscSize,
											game->skySettings.sunDiscFalloff };

		graphics_drawing_update_lighting(graphics, &light, &game->worldCamera, ambient);
		HdrSettings hdrSettings = 
		{
			game->skySettings.hdrExposure,
			game->skySettings.hdrContrast,
		};

		// Todo(Leo): this really need updating only just before post process, but it doesn't really matter
		graphics_drawing_update_hdr_settings(graphics, &hdrSettings);
	}

	/// DRAW SKY
	graphics_draw_model(graphics, game->skybox, identity_m44, false, nullptr, 0);
	// graphics_draw_meshes(graphics,
	// 					1, &identity_m44,
	// 					assets_get_mesh(game->assets, MeshAssetId_skysphere),
	// 					assets_get_material(game->assets, MaterialAssetId_sky));



	/// DRAW POTS
	{
		// Todo(Leo): store these as matrices, we can easily retrieve position (that is needed somwhere) from that too.
		m44 * potTransformMatrices = push_memory<m44>(*global_transientMemory, game->smallPots.count, ALLOC_GARBAGE);
		for(s32 i = 0; i < game->smallPots.count; ++i)
		{
			potTransformMatrices[i] = transform_matrix(game->smallPots.transforms[i]);
		}
		graphics_draw_meshes(graphics, game->smallPots.count, potTransformMatrices, game->potMesh, game->potMaterial);
	}

	/// DRAW STATIC SCENERY
	for (auto const & scenery : game->sceneries)
	{
		graphics_draw_meshes(	graphics, scenery.count, scenery.transforms,
								assets_get_mesh(game->assets, scenery.mesh),
								assets_get_material(game->assets, scenery.material));
	}
	
	{
		for(s32 i = 0; i < game->terrainCount; ++i)
		{
			graphics_draw_meshes(graphics, 1, game->terrainTransforms + i, game->terrainMeshes[i], game->terrainMaterial);
		}

		graphics_draw_meshes(graphics, 1, &game->seaTransform, game->seaMesh, game->seaMaterial);

		auto compute_transform_matrices = [](MemoryArena & allocator, s32 count, Transform3D * transforms)
		{
			m44 * result = push_memory<m44>(allocator, count, ALLOC_GARBAGE);

			for (s32 i = 0; i < count; ++i)
			{
				result[i] = transform_matrix(transforms[i]);
			}

			return result;
		};

		m44 * bigPotTransforms = compute_transform_matrices(*global_transientMemory, game->bigPotTransforms.count, game->bigPotTransforms.memory);
		graphics_draw_meshes(graphics, game->bigPotTransforms.count, bigPotTransforms, game->bigPotMesh, game->bigPotMaterial);		

		monuments_draw(game->monuments, game->assets);
	}


	// DRAW BOX CONTAINERS
	{
		MeshHandle boxMesh 		= assets_get_mesh(game->assets, MeshAssetId_box);
		MeshHandle boxCoverMesh = assets_get_mesh(game->assets, MeshAssetId_box_cover);
		MaterialHandle material = assets_get_material(game->assets, MaterialAssetId_box);

		m44 * boxTransformMatrices = push_memory<m44>(*global_transientMemory, game->boxes.count, ALLOC_GARBAGE);
		m44 * coverTransformMatrices = push_memory<m44>(*global_transientMemory, game->boxes.count, ALLOC_GARBAGE);

		for (s32 i = 0; i < game->boxes.count; ++i)
		{
			boxTransformMatrices[i] = transform_matrix(game->boxes.transforms[i]);
			coverTransformMatrices[i] = boxTransformMatrices[i] * transform_matrix(game->boxes.coverLocalTransforms[i]);
		}

		graphics_draw_meshes(graphics, game->boxes.count, boxTransformMatrices, boxMesh, material);
		graphics_draw_meshes(graphics, game->boxes.count, coverTransformMatrices, boxCoverMesh, material);
	}

	/// DEBUG DRAW COLLIDERS
	{
		FS_DEBUG_BACKGROUND(collisions_debug_draw_colliders(game->collisionSystem));
		FS_DEBUG_PLAYER(debug_draw_circle_xy(game->player.characterTransform.position + v3{0,0,0.7}, 0.25f, colour_bright_green));
	}

	if (game->drawMCStuff)
	{
		graphics_draw_procedural_mesh(	graphics,
										game->metaballVertexCount, game->metaballVertices,
										game->metaballIndexCount, game->metaballIndices,
										game->metaballTransform,
										game->metaballMaterial);

		if (game->metaballVertexCount2 > 0 && game->metaballIndexCount2 > 0)
		{
			graphics_draw_procedural_mesh(	graphics,
											game->metaballVertexCount2, game->metaballVertices2,
											game->metaballIndexCount2, game->metaballIndices2,
											game->metaballTransform2,
											game->metaballMaterial);
		}
	}

	{
		m44 trainTransformMatrix = transform_matrix(game->trainTransform);
		graphics_draw_meshes(graphics, 1, &trainTransformMatrix, game->trainMesh, game->trainMaterial);
	}

	/// DRAW RACCOONS
	{
		m44 * raccoonTransformMatrices = push_memory<m44>(*global_transientMemory, game->raccoonCount, ALLOC_GARBAGE);
		for (s32 i = 0; i < game->raccoonCount; ++i)
		{
			raccoonTransformMatrices[i] = transform_matrix(game->raccoonTransforms[i]);
		}
		graphics_draw_meshes(graphics, game->raccoonCount, raccoonTransformMatrices, game->raccoonMesh, game->raccoonMaterial);
	}


	/// PLAYER
	/// CHARACTER 2
	{

		m44 boneTransformMatrices [32];
		
		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, game->player.skeletonAnimator);

		graphics_draw_model(graphics, 	game->player.animatedRenderer.model,
												transform_matrix(game->player.characterTransform),
												true,
												boneTransformMatrices, array_count(boneTransformMatrices));

		// -------------------------------------------------------------------------------

		update_animated_renderer(boneTransformMatrices, game->noblePersonSkeletonAnimator);

		graphics_draw_model(graphics, 	game->player.animatedRenderer.model,
												transform_matrix(game->noblePersonTransform),
												true,
												boneTransformMatrices, array_count(boneTransformMatrices));
	}


	/// DRAW UNUSED WATER
	draw_clouds(game->clouds, graphics, game->assets);
	draw_waters(game->waters, graphics, game->assets);

	for (auto & tree : game->trees.array)
	{
		m44 transform = transform_matrix(tree.position, tree.rotation, {1,1,1});
		if (tree.mesh.vertices.count > 0 || tree.mesh.indices.count > 0)
		{
			graphics_draw_procedural_mesh(	graphics,
											tree.mesh.vertices.count, tree.mesh.vertices.memory,
											tree.mesh.indices.count, tree.mesh.indices.memory,
											transform, assets_get_material(game->assets, MaterialAssetId_tree));
		}

		if (tree.drawSeed)
		{
			m44 seedTransform = transform;
			if (tree.planted)
			{
				seedTransform[3].z -= 0.3;
			}
			graphics_draw_meshes(graphics, 1, &seedTransform, tree.seedMesh, tree.seedMaterial);
		}

		if (tree.hasFruit)
		{
			v3 fruitPosition = multiply_point(transform, tree.fruitPosition);

			f32 fruitSize = tree.fruitAge / tree.fruitMaturationTime;
			v3 fruitScale = {fruitSize, fruitSize, fruitSize};

			m44 fruitTransform = transform_matrix(fruitPosition, quaternion_identity, fruitScale);

			FS_DEBUG_ALWAYS(debug_draw_circle_xy(fruitPosition, 0.5, colour_bright_red));

			graphics_draw_meshes(graphics, 1, &fruitTransform, tree.seedMesh, tree.seedMaterial);
		}

		FS_DEBUG_BACKGROUND(debug_draw_circle_xy(tree.position, 2, colour_bright_red));
	}

	{
		for (auto & tree : game->trees.array)
		{
			leaves_draw(tree.leaves, tree.settings->leafColour);
		}
	}

	/// SCENE BUILDING BLOCKS
	{
		graphics_draw_meshes(	graphics,
								game->scene.buildingBlocks.count,
								game->scene.buildingBlocks.memory,
								assets_get_mesh(game->assets, MeshAssetId_default_cube),
								assets_get_material(game->assets, MaterialAssetId_building_block));

		graphics_draw_meshes( 	graphics,
								game->scene.buildingPipes.count,
								game->scene.buildingPipes.memory,
								assets_get_mesh(game->assets, MeshAssetId_default_cylinder),
								assets_get_material(game->assets, MaterialAssetId_building_block));
	}

	/// CAMERA TARGET GIZMO
	if (game->cameraMode == CameraMode_editor && game->editorCamera.showPivot)
	{
		m44 transform = transform_matrix(	game->editorCamera.pivotPosition,
											quaternion_identity,
											{0.1, 0.1, 0.1});

		// Note(Leo): these are random different color materials we had available at the time
		MaterialAssetId materialId = (game->editorCamera.usedControlMode == EditorCameraControlMode_translate) ?
										MaterialAssetId_raccoon :
										MaterialAssetId_character;

		graphics_draw_meshes(	graphics,
								1, &transform,
								assets_get_mesh(game->assets, MeshAssetId_default_cube),
								assets_get_material(game->assets, materialId));
	}

	// Castle
	{
		m44 transform 	= translation_matrix(game->castlePosition);

		graphics_draw_meshes(	graphics, 1, &transform,
								assets_get_mesh(game->assets, MeshAssetId_castle_main),
								assets_get_material(game->assets, MaterialAssetId_building_block));
	}

}