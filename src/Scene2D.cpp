/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 2d development scene
=============================================================================*/

// #include "DefaultSceneGui.cpp"

struct Scene2d
{
	Array<Transform3D> transformStorage;
	Array<Renderer> renderSystem;
	Array<Animator> animatorSystem;

	// Todo(Leo): make this similar 'system' to others
	CollisionManager2D collisionManager;

	Camera worldCamera;
	// CameraController3rdPerson cameraController;
	CameraControllerSideScroller cameraController;
	CharacterControllerSideScroller characterController;

	// Todo(Leo): make animation state controller or similar for these
	Animation laddersUpAnimation;
	Animation laddersDownAnimation;

	// Todo(Leo): make controller for these
	CharacterControllerSideScroller::LadderTriggerFunc ladderTrigger1;
	CharacterControllerSideScroller::LadderTriggerFunc ladderTrigger2;
	bool32 ladderOn = false;
	bool32 ladder2On = false;

	Gui 	gui;
	bool32 	guiVisible;
};

internal bool32 update_scene_2d(void * scenePtr, game::Input * input)
{
	Scene2d * scene = reinterpret_cast<Scene2d*>(scenePtr);

	scene->collisionManager.do_collisions();

	/// Update Character
	scene->characterController.update(input, &scene->collisionManager);
	update_animator_system(input, scene->animatorSystem);

	  //////////////////////////////
	 /// 	RENDERING 			///
	//////////////////////////////
	/*
		1. Update camera
		2. Render normal models
		3. Render animated models
	*/

	scene->cameraController.update(input);
    update_camera_system(&scene->worldCamera, input, platformGraphics, platformWindow);

	for (auto & renderer : scene->renderSystem)
	{
		platformApi->draw_model(platformGraphics, renderer.model,
								transform_matrix(*renderer.transform),
								renderer.castShadows,
								nullptr, 0);
	}

	Light light = { normalize_v3({1, 1, -3}), {0.95, 0.95, 0.9}};
	v3 ambient = {0.2, 0.25, 0.4};
	platformApi->update_lighting(platformGraphics, &light, &scene->worldCamera, ambient);



	bool32 keepScene = true;;

	if (is_clicked(input->start))
	{
		if(scene->guiVisible)
		{
			scene->guiVisible = false;
		}
		else
		{
			scene->guiVisible = true;
			scene->gui.selectedIndex = 0;
		}
	}

	if (scene->guiVisible)
	{
		gui_start(scene->gui, input);

		if (gui_button(v4{1,1,1,1}))
		{
			scene->guiVisible = false;
		}

		if (gui_button(v4{1,1,1,1}))
		{
			keepScene = false;
		}

		gui_end();
	}

	return keepScene;
}

internal void * load_scene_2d(MemoryArena & persistentMemory)
{
	void * scenePtr = allocate(persistentMemory, sizeof(Scene2d), true);
	Scene2d * scene = reinterpret_cast<Scene2d*>(scenePtr);

	// Note(Leo): lol this is so much better than others :D
	scene->gui = make_gui();

	scene->transformStorage = allocate_array<Transform3D>(persistentMemory, 100);
	scene->renderSystem 	= allocate_array<Renderer>(persistentMemory, 100);
	scene->animatorSystem 	= allocate_array<Animator>(persistentMemory, 100);


	auto allocate_transform = [](Array<Transform3D> & storage, Transform3D value) -> Transform3D *
	{
		u64 index = storage.count();
		storage.push(value);
		return &storage[index];
	};

	scene->collisionManager =
	{
		.colliders 	= allocate_array<Collider2D>(persistentMemory, 200),
		.collisions = allocate_array<Collision2D>(persistentMemory, 300, ALLOC_FILL | ALLOC_NO_CLEAR) // Todo(Leo): Not enough..
	};

	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	// Create MateriaLs
	{
		PipelineHandle shader = platformApi->push_pipeline(platformGraphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});

		TextureAsset whiteTextureAsset = make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xffffffff}), 1, 1);
		TextureAsset blackTextureAsset = make_texture_asset(allocate_array<u32>(*global_transientMemory, {0xff000000}), 1, 1);

		TextureHandle whiteTexture = platformApi->push_texture(platformGraphics, &whiteTextureAsset);
		TextureHandle blackTexture = platformApi->push_texture(platformGraphics, &blackTextureAsset);

		auto load_and_push_texture = [](const char * path) -> TextureHandle
		{
			auto asset = load_texture_asset(path, global_transientMemory);
			auto result = platformApi->push_texture(platformGraphics, &asset);
			return result;
		};

		auto tilesTexture 	= load_and_push_texture("assets/textures/tiles.jpg");
		auto lavaTexture 	= load_and_push_texture("assets/textures/lava.jpg");
		auto faceTexture 	= load_and_push_texture("assets/textures/texture.jpg");

		auto push_material = [shader](MaterialType type, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = make_material_asset(shader, allocate_array(*global_transientMemory, {a, b, c}));
			MaterialHandle handle = platformApi->push_material(platformGraphics, &asset);
			return handle;
		};

		materials =
		{
			.character 	= push_material(	MaterialType::Character,
											lavaTexture,
											faceTexture,
											blackTexture),

			.environment = push_material(	MaterialType::Character,
											tilesTexture,
											blackTexture,
											blackTexture)
		};
	}

    auto push_model = [] (MeshHandle mesh, MaterialHandle material) -> ModelHandle
    {
    	auto handle = platformApi->push_model(platformGraphics, mesh, material);
    	return handle;
    };

	// Characters
	Transform3D* characterTransform = {};
	{
		auto characterMesh 			= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/cube_head.glb"), "cube_head");
		auto characterMeshHandle 	= platformApi->push_mesh(platformGraphics, &characterMesh);

		// Our dude
		auto transform = allocate_transform(scene->transformStorage, {});
		auto model = push_model(characterMeshHandle, materials.character);

		characterTransform = transform;

		scene->renderSystem.push({transform, model});

		scene->characterController 	= 
		{
			.transform 	= transform,
			.collider 	= push_collider(&scene->collisionManager, transform, {0.4f, 0.8f}, {0.0f, 0.5f}),
		};


		scene->characterController.OnTriggerLadder1 = &scene->ladderTrigger1;
		scene->characterController.OnTriggerLadder2 = &scene->ladderTrigger2;

		// Other dude
		transform 	= allocate_transform(scene->transformStorage, {2, 0.5f, 12.25f});
		model 	= push_model(characterMeshHandle, materials.character);
		scene->renderSystem.push({transform, model});
	}

    scene->worldCamera = make_camera(45, 0.1f, 1000.0f);

    scene->cameraController =
    {
    	.camera 		= &scene->worldCamera,
    	.target 		= characterTransform
    };

	// Environment
	{
		constexpr float depth = 100;
		constexpr float width = 100;
		constexpr float ladderHeight = 1.0f;

		constexpr bool32 addPillars 	= true;
		constexpr bool32 addLadders 	= true;
		constexpr bool32 addButtons 	= true;
		constexpr bool32 addPlatforms 	= true;

		{
			auto groundQuad 	= mesh_primitives::create_quad(global_transientMemory, false);
			auto meshTransform	= translation_matrix({-width / 2, -depth /2, 0}) * scale_matrix({width, depth, 0});

			mesh_ops::transform(&groundQuad, meshTransform);
			mesh_ops::transform_tex_coords(&groundQuad, {0,0}, {width / 2, depth / 2});

			auto groundQuadHandle 	= platformApi->push_mesh(platformGraphics, &groundQuad);
			auto model 			= push_model(groundQuadHandle, materials.environment);
			auto transform 			= allocate_transform(scene->transformStorage, {});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {100, 1}, {0, -1.0f});
		}

		if (addPillars)
		{
			auto pillarMesh 		= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/big_pillar.glb"), "big_pillar");
			auto pillarMeshHandle 	= platformApi->push_mesh(platformGraphics, &pillarMesh);

			auto model 	= push_model(pillarMeshHandle, materials.environment);
			auto transform 	= allocate_transform(scene->transformStorage, {-width / 4, 0, 0});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {2, 25});

			model = push_model(pillarMeshHandle, materials.environment);
			transform = allocate_transform(scene->transformStorage, {width / 4, 0, 0});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {2, 25});
		}

		if (addLadders)
		{
			auto ladderMesh 		= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/ladder.glb"), "LadderSection");
			auto ladderMeshHandle 	= platformApi->push_mesh(platformGraphics, &ladderMesh);

			auto root1 	= allocate_transform(scene->transformStorage, {0, 0.5f, -ladderHeight});
			auto root2 	= allocate_transform(scene->transformStorage, {10, 0.5f, 6 - ladderHeight});
			auto bones1 = allocate_array<Transform3D*>(persistentMemory, 6);
			auto bones2 = allocate_array<Transform3D*>(persistentMemory, 6);

			int ladderRigBoneCount = 6;
			auto channels = allocate_array<TranslationChannel>(persistentMemory, ladderRigBoneCount);

			int ladder2StartIndex = 6;
			int ladderCount = 12;

			for (s32 ladderIndex = 0; ladderIndex < ladderCount; ++ladderIndex)
			{
				auto model 	= push_model(ladderMeshHandle, materials.environment);
				auto transform 	= allocate_transform(scene->transformStorage, {});

				scene->renderSystem.push({transform, model});
				push_collider(&scene->collisionManager, transform,
														{1.0f, 0.5f},
														{0, 0.5f},
														ColliderTag::Ladder);

				if (ladderIndex < ladder2StartIndex)
				{
					bones1.push(transform);
	

					// Todo(Leo): only one animation needed, move somewhere else				
					auto keyframeTimes = allocate_array<float>(persistentMemory, {ladderIndex * 0.12f, (ladderIndex + 1) * 0.15f});
					auto translations = allocate_array<v3>(persistentMemory, {{0,0,0}, {0,0,ladderHeight}});

					channels.push({ladderIndex, std::move(keyframeTimes),	std::move(translations)});
					channels.last().interpolationMode = INTERPOLATION_MODE_LINEAR;
					// channels.last().type = ANIMATION_CHANNEL_TRANSLATION;
				}
				else
				{
					bones2.push(transform);
				}
			}	

			scene->laddersUpAnimation 	= make_animation(std::move(channels), {});

			scene->laddersDownAnimation = copy_animation(persistentMemory, scene->laddersUpAnimation);
			reverse_animation_clip(scene->laddersDownAnimation);

			auto rig1 = make_animation_rig(root1, std::move(bones1));
			auto rig2 = make_animation_rig(root2, std::move(bones2));

			scene->animatorSystem.push(make_animator(std::move(rig1)));
			scene->animatorSystem.push(make_animator(std::move(rig2)));

			scene->ladderTrigger1 = [scene]() -> void
			{
				scene->ladderOn = !scene->ladderOn;
				if (scene->ladderOn)
					play_animation_clip(&scene->animatorSystem[0], &scene->laddersUpAnimation);
				else
					play_animation_clip(&scene->animatorSystem[0], &scene->laddersDownAnimation);
			};

			scene->ladderTrigger2 = [scene]() -> void
			{
				scene->ladder2On = !scene->ladder2On;
				if (scene->ladder2On)
					play_animation_clip(&scene->animatorSystem[1], &scene->laddersUpAnimation);
				else
					play_animation_clip(&scene->animatorSystem[1], &scene->laddersDownAnimation);
			};
		}

		logDebug(0) << "Ladders done";

		if (addPlatforms)
		{
			v3 platformPositions [] =
			{
				{-6, 0, 6},
				{-4, 0, 6},
				{-2, 0, 6},
	
				{2, 0, 6},
				{4, 0, 6},
				{6, 0, 6},
				{8, 0, 6},
				{10, 0, 6},

				{2, 0, 12},
				{4, 0, 12},
				{6, 0, 12},
				{8, 0, 12},
			};

			auto platformMeshAsset 	= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/platform.glb"), "platform");
			auto platformMeshHandle = platformApi->push_mesh(platformGraphics, &platformMeshAsset);

			int platformCount = 12;
			for (int platformIndex = 0; platformIndex < platformCount; ++platformIndex)
			{
				auto model 	= push_model(platformMeshHandle, materials.environment);
				auto transform 	= allocate_transform(scene->transformStorage, {platformPositions[platformIndex]});

				scene->renderSystem.push({transform, model});
				push_collider(&scene->collisionManager, transform, {1.0f, 0.25f});
			}
		}

		if(addButtons)
		{
			auto keyholeMeshAsset 	= load_mesh_glb(*global_transientMemory, read_gltf_file(*global_transientMemory, "assets/models/keyhole.glb"), "keyhole");
			auto keyholeMeshHandle 	= platformApi->push_mesh(platformGraphics, &keyholeMeshAsset);

			auto model 	= push_model(keyholeMeshHandle, materials.environment);
			auto transform 	= allocate_transform(scene->transformStorage, {.position = v3{5, 0, 0}, .rotation = quaternion::axis_angle(up_v3, to_radians(180))});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger);

			model 	= push_model(keyholeMeshHandle, materials.environment);
			transform 	= allocate_transform(scene->transformStorage, {.position = v3{4, 0, 6}, .rotation = quaternion::axis_angle(up_v3, to_radians(180))});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger2);
		}
	}

	logDebug(0) << "Loaded 2D scene";
	return scenePtr;
}
