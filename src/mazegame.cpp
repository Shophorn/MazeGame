/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

// Note(Leo): Make unity build here.
#include "Handle.cpp"
#include "Random.cpp"
#include "MapGenerator.cpp"
#include "Transform3D.cpp"
#include "Animator.cpp"
#include "Camera.cpp"
#include "Collisions.cpp"
#include "CollisionManager.cpp"
#include "CharacterSystems.cpp"
#include "CameraController.cpp"

/// Note(Leo): These still use external libraries we may want to get rid of
#include "Files.cpp"
#include "AudioFile.cpp"
#include "MeshLoader.cpp"
#include "TextureLoader.cpp"

// Todo(Leo): this is stupid and redundant, but let's go with it for a while
struct Renderer
{
	RenderedObjectHandle handle;
};

struct RenderSystemEntry
{
	Handle<Transform3D> transform;
	Handle<Renderer> renderer;
};

internal void
update_render_system(game::RenderInfo * renderer, ArenaArray<RenderSystemEntry> entries)
{
	for (int i = 0; i < entries.count(); ++i)
	{
		renderer->render(	entries[i].renderer->handle,
							entries[i].transform->get_matrix());
	}
}

internal void
update_camera_system(game::RenderInfo * renderer, game::PlatformInfo * platform, game::Input * input, Camera * camera)
{
	// Note(Leo): Update aspect ratio each frame, in case screen size has changed.
    camera->aspectRatio = (real32)platform->windowWidth / (real32)platform->windowHeight;

	// Ccamera
    renderer->set_camera(	camera->ViewProjection(),
							camera->PerspectiveProjection());
}

struct GameState
{
	ArenaArray<RenderSystemEntry> renderSystem;
	ArenaArray<Handle<Animator>> animatorSystem;

	// Todo(Leo): make this similar 'system' to others
	CollisionManager collisionManager;


	Camera worldCamera;
	// CameraController3rdPerson cameraController;
	CameraControllerSideScroller cameraController;
	CharacterControllerSideScroller characterController;

	// Todo(Leo): make animation state controller or similar for these
	AnimationClip 	laddersUpAnimation;
	AnimationClip 	laddersDownAnimation;

	// Todo(Leo): make controller for these
	CharacterControllerSideScroller::LadderTriggerFunc ladderTrigger1;
	CharacterControllerSideScroller::LadderTriggerFunc ladderTrigger2;
	bool32 ladderOn = false;
	bool32 ladder2On = false;

	struct
	{
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	/* Note(Leo): MEMORY
	'persistentMemoryArena' is used to store things from frame to frame.
	'transientMemoryArena' is intended to be used in asset loading etc. and is flushed each frame*/
	MemoryArena persistentMemoryArena;
	MemoryArena transientMemoryArena;

	int32 gameGuiButtonCount;
	ArenaArray<Rectangle> gameGuiButtons;
	ArenaArray<GuiHandle> gameGuiButtonHandles;
	bool32 showGameMenu;

	int32 menuGuiButtonCount;
	ArenaArray<Rectangle> menuGuiButtons;
	ArenaArray<GuiHandle> menuGuiButtonHandles;

	int32 selectedGuiButtonIndex;

	bool32 levelLoaded = false;
};

internal void
initialize_game_state(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	*state = {};

	// Note(Leo): Create persistent arena in the same memoryblock as game state, right after it.
	byte * persistentMemoryAddress 	= reinterpret_cast<byte*>(memory->persistentMemory) + sizeof(GameState);
	uint64 persistentMemorySize 	= memory->persistentMemorySize - sizeof(GameState);
	state->persistentMemoryArena 	= make_memory_arena(persistentMemoryAddress, persistentMemorySize);

	byte * transientMemoryAddress 	= reinterpret_cast<byte*>(memory->transientMemory);
	uint64 transientMemorySize 		= memory->transientMemorySize;
	state->transientMemoryArena 	= make_memory_arena(transientMemoryAddress, transientMemorySize);

	std::cout << "Screen size = (" << platformInfo->windowWidth << "," << platformInfo->windowHeight << ")\n";
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

internal void
load_menu(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	// Create MateriaLs
	{
		TextureAsset whiteTextureAsset = {};
		whiteTextureAsset.pixels = push_array<uint32>(&state->transientMemoryArena, 1);
		whiteTextureAsset.pixels[0] = 0xffffffff;
		whiteTextureAsset.width = 1;
		whiteTextureAsset.height = 1;
		whiteTextureAsset.channels = 4;

		TextureAsset blackTextureAsset = {};
		blackTextureAsset.pixels = push_array<uint32>(&state->transientMemoryArena, 1);
		blackTextureAsset.pixels[0] = 0xff000000;
		blackTextureAsset.width = 1;
		blackTextureAsset.height = 1;
		blackTextureAsset.channels = 4;

		TextureHandle whiteTexture = platformInfo->graphicsContext->PushTexture(&whiteTextureAsset);
		TextureHandle blackTexture = platformInfo->graphicsContext->PushTexture(&blackTextureAsset);

	    TextureAsset textureAssets [] = {
	        load_texture_asset("textures/lava.jpg", &state->transientMemoryArena),
	        load_texture_asset("textures/texture.jpg", &state->transientMemoryArena),
	    };

		TextureHandle texB = platformInfo->graphicsContext->PushTexture(&textureAssets[0]);
		TextureHandle texC = platformInfo->graphicsContext->PushTexture(&textureAssets[1]);

		MaterialAsset characterMaterialAsset = {MaterialType::Character, texB, texC, blackTexture};
		state->materials.character = platformInfo->graphicsContext->PushMaterial(&characterMaterialAsset);
	}

	state->menuGuiButtonCount = 4;
	state->menuGuiButtons = push_array<Rectangle>(&state->persistentMemoryArena, state->menuGuiButtonCount);
	state->menuGuiButtonHandles = push_array<GuiHandle>(&state->persistentMemoryArena, state->menuGuiButtonCount);

	state->menuGuiButtons[0] = {380, 200, 180, 40};
	state->menuGuiButtons[1] = {380, 260, 180, 40};
	state->menuGuiButtons[2] = {380, 320, 180, 40};
	state->menuGuiButtons[3] = {380, 380, 180, 40};

	MeshAsset quadAsset = mesh_primitives::create_quad(&state->transientMemoryArena);
 	MeshHandle quadHandle = platformInfo->graphicsContext->PushMesh(&quadAsset);

	for (int guiButtonIndex = 0; guiButtonIndex < state->menuGuiButtonCount; ++guiButtonIndex)
	{
		state->menuGuiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, state->materials.character);
	}

	state->selectedGuiButtonIndex = 0;

 	// Note(Leo): Apply all pushed changes and flush transient memory
    platformInfo->graphicsContext->Apply();
    flush_memory_arena(&state->transientMemoryArena);
}

internal void
output_sound(int frameCount, game::StereoSoundSample * samples)
{
	// Note(Leo): Shit these are bad :DD
	local_persist int runningSampleIndex = 0;
	local_persist AudioFile<float> file;
	local_persist bool32 fileLoaded = false;
	local_persist int32 fileSampleCount;
	local_persist int32 fileSampleIndex;

	if (fileLoaded == false)
	{
		fileLoaded = true;
		
		file.load("sounds/Wind-Mark_DiAngelo-1940285615.wav");
		fileSampleCount = file.getNumSamplesPerChannel();
	}

	// Todo(Leo): get volume from some input structure
	real32 volume = 0.5f;

	for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
	{
		samples[sampleIndex].left = file.samples[0][fileSampleIndex] * volume;
		samples[sampleIndex].right = file.samples[1][fileSampleIndex] * volume;

		fileSampleIndex += 1;
		fileSampleIndex %= fileSampleCount;
	}
}

enum MenuResult { MENU_NONE, MENU_EXIT, MENU_LOADLEVEL, MENU_CONTINUE };

internal MenuResult 
UpdateMainLevel(
	GameState *				state,
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platform,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,
	game::RenderInfo * 		outRenderInfo);

internal MenuResult 
UpdateMenu(
	GameState *				state,
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platform,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,
	game::RenderInfo * 		outRenderInfo);

extern "C" game::UpdateResult
GameUpdate(
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platform,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,
	game::RenderInfo * 		outRenderInfo
){
	GameState * state = reinterpret_cast<GameState*>(memory->persistentMemory);

	if (memory->isInitialized == false)
	{
		initialize_game_state (state, memory, platform);
		memory->isInitialized = true;

		load_menu(state, memory, platform);
	}

	game::UpdateResult result = {};
	result.exit = false;
	if (state->levelLoaded == false)
	{
		MenuResult menuResult = UpdateMenu(state, input, memory, platform, network, soundOutput, outRenderInfo);
		if (menuResult == MENU_EXIT)
		{
			result = { true };
		}
		else if (menuResult == MENU_LOADLEVEL)
		{
			platform->graphicsContext->UnloadAll();
			flush_memory_arena(&state->persistentMemoryArena);

			load_main_level(state, memory, platform);
			state->levelLoaded = true;
		}
	}
	else
	{
		MenuResult result = UpdateMainLevel(state, input, memory, platform, network, soundOutput, outRenderInfo);
		if (result == MENU_EXIT)
		{
			platform->graphicsContext->UnloadAll();
			clear_memory_arena(&state->persistentMemoryArena);

			load_menu(state, memory, platform);
			state->levelLoaded = false;	
		}
	}
	
	if (input->select.IsClicked())
	{
		if (platform->windowIsFullscreen)
			result.setWindow = game::UpdateResult::SET_WINDOW_WINDOWED;
		else
			result.setWindow = game::UpdateResult::SET_WINDOW_FULLSCREEN;
	}

	/// Output sound
	{
		output_sound(soundOutput->sampleCount, soundOutput->samples);
	}

	return result;
}

internal MenuResult 
UpdateMenu(
	GameState *				state,
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platform,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,
	game::RenderInfo * 		outRenderInfo)
{
	// Input
	MenuResult result = MENU_NONE;
	{
		if (input->down.IsClicked())
		{
			state->selectedGuiButtonIndex += 1;
			state->selectedGuiButtonIndex %= state->menuGuiButtonCount;
		}

		if (input->up.IsClicked())
		{
			state->selectedGuiButtonIndex -= 1;
			if (state->selectedGuiButtonIndex < 0)
			{
				state->selectedGuiButtonIndex = state->menuGuiButtonCount - 1;
			}
		}

		if (input->confirm.IsClicked())
		{
			switch (state->selectedGuiButtonIndex)
			{
				case 0: 
					std::cout << "Start game\n";
					result = MENU_LOADLEVEL;
					break;

				case 1: std::cout << "Options\n"; break;
				case 2: std::cout << "Credits\n"; break;
				
				case 3:
					std::cout << "Exit\n";
					result = MENU_EXIT;
					break;
			}
		}
	}

	/// Output Render info
	{
		Vector2 windowSize = {(real32)platform->windowWidth, (real32)platform->windowHeight};
		Vector2 halfWindowSize = windowSize / 2.0f;

		for (int  guiButtonIndex = 0; guiButtonIndex < state->menuGuiButtonCount; ++guiButtonIndex)
		{
			Vector2 guiTranslate = state->menuGuiButtons[guiButtonIndex].position / halfWindowSize - 1.0f;
			Vector2 guiScale = state->menuGuiButtons[guiButtonIndex].size / halfWindowSize;

			if (guiButtonIndex == state->selectedGuiButtonIndex)
			{
				guiTranslate = (state->menuGuiButtons[guiButtonIndex].position - Vector2{15, 15}) / halfWindowSize - 1.0f;
				guiScale = (state->menuGuiButtons[guiButtonIndex].size + Vector2{30, 30}) / halfWindowSize;
			}

			Matrix44 guiTransform = Matrix44::Translate({guiTranslate.x, guiTranslate.y, 0}) * Matrix44::Scale({guiScale.x, guiScale.y, 1.0});

			outRenderInfo->render_gui(state->menuGuiButtonHandles[guiButtonIndex], guiTransform);
			// outRenderInfo->guiObjects[state->menuGuiButtonHandles[guiButtonIndex]] = guiTransform;
		}

		// Ccamera
	    outRenderInfo->set_camera(	state->worldCamera.ViewProjection(),
	    							state->worldCamera.PerspectiveProjection());
	}

	return result;
}

internal MenuResult 
UpdateMainLevel(
	GameState *				state,
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platform,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,
	game::RenderInfo * 		outRenderInfo)
{
	flush_memory_arena(&state->transientMemoryArena);

	state->collisionManager.do_collisions();

	/// Update Character
	state->characterController.Update(input, &state->collisionManager);
	update_animator_system(input, state->animatorSystem);

	state->cameraController.Update(input);
    update_camera_system(outRenderInfo, platform, input, &state->worldCamera);
	update_render_system(outRenderInfo, state->renderSystem);

	// Note(Leo): This is just a reminder
	// Todo(Leo): Remove if unnecessary
	/// Update network
	{
		// network->outPackage = {};
		// network->outPackage.characterPosition = state->character.position;
		// network->outPackage.characterRotation = state->character.rotation;
	}


	// Overlay menu
	if (input->start.IsClicked())
	{
		state->selectedGuiButtonIndex = 0;
		state->showGameMenu = !state->showGameMenu;
	}

	MenuResult gameMenuResult = MENU_NONE;
	if (state->showGameMenu)
	{
		if (input->down.IsClicked())
		{
			state->selectedGuiButtonIndex += 1;
			state->selectedGuiButtonIndex %= state->gameGuiButtonCount;
		}

		if (input->up.IsClicked())
		{
			state->selectedGuiButtonIndex -= 1;
			if (state->selectedGuiButtonIndex < 0)
			{
				state->selectedGuiButtonIndex = state->gameGuiButtonCount - 1;
			}
		}

		if (input->confirm.IsClicked())
		{
			switch (state->selectedGuiButtonIndex)
			{
				case 0: 
					std::cout << "Continue Game\n";
					gameMenuResult = MENU_CONTINUE;
					break;
				
				case 1:
					std::cout << "Quit to menu\n";
					gameMenuResult = MENU_EXIT;
					break;
			}
		}
	}
	if (gameMenuResult == MENU_CONTINUE)
	{
		state->showGameMenu = false;
	}

	/// Output Render info
	// Todo(Leo): Get info about limits of render output array sizes and constraint to those
	{

		if (state->showGameMenu)
		{
			/// ----- GUI -----
			Vector2 windowSize = {(real32)platform->windowWidth, (real32)platform->windowHeight};
			Vector2 halfWindowSize = windowSize / 2.0f;

			for (int  guiButtonIndex = 0; guiButtonIndex < state->gameGuiButtonCount; ++guiButtonIndex)
			{
				Vector2 guiTranslate = state->gameGuiButtons[guiButtonIndex].position / halfWindowSize - 1.0f;
				Vector2 guiScale = state->gameGuiButtons[guiButtonIndex].size / halfWindowSize;

				if (guiButtonIndex == state->selectedGuiButtonIndex)
				{
					guiTranslate = (state->gameGuiButtons[guiButtonIndex].position - Vector2{15, 15}) / halfWindowSize - 1.0f;
					guiScale = (state->gameGuiButtons[guiButtonIndex].size + Vector2{30, 30}) / halfWindowSize;
				}

				Matrix44 guiTransform = Matrix44::Translate({guiTranslate.x, guiTranslate.y, 0}) * Matrix44::Scale({guiScale.x, guiScale.y, 1.0});

				outRenderInfo->render_gui(state->gameGuiButtonHandles[guiButtonIndex], guiTransform);
			}
		}
		else
		{
			for (int  guiButtonIndex = 0; guiButtonIndex < state->gameGuiButtonCount; ++guiButtonIndex)
			{
				Matrix44 guiTransform = {};
				outRenderInfo->render_gui(state->gameGuiButtonHandles[guiButtonIndex], guiTransform);
			}
		}
	}

	return gameMenuResult;
}