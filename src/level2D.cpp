internal void
update_main_level(GameState * state, game::Input * input, game::RenderInfo * renderer, game::PlatformInfo * platform)
{
	state->collisionManager.do_collisions();

	/// Update Character
	state->characterController.Update(input, &state->collisionManager);
	update_animator_system(input, state->animatorSystem);

	state->cameraController.Update(input);
    update_camera_system(renderer, platform, input, &state->worldCamera);
	update_render_system(renderer, state->renderSystem);
}

internal void 
load_main_level(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	allocate_for_handle<Transform3D>(&state->persistentMemoryArena, 100);
	allocate_for_handle<Collider>(&state->persistentMemoryArena, 100);
	allocate_for_handle<Renderer>(&state->persistentMemoryArena, 100);
	allocate_for_handle<Animator>(&state->persistentMemoryArena, 100);

	state->renderSystem = reserve_array<RenderSystemEntry>(&state->persistentMemoryArena, 100);
	state->animatorSystem = reserve_array<Handle<Animator>>(&state->persistentMemoryArena, 100);

	state->collisionManager =
	{
		.colliders 	= reserve_array<Handle<Collider>>(&state->persistentMemoryArena, 200),
		.collisions = reserve_array<Collision>(&state->persistentMemoryArena, 300) // Todo(Leo): Not enough..
	};

	// Create MateriaLs
	{
		TextureAsset whiteTextureAsset = make_texture_asset(push_array<uint32>(&state->transientMemoryArena, {0xffffffff}), 1, 1);
		TextureAsset blackTextureAsset = make_texture_asset(push_array<uint32>(&state->transientMemoryArena, {0xff000000}), 1, 1);

		TextureHandle whiteTexture = platformInfo->graphicsContext->PushTexture(&whiteTextureAsset);
		TextureHandle blackTexture = platformInfo->graphicsContext->PushTexture(&blackTextureAsset);

		auto load_and_push_texture = [state, platformInfo](const char * path) -> TextureHandle
		{
			auto asset = load_texture_asset(path, &state->transientMemoryArena);
			auto result = platformInfo->graphicsContext->PushTexture(&asset);
			return result;
		};

		auto tilesTexture 	= load_and_push_texture("textures/tiles.jpg");
		auto lavaTexture 	= load_and_push_texture("textures/lava.jpg");
		auto faceTexture 	= load_and_push_texture("textures/texture.jpg");

		auto push_material = [platformInfo](MaterialType type, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = { type, a, b, c };
			MaterialHandle handle = platformInfo->graphicsContext->PushMaterial(&asset);
			return handle;
		};

		state->materials =
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

    auto push_mesh = [platformInfo] (MeshAsset * asset) -> MeshHandle
    {
    	auto handle = platformInfo->graphicsContext->PushMesh(asset);
    	return handle;
    };

    auto push_renderer = [platformInfo] (MeshHandle mesh, MaterialHandle material) -> RenderedObjectHandle
    {
    	auto handle = platformInfo->graphicsContext->PushRenderedObject(mesh, material);
    	return handle;
    };

	// Characters
	Handle<Transform3D> characterTransform = {};
	{
		auto characterMesh 			= load_model_obj(&state->transientMemoryArena, "models/character.obj");
		auto characterMeshHandle 	= push_mesh(&characterMesh);

		// Our dude
		auto transform = make_handle<Transform3D>({});
		auto renderer = make_handle<Renderer>({push_renderer(characterMeshHandle, state->materials.character)});

		characterTransform = transform;

		push_one(state->renderSystem, {transform, renderer});

		state->characterController 	= 
		{
			.transform 	= transform,
			.collider 	= push_collider(&state->collisionManager, transform, {0.4f, 0.8f}, {0.0f, 0.5f}),
		};


		state->characterController.OnTriggerLadder1 = &state->ladderTrigger1;
		state->characterController.OnTriggerLadder2 = &state->ladderTrigger2;

		// Other dude
		transform 	= make_handle<Transform3D>({2, 0.5f, 12.25f});
		renderer 	= make_handle<Renderer>({push_renderer(characterMeshHandle, state->materials.character)});
		push_one(state->renderSystem, {transform, renderer});
	}

    state->worldCamera =
    {
    	.forward 		= World::Forward,
    	.fieldOfView 	= 60,
    	.nearClipPlane 	= 0.1f,
    	.farClipPlane 	= 1000.0f,
    	.aspectRatio 	= (float)platformInfo->windowWidth / (float)platformInfo->windowHeight	
    };

    state->cameraController =
    {
    	.camera 		= &state->worldCamera,
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
			auto groundQuad 	= mesh_primitives::create_quad(&state->transientMemoryArena);
			auto meshTransform	= Matrix44::Translate({-width / 2, -depth /2, 0}) * Matrix44::Scale({width, depth, 0});
			mesh_ops::transform(&groundQuad, meshTransform);
			mesh_ops::transform_tex_coords(&groundQuad, {0,0}, {width / 2, depth / 2});

			auto groundQuadHandle 	= push_mesh(&groundQuad);
			auto renderer 			= make_handle<Renderer>({push_renderer(groundQuadHandle, state->materials.environment)});
			auto transform 			= make_handle<Transform3D>({});

			push_one(state->renderSystem, {transform, renderer});
			push_collider(&state->collisionManager, transform, {100, 1}, {0, -1.0f});
		}

		if (addPillars)
		{
			auto pillarMesh 		= load_model_glb(&state->transientMemoryArena, "models/big_pillar.glb", "big_pillar");
			auto pillarMeshHandle 	= push_mesh(&pillarMesh);

			auto renderer 	= make_handle<Renderer>({push_renderer(pillarMeshHandle, state->materials.environment)});
			auto transform 	= make_handle<Transform3D>({-width / 4, 0, 0});

			push_one(state->renderSystem, {transform, renderer});
			push_collider(&state->collisionManager, transform, {2, 25});

			renderer = make_handle<Renderer>({push_renderer(pillarMeshHandle, state->materials.environment)});
			transform = make_handle<Transform3D>({width / 4, 0, 0});

			push_one(state->renderSystem, {transform, renderer});
			push_collider(&state->collisionManager, transform, {2, 25});
		}

		if (addLadders)
		{
			auto ladderMesh 		= load_model_glb(&state->transientMemoryArena, "models/ladder.glb", "LadderSection");
			auto ladderMeshHandle 	= push_mesh(&ladderMesh);

			auto root1 	= make_handle<Transform3D>({0, 0.5f, -ladderHeight});
			auto root2 	= make_handle<Transform3D>({10, 0.5f, 6 - ladderHeight});
			auto bones1 = reserve_array<Handle<Transform3D>>(&state->persistentMemoryArena, 6);
			auto bones2 = reserve_array<Handle<Transform3D>>(&state->persistentMemoryArena, 6);

			int ladderRigBoneCount = 6;
			auto animations = reserve_array<Animation>(&state->persistentMemoryArena, ladderRigBoneCount);

			auto parent1 = root1;
			auto parent2 = root2;
			
			int ladder2StartIndex = 6;
			int ladderCount = 12;
			for (int ladderIndex = 0; ladderIndex < ladderCount; ++ladderIndex)
			{
				auto renderer 	= make_handle<Renderer>({push_renderer(ladderMeshHandle, state->materials.environment)});
				auto transform 	= make_handle<Transform3D>({});

				push_one(state->renderSystem, {transform, renderer});
				push_collider(&state->collisionManager, transform,
														{1.0f, 0.5f},
														{0, 0.5f},
														ColliderTag::Ladder);

				if (ladderIndex < ladder2StartIndex)
				{
					transform->parent = parent1;
					parent1 = transform;
					push_one(bones1, transform);
	

					// Todo(Leo): only one animation needed, move somewhere else				
					auto keyframes = push_array(&state->persistentMemoryArena, {
						Keyframe{(ladderIndex % ladder2StartIndex) * 0.12f, {0, 0, 0}},
						Keyframe{((ladderIndex % ladder2StartIndex) + 1) * 0.15f, {0, 0, ladderHeight}}
					});
					push_one(animations, {keyframes});
				}
				else
				{
					transform->parent = parent2;
					parent2 = transform;	
					push_one(bones2, transform);
				}
			}	

			state->laddersUpAnimation 	= make_animation_clip(animations);
			state->laddersDownAnimation = duplicate_animation_clip(&state->persistentMemoryArena, &state->laddersUpAnimation);
			reverse_animation_clip(&state->laddersDownAnimation);

			auto keyframeCounters1 = push_array<uint64>(&state->persistentMemoryArena, bones1.count());
			auto keyframeCounters2 = push_array<uint64>(&state->persistentMemoryArena, bones2.count());

			auto rig1 = make_animation_rig(root1, bones1, keyframeCounters1);
			auto rig2 = make_animation_rig(root2, bones2, keyframeCounters2);

			auto animator1 = make_animator(rig1);
			auto animator2 = make_animator(rig2);

			push_one(state->animatorSystem, make_handle(animator1));
			push_one(state->animatorSystem, make_handle(animator2));

			state->ladderTrigger1 = [state]() -> void
			{
				state->ladderOn = !state->ladderOn;
				if (state->ladderOn)
					play_animation_clip(state->animatorSystem[0], &state->laddersUpAnimation);
				else
					play_animation_clip(state->animatorSystem[0], &state->laddersDownAnimation);
			};

			state->ladderTrigger2 = [state]() -> void
			{
				state->ladder2On = !state->ladder2On;
				if (state->ladder2On)
					play_animation_clip(state->animatorSystem[1], &state->laddersUpAnimation);
				else
					play_animation_clip(state->animatorSystem[1], &state->laddersDownAnimation);
			};
		}

		if (addPlatforms)
		{
			Vector3 platformPositions [] =
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

			auto platformMeshAsset 	= load_model_obj(&state->transientMemoryArena, "models/platform.obj");
			auto platformMeshHandle = push_mesh(&platformMeshAsset);

			int platformCount = 12;
			for (int platformIndex = 0; platformIndex < platformCount; ++platformIndex)
			{
				auto renderer 	= make_handle<Renderer>({push_renderer(platformMeshHandle, state->materials.environment)});
				auto transform 	= make_handle<Transform3D>({platformPositions[platformIndex]});

				push_one(state->renderSystem, {transform, renderer});
				push_collider(&state->collisionManager, transform, {1.0f, 0.25f});
			}
		}

		if(addButtons)
		{
			auto keyholeMeshAsset 	= load_model_obj(&state->transientMemoryArena, "models/keyhole.obj");
			auto keyholeMeshHandle 	= push_mesh (&keyholeMeshAsset);

			auto renderer 	= make_handle<Renderer>({push_renderer(keyholeMeshHandle, state->materials.environment)});
			auto transform 	= make_handle<Transform3D>({Vector3{5, 0, 0}});

			push_one(state->renderSystem, {transform, renderer});
			push_collider(&state->collisionManager, transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger);

			renderer 	= make_handle<Renderer>({push_renderer(keyholeMeshHandle, state->materials.environment)});
			transform 	= make_handle<Transform3D>({Vector3{4, 0, 6}});

			push_one(state->renderSystem, {transform, renderer});
			push_collider(&state->collisionManager, transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger2);
		}
	}

	state->gameGuiButtonCount 	= 2;
	state->gameGuiButtons 		= push_array<Rectangle>(&state->persistentMemoryArena, state->gameGuiButtonCount);
	state->gameGuiButtonHandles = push_array<GuiHandle>(&state->persistentMemoryArena, state->gameGuiButtonCount);

	state->gameGuiButtons[0] = {380, 200, 180, 40};
	state->gameGuiButtons[1] = {380, 260, 180, 40};

	MeshAsset quadAsset 	= mesh_primitives::create_quad(&state->transientMemoryArena);
 	MeshHandle quadHandle 	= push_mesh(&quadAsset);

	for (int guiButtonIndex = 0; guiButtonIndex < state->gameGuiButtonCount; ++guiButtonIndex)
	{
		state->gameGuiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, state->materials.character);
	}

	state->selectedGuiButtonIndex = 0;
	state->showGameMenu = false;

 	// Note(Leo): Apply all pushed changes and flush transient memory
    platformInfo->graphicsContext->Apply();
    flush_memory_arena(&state->transientMemoryArena);
}