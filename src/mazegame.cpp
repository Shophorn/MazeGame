/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"


const char * bool_str(bool value)
{
	return (value ? "True" : "False");
}

template<typename T>
constexpr bool32 is_type_transform();

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
#include "AudioFile.cpp"
#include "MeshLoader.cpp"
#include "TextureLoader.cpp"

template<typename T>
constexpr bool32 is_type_transform()
{
	constexpr bool32 result = std::is_same<T, Transform3D>::value;
	return result;
}

struct GameState
{
	Handle<Transform3D> characterTransform;
	RenderedObjectHandle characterRenderer;
	CharacterControllerSideScroller characterController;

	AnimationRig ladderRig1;
	AnimationRig ladderRig2;

	Animator 		laddersAnimator1;
	Animator 		laddersAnimator2;

	AnimationClip 	laddersUpAnimation;
	AnimationClip 	laddersDownAnimation;

	Camera worldCamera;
	// CameraController3rdPerson cameraController;
	CameraControllerSideScroller cameraController;

	struct
	{
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	ArenaArray<RenderedObjectHandle> environmentRenderers;
	ArenaArray<Handle<Transform3D>> environmentTransforms;

	bool32 ladderOn = false;
	bool32 ladder2On = false;

	CollisionManager collisionManager;

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
	/*
	Note(Leo): Load all assets to state->transientMemoryArena, process them and load proper
	structures to graphics context. Afterwards, just flush memory arena.
	*/

	allocate_for_handle<Transform3D>(&state->persistentMemoryArena, 100);
	allocate_for_handle<Collider>(&state->persistentMemoryArena, 100);

	state->collisionManager =
	{
		.colliders 		= push_array<Handle<Collider>>(&state->persistentMemoryArena, 200, false),
		.collisions 	= push_array<Collision>(&state->persistentMemoryArena, 300, false) // Todo(Leo): Not enough..
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
	{
		auto characterMesh 				= load_model(&state->transientMemoryArena, "models/character.obj");
		auto characterMeshHandle 		= push_mesh(&characterMesh);

		state->characterRenderer 		= push_renderer(characterMeshHandle, state->materials.character);
		state->characterTransform 		= create_handle<Transform3D>({});
		state->characterController 	= 
		{
			.transform 	= state->characterTransform,
			.collider 	= push_collider(&state->collisionManager, state->characterTransform, {0.4f, 0.8f}, {0.0f, 0.5f}),
		};

		state->characterController.OnTriggerLadder = [state]() -> void
		{
			state->ladderOn = !state->ladderOn;
			if (state->ladderOn)
				play_animation_clip(&state->laddersAnimator1, &state->laddersUpAnimation);
			else
				play_animation_clip(&state->laddersAnimator1, &state->laddersDownAnimation);
		};

		state->characterController.OnTriggerLadder2 = [state]() -> void
		{
			std::cout << "[OnTriggerLadder2()]\n";
			state->ladder2On = !state->ladder2On;
			if (state->ladder2On)
				play_animation_clip(&state->laddersAnimator2, &state->laddersUpAnimation);
			else
				play_animation_clip(&state->laddersAnimator2, &state->laddersDownAnimation);
		};
	}

    state->worldCamera =
    {
    	.forward 		= World::Forward,
    	.fieldOfView 	= 60,
    	.nearClipPlane 	= 0.1f,
    	.farClipPlane 	= 1000.0f,
    	.aspectRatio 	= (real32)platformInfo->windowWidth / (real32)platformInfo->windowHeight	
    };

    state->cameraController =
    {
    	.camera 		= &state->worldCamera,
    	.target 		= state->characterTransform
    };

	// Environment
	{
		constexpr int groundCount 		= 1;
		constexpr int platformCount 	= 12;
		constexpr int pillarCount 		= 2;
		constexpr int ladderCount 		= 12;
		constexpr int buttonCount 		= 2;

		int32 environmentObjectCount 	= groundCount 
										+ platformCount
										+ pillarCount 
										+ ladderCount
										+ buttonCount;

		constexpr float depth = 100;
		constexpr float width = 100;
		constexpr float ladderHeight = 1.0f;

		constexpr bool32 addPillars 	= true;
		constexpr bool32 addLadders 	= true;
		constexpr bool32 addButtons 	= true;
		constexpr bool32 addPlatforms 	= true;

		state->environmentRenderers 	= push_array<RenderedObjectHandle>(&state->persistentMemoryArena, environmentObjectCount, false);
		state->environmentTransforms 	= push_array<Handle<Transform3D>>(&state->persistentMemoryArena, environmentObjectCount, false);

		{
			auto groundQuad 	= mesh_primitives::create_quad(&state->transientMemoryArena);
			auto meshTransform	= Matrix44::Translate({-width / 2, -depth /2, 0}) * Matrix44::Scale({width, depth, 0});
			mesh_ops::transform(&groundQuad, meshTransform);
			mesh_ops::transform_tex_coords(&groundQuad, {0,0}, {width / 2, depth / 2});

			auto groundQuadHandle 	= push_mesh(&groundQuad);
			auto renderer 			= push_renderer(groundQuadHandle, state->materials.environment);
			auto transform 			= create_handle<Transform3D>({});

			push_one(&state->environmentRenderers, renderer);
			push_one(&state->environmentTransforms, transform);
			push_collider(&state->collisionManager, transform, {100, 1}, {0, -1.0f});
		}

		if (addPillars)
		{
			auto pillarMesh 		= load_model(&state->transientMemoryArena, "models/big_pillar.obj");
			auto pillarMeshHandle 	= push_mesh(&pillarMesh);

			auto renderer 	= push_renderer(pillarMeshHandle, state->materials.environment);
			auto transform 	= create_handle<Transform3D>({-width / 4, 0, 0});

			push_one(&state->environmentRenderers, renderer);
			push_one(&state->environmentTransforms, transform);
			push_collider(&state->collisionManager, transform, {2, 25});

			renderer = push_renderer(pillarMeshHandle, state->materials.environment);
			transform = create_handle<Transform3D>({width / 4, 0, 0});

			push_one(&state->environmentRenderers, renderer);
			push_one(&state->environmentTransforms, transform);
			push_collider(&state->collisionManager, transform, {2, 25});
		}

		if (addLadders)
		{
			int firstLadderIndex = state->environmentRenderers.count;
	
			auto ladderMesh 		= load_model(&state->transientMemoryArena, "models/ladder.obj");
			auto ladderMeshHandle 	= push_mesh(&ladderMesh);

			Handle<Transform3D> root1 = create_handle<Transform3D>({0, 0.5f, -ladderHeight});
			Handle<Transform3D> root2 = create_handle<Transform3D>({10, 0.5f, 6 - ladderHeight});
			auto bones1 = push_array<Handle<Transform3D>>(&state->persistentMemoryArena, 6, false);
			auto bones2 = push_array<Handle<Transform3D>>(&state->persistentMemoryArena, 6, false);

			Handle<Transform3D> parent1 = root1;
			Handle<Transform3D> parent2 = root2;
			auto animations = push_array<Animation>(&state->persistentMemoryArena, ladderCount, false);

			int ladder2StartIndex = 6;

			for (int ladderIndex = 0; ladderIndex < ladderCount; ++ladderIndex)
			{
				auto renderer 	= push_renderer(ladderMeshHandle, state->materials.environment);
				auto transform 	= create_handle<Transform3D>({});

				push_one(&state->environmentRenderers, renderer);
				push_one(&state->environmentTransforms, transform);

				push_collider(&state->collisionManager, transform,
														{1.0f, 0.5f},
														{0, 0.5f},
														ColliderTag::Ladder);


				if (ladderIndex < ladder2StartIndex)
				{
					transform->parent = parent1;
					parent1 = transform;
					push_one(&bones1, transform);
	

					// Todo(Leo): only one animation needed, move somewhere else				
					auto keyframes = push_array(&state->persistentMemoryArena, {
						Keyframe{(ladderIndex % ladder2StartIndex) * 0.35f, {0, 0, 0}},
						Keyframe{((ladderIndex % ladder2StartIndex) + 1) * 0.35f, {0, 0, ladderHeight}}
					});
					push_one(&animations, {keyframes});
				}
				else
				{
					transform->parent = parent2;
					parent2 = transform;	
					push_one(&bones2, transform);
				}
			}	


			state->laddersUpAnimation 	= make_animation_clip(animations);
			state->laddersDownAnimation = duplicate_animation_clip(&state->persistentMemoryArena, &state->laddersUpAnimation);
			reverse_animation_clip(&state->laddersDownAnimation);

			state->ladderRig1 = make_animation_rig(root1, bones1, push_array<uint64>(&state->persistentMemoryArena, bones1.count));
			state->ladderRig2 = make_animation_rig(root2, bones2, push_array<uint64>(&state->persistentMemoryArena, bones2.count));

			state->laddersAnimator1 = make_animator(&state->ladderRig1);
			state->laddersAnimator2 = make_animator(&state->ladderRig2);

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

			auto platformMeshAsset 	= load_model(&state->transientMemoryArena, "models/platform.obj");
			auto platformMeshHandle = push_mesh(&platformMeshAsset);

			int platformIndex = 0;
			for (int platformIndex = 0; platformIndex < platformCount; ++platformIndex)
			{
				auto renderer 	= push_renderer(platformMeshHandle, state->materials.environment);
				auto transform 	= create_handle<Transform3D>({platformPositions[platformIndex]});

				push_one(&state->environmentRenderers, renderer);
				push_one(&state->environmentTransforms, transform);
				push_collider(&state->collisionManager, transform, {1.0f, 0.25f});
			}
		}

		if(addButtons)
		{
			auto keyholeMeshAsset 	= load_model(&state->transientMemoryArena, "models/keyhole.obj");
			auto keyholeMeshHandle 	= push_mesh (&keyholeMeshAsset);

			auto renderer 	= push_renderer(keyholeMeshHandle, state->materials.environment);
			auto transform 	= create_handle<Transform3D>({Vector3{5, 0, 0}});

			push_one(&state->environmentRenderers, renderer);
			push_one(&state->environmentTransforms, transform);
			push_collider(&state->collisionManager, transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger);

			renderer 	= push_renderer(keyholeMeshHandle, state->materials.environment);
			transform 	= create_handle<Transform3D>({Vector3{-4, 0, 6}});

			push_one(&state->environmentRenderers, renderer);
			push_one(&state->environmentTransforms, transform);
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

void
OutputSound(int frameCount, game::StereoSoundSample * samples)
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
		OutputSound(soundOutput->sampleCount, soundOutput->samples);
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
	state->laddersAnimator1.Update(input);
	state->laddersAnimator2.Update(input);

	// Note(Leo): Update aspect ratio each frame, in case screen size has changed.
    state->worldCamera.aspectRatio = (real32)platform->windowWidth / (real32)platform->windowHeight;
    state->cameraController.Update(input);

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
		Matrix44 characterTransformMatrix = state->characterTransform->get_matrix();

		outRenderInfo->render(state->characterRenderer, characterTransformMatrix);

		int environmentCount = state->environmentRenderers.count;
		for (int i = 0; i < environmentCount; ++i)
		{
			outRenderInfo->render(state->environmentRenderers[i], state->environmentTransforms[i]->get_matrix());
		}

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

		// Ccamera
	    outRenderInfo->set_camera(	state->worldCamera.ViewProjection(),
	    							state->worldCamera.PerspectiveProjection());
	}

	return gameMenuResult;
}