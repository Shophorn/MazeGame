/*=============================================================================
Leo Tamminen
shophorn @ internet

Scene description for 2d development scene
=============================================================================*/

// #include "DefaultSceneGui.cpp"

namespace scene_2d
{
	struct Scene
	{
		Array<Transform3D> transformStorage;
		Array<RenderSystemEntry> renderSystem;
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

		SceneGui gui;
	};

	internal u64
	get_alloc_size() { return sizeof(Scene); };

	internal MenuResult
	update(	void * 						scenePtr,
			game::Input * 				input,
			platform::Graphics*,
			platform::Window*,
			platform::Functions*);

	internal void
	load(	void * 						scenePtr,
			MemoryArena * 				persistentMemory,
			MemoryArena * 				transientMemory,
			platform::Graphics*,
			platform::Window*,
			platform::Functions*);
}

SceneInfo get_2d_scene_info()
{
	return make_scene_info(	scene_2d::get_alloc_size,
							scene_2d::load,
							scene_2d::update);
}

internal MenuResult
scene_2d::update(	void * scenePtr,
					game::Input * input,
					platform::Graphics * graphics,
					platform::Window * window,
					platform::Functions * functions)
{
	Scene * scene = reinterpret_cast<Scene*>(scenePtr);

	scene->collisionManager.do_collisions();

	/// Update Character
	scene->characterController.update(input, &scene->collisionManager);
	update_animator_system(input, scene->animatorSystem);

	scene->cameraController.update(input);
    update_camera_system(&scene->worldCamera, input, graphics, window, functions);
	update_render_system(scene->renderSystem, graphics, functions);

	Light light = { vector::normalize(v3{1, 1, -3}), {0.95, 0.95, 0.9}};
	v3 ambient = {0.2, 0.25, 0.4};
	functions->update_lighting(graphics, &light, &scene->worldCamera, ambient);


	auto result = update_scene_gui(&scene->gui, input, graphics, functions);
	return result;
}

internal void 
scene_2d::load(	void * scenePtr,
				MemoryArena * persistentMemory,
				MemoryArena * transientMemory,
				platform::Graphics * graphics,
				platform::Window * platform,
				platform::Functions * functions)
{
	Scene * scene = reinterpret_cast<Scene*>(scenePtr);

	// Note(Leo): lol this is so much better than others :D
	scene->gui = make_scene_gui(transientMemory, graphics, functions);

	// auto * persistentMemory = &state->persistentMemoryArena;
	// auto * transientMemory = &state->transientMemoryArena;

	scene->transformStorage = allocate_array<Transform3D>(*persistentMemory, 100);
	scene->renderSystem 	= allocate_array<RenderSystemEntry>(*persistentMemory, 100);
	scene->animatorSystem 	= allocate_array<Animator>(*persistentMemory, 100);


	auto allocate_transform = [](Array<Transform3D> & storage, Transform3D value) -> Transform3D *
	{
		u64 index = storage.count();
		storage.push(value);
		return &storage[index];
	};

	scene->collisionManager =
	{
		.colliders 	= allocate_array<Collider2D>(*persistentMemory, 200),
		.collisions = allocate_array<Collision2D>(*persistentMemory, 300, ALLOC_FILL_UNINITIALIZED) // Todo(Leo): Not enough..
	};

	struct MaterialCollection {
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	// Create MateriaLs
	{
		PipelineHandle shader = functions->push_pipeline(graphics, "assets/shaders/vert.spv", "assets/shaders/frag.spv", {.textureCount = 3});

		TextureAsset whiteTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xffffffff}), 1, 1);
		TextureAsset blackTextureAsset = make_texture_asset(allocate_array<u32>(*transientMemory, {0xff000000}), 1, 1);

		TextureHandle whiteTexture = functions->push_texture(graphics, &whiteTextureAsset);
		TextureHandle blackTexture = functions->push_texture(graphics, &blackTextureAsset);

		auto load_and_push_texture = [transientMemory, functions, graphics](const char * path) -> TextureHandle
		{
			auto asset = load_texture_asset(path, transientMemory);
			auto result = functions->push_texture(graphics, &asset);
			return result;
		};

		auto tilesTexture 	= load_and_push_texture("assets/textures/tiles.jpg");
		auto lavaTexture 	= load_and_push_texture("assets/textures/lava.jpg");
		auto faceTexture 	= load_and_push_texture("assets/textures/texture.jpg");

		auto push_material = [shader, functions, transientMemory, graphics](MaterialType type, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = make_material_asset(shader, allocate_array(*transientMemory, {a, b, c}));
			MaterialHandle handle = functions->push_material(graphics, &asset);
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

    auto push_model = [functions, graphics] (MeshHandle mesh, MaterialHandle material) -> ModelHandle
    {
    	auto handle = functions->push_model(graphics, mesh, material);
    	return handle;
    };

	// Characters
	Transform3D* characterTransform = {};
	{
		auto characterMesh 			= load_model_obj(transientMemory, "assets/models/character.obj");
		auto characterMeshHandle 	= functions->push_mesh(graphics, &characterMesh);

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
			auto groundQuad 	= mesh_primitives::create_quad(transientMemory, false);
			auto meshTransform	= make_translation_matrix({-width / 2, -depth /2, 0}) * make_scale_matrix({width, depth, 0});

			mesh_ops::transform(&groundQuad, meshTransform);
			mesh_ops::transform_tex_coords(&groundQuad, {0,0}, {width / 2, depth / 2});

			auto groundQuadHandle 	= functions->push_mesh(graphics, &groundQuad);
			auto model 			= push_model(groundQuadHandle, materials.environment);
			auto transform 			= allocate_transform(scene->transformStorage, {});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {100, 1}, {0, -1.0f});
		}

		if (addPillars)
		{
			auto pillarMesh 		= load_model_glb(*transientMemory, read_gltf_file(*transientMemory, "assets/models/big_pillar.glb"), "big_pillar");
			auto pillarMeshHandle 	= functions->push_mesh(graphics, &pillarMesh);

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
			auto ladderMesh 		= load_model_glb(*transientMemory, read_gltf_file(*transientMemory, "assets/models/ladder.glb"), "LadderSection");
			auto ladderMeshHandle 	= functions->push_mesh(graphics, &ladderMesh);

			auto root1 	= allocate_transform(scene->transformStorage, {0, 0.5f, -ladderHeight});
			auto root2 	= allocate_transform(scene->transformStorage, {10, 0.5f, 6 - ladderHeight});
			auto bones1 = allocate_array<Transform3D*>(*persistentMemory, 6);
			auto bones2 = allocate_array<Transform3D*>(*persistentMemory, 6);

			int ladderRigBoneCount = 6;
			auto animations = allocate_array<BoneAnimation>(*persistentMemory, ladderRigBoneCount, ALLOC_EMPTY);

			auto parent1 = root1;
			auto parent2 = root2;
			
			int ladder2StartIndex = 6;
			int ladderCount = 12;
			for (u32 ladderIndex = 0; ladderIndex < ladderCount; ++ladderIndex)
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
					transform->parent = parent1;
					parent1 = transform;
					bones1.push(transform);
	
					// Todo(Leo): only one animation needed, move somewhere else				
					auto keyframes = allocate_array(*persistentMemory, {
						PositionKeyframe{(ladderIndex % ladder2StartIndex) * 0.12f, {0, 0, 0}},
						PositionKeyframe{((ladderIndex % ladder2StartIndex) + 1) * 0.15f, {0, 0, ladderHeight}}
					});
					animations.push({ladderIndex, std::move(keyframes)});
				}
				else
				{
					transform->parent = parent2;
					parent2 = transform;	
					bones2.push(transform);
				}
			}	

			scene->laddersUpAnimation 	= make_animation(std::move(animations));

			scene->laddersDownAnimation = copy_animation(*persistentMemory, scene->laddersUpAnimation);
			reverse_animation_clip(&scene->laddersDownAnimation);

			auto keyframeCounters1 = allocate_array<u64>(*persistentMemory, bones1.count(), ALLOC_FILL_UNINITIALIZED);
			auto keyframeCounters2 = allocate_array<u64>(*persistentMemory, bones2.count(), ALLOC_FILL_UNINITIALIZED);

			std::cout << "bones: " << bones1.count() << ", keyframes: " << keyframeCounters1.count() << "\n"; 

			auto rig1 = make_animation_rig(root1, std::move(bones1), std::move(keyframeCounters1));
			auto rig2 = make_animation_rig(root2, std::move(bones2), std::move(keyframeCounters2));

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

			auto platformMeshAsset 	= load_model_obj(transientMemory, "assets/models/platform.obj");
			auto platformMeshHandle = functions->push_mesh(graphics, &platformMeshAsset);

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
			auto keyholeMeshAsset 	= load_model_obj(transientMemory, "assets/models/keyhole.obj");
			auto keyholeMeshHandle 	= functions->push_mesh(graphics, &keyholeMeshAsset);

			auto model 	= push_model(keyholeMeshHandle, materials.environment);
			auto transform 	= allocate_transform(scene->transformStorage, {v3{5, 0, 0}});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger);

			model 	= push_model(keyholeMeshHandle, materials.environment);
			transform 	= allocate_transform(scene->transformStorage, {v3{4, 0, 6}});

			scene->renderSystem.push({transform, model});
			push_collider(&scene->collisionManager, transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger2);
		}
	}
}
