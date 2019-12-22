/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"


/* Todo(Leo): this struct is horrible:
	we can use -> to access the actual represented type/item
	and . to access the 'meta' described here

	... though 'meta' things means only 'is_valid' and 'get_index',
	so they should be easy to fix.
*/

const char * bool_str(bool value)
{
	return (value ? "True" : "False");
}

template<typename T>
constexpr bool32 is_type_transform();

template<typename T>
struct Handle
{
	// Todo(Leo): Make use of Null thing down below
	bool32 is_valid() const
	{ 
		bool32 result = (index > -1) && (index < storage.count);
		return result;
	}

	T * operator->()
	{ 
		MAZEGAME_ASSERT(is_valid(), "Cannot reference uninitialized Handle.");
		return &storage[index];
	}

	const T * operator -> () const
	{
		MAZEGAME_ASSERT(is_valid(), "Cannot reference uninitialized Handle.");
		return &storage[index];	
	}


	// Note(Leo): We may not need these, as I am not quite sure if they are good idea or not
	// operator T* ()
	// {
	// 	MAZEGAME_ASSERT(IsValid(), "Cannot reference uninitialized Handle.");
	// 	return &storage[index];	
	// }

	// operator const T* () const
	// {
	// 	MAZEGAME_ASSERT(IsValid(), "Cannot reference uninitialized Handle.");
	// 	return &storage[index];	
	// }

	/* Note(Leo): We use concrete item instead of constructor arguments
	and (relay/depend/what is the word??) on copy elision to remove copy */
	static Handle
	create (T item)
	{
		Handle result = {};
		result.index = storage.Push(item);

		// if constexpr (std::is_same<T, Transform3D>)
		if constexpr (is_type_transform<T>())
		{
			std::cout << "[Handle]: created " << result.get_index() << "\n";
		}

		return result;
	}

	static void
	allocate(MemoryArena * memoryArena, int32 count)
	{
		storage = push_array<T>(memoryArena, count, false);
	}

	Handle () = default;
	Handle (const Handle & other) = default;

	inline static Handle Null = {};

	int64 get_index() { return index; }

private:
	int64 index = -1;

	// This is not necessarily the best idea
	inline global_variable ArenaArray<T> storage;
};

template<typename T>
Handle<T> create_handle(T item)
{
	auto result = Handle<T>::create(item);
	return result;
}

struct Transform3D
{
	Vector3 position 	= {0, 0, 0};
	real32 scale 		= 1.0f;
	Quaternion rotation = Quaternion::Identity();

	Handle<Transform3D> parent;

	Matrix44 GetMatrix() const noexcept
	{
		/* Study(Leo): members of this struct are ordered so that position and scale would
		occupy first half of struct, and rotation the other half by itself. Does this matter,
		and furthermore does that matter in this function call, both in order arguments are put
		there as well as is the order same as this' members order. */
		Matrix44 result = Matrix44::Transform(position, rotation, scale);

		if (parent.is_valid())
		{
			result = result * parent->GetMatrix();
		}

		return result;
	}

	Vector3 GetWorldPosition() const noexcept
	{
		if (parent.is_valid())
		{
			return parent->GetMatrix() * position;
		}

		return position;
	}
};

template<typename T>
constexpr bool32 is_type_transform()
{
	constexpr bool32 result = std::is_same<T, Transform3D>::value;
	return result;
}


// Note(Leo): Make unity build here.
#include "Random.cpp"
#include "MapGenerator.cpp"
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

using float3 = Vector3;

using UpdateFunc = void(game::Input * input);

struct GameState
{
	Handle<Transform3D> characterTransform;
	CharacterControllerSideScroller characterController;

	Animator laddersAnimator;
	AnimationClip laddersUpAnimation;
	AnimationClip laddersDownAnimation;

	Camera worldCamera;
	// CameraController3rdPerson cameraController;
	CameraControllerSideScroller cameraController;

	struct
	{
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	ArenaArray<RenderedObjectHandle> environmentObjects;
	ArenaArray<Handle<Transform3D>> environmentTransforms;

	bool32 ladderOn = false;

	CollisionManager collisionManager;
	RenderedObjectHandle characterObjectHandle;

	int32 staticColliderCount;
	ArenaArray<Rectangle> staticColliders;

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

void
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

void 
load_main_level(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	/*
	Note(Leo): Load all assets to state->transientMemoryArena, process them and load proper
	structures to graphics context. Afterwards, just flush memory arena.
	*/

	Handle<Transform3D>::allocate(&state->persistentMemoryArena, 100);
	Handle<Collider>::allocate(&state->persistentMemoryArena, 100);

	state->collisionManager =
	{
		.colliders 		= push_array<Handle<Collider>>(&state->persistentMemoryArena, 200, false),
		.collisions 	= push_array<Collision>(&state->persistentMemoryArena, 300, false) // Todo(Leo): Not enough..
	};

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

		// Note(Leo): Loads texture asset from "textures/<name>.jpg" to a variable <name>TextureAsset
 		#define LOAD_TEXTURE(name) TextureAsset name ## TextureAsset = LoadTextureAsset("textures/" #name ".jpg", &state->transientMemoryArena);

		LOAD_TEXTURE(tiles)
		LOAD_TEXTURE(lava)
		LOAD_TEXTURE(texture)
		
		#undef LOAD_TEXTURE

		// Note(Leo): Pushes texture asset to graphics context and stores the handle in a variable named <name>Texture
		#define PUSH_TEXTURE(name) TextureHandle name ## Texture = platformInfo->graphicsContext->PushTexture(&name ## TextureAsset);

		PUSH_TEXTURE(tiles)
		PUSH_TEXTURE(lava)
		PUSH_TEXTURE(texture)

		#undef PUSH_TEXTURE

		auto push_material = [platformInfo](MaterialType type, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = { type, a, b, c };
			MaterialHandle handle = platformInfo->graphicsContext->PushMaterial(&asset);
			return handle;
		};

		state->materials =
		{
			.character  	= push_material(	MaterialType::Character,
											lavaTexture,
											textureTexture,
											blackTexture),

			.environment 	= push_material(	MaterialType::Character,
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
		auto characterMesh 					= load_model(&state->transientMemoryArena, "models/character.obj");
		auto characterMeshHandle 			= push_mesh(&characterMesh);

		state->characterObjectHandle 		= push_renderer(characterMeshHandle,
															state->materials.character);

		state->characterTransform = create_handle<Transform3D>({});
		state->characterController = 
		{
			.transform 	= state->characterTransform,
			.collider 	= state->collisionManager.PushCollider(state->characterTransform, {0.4f, 0.8f}, {0.0f, 0.5f}),
		};

		state->characterTransform->position = {0, 0, 1};

		state->characterController.OnTriggerLadder = [state]() -> void
		{
			state->ladderOn = !state->ladderOn;
			if (state->ladderOn)
				state->laddersAnimator.Play(&state->laddersUpAnimation);
			else
				state->laddersAnimator.Play(&state->laddersDownAnimation);

			std::cout << "Ladder button triggered\n";

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
		constexpr int platformCount 	= 4;
		constexpr int pillarCount 		= 2;
		constexpr int ladderCount 		= 6;
		constexpr int keyholeCount 		= 1;

		int32 environmentObjectCount 	= groundCount 
										+ platformCount
										+ pillarCount 
										+ ladderCount
										+ keyholeCount;


		constexpr float depth = 100;
		constexpr float width = 100;
		constexpr float ladderHeight = 1.0f;

		constexpr bool32 addPillars 	= true;
		constexpr bool32 addLadders 	= true;
		constexpr bool32 addButton 		= true;
		constexpr bool32 addPlatforms 	= true;

		state->environmentObjects 		= push_array<RenderedObjectHandle>(&state->persistentMemoryArena, environmentObjectCount, false);
		state->environmentTransforms 	= push_array<Handle<Transform3D>>(&state->persistentMemoryArena, environmentObjectCount, false);

		{
			auto groundQuad 				= mesh_primitives::create_quad(&state->transientMemoryArena);

			auto meshTransform				= Matrix44::Translate({-width / 2, -depth /2, 0}) * Matrix44::Scale({width, depth, 0});
			mesh_ops::transform(&groundQuad, meshTransform);
			mesh_ops::transform_tex_coords(&groundQuad, {0,0}, {width / 2, depth / 2});

			auto groundQuadHandle 							= push_mesh(&groundQuad);
			auto index = state->environmentObjects.Push(push_renderer(groundQuadHandle, state->materials.environment));
			state->environmentTransforms.Push(create_handle<Transform3D>({}));
			state->collisionManager.PushCollider(state->environmentTransforms[index], {100, 1}, {0, -1.0f});
		}

		if (addPillars)
		{
			auto pillarMesh 		= load_model(&state->transientMemoryArena, "models/big_pillar.obj");
			auto pillarMeshHandle 	= push_mesh(&pillarMesh);

			auto index = state->environmentObjects.Push(push_renderer(pillarMeshHandle, state->materials.environment));
			state->environmentTransforms.Push(create_handle<Transform3D>({-width / 4, 0, 0}));
			state->collisionManager.PushCollider(state->environmentTransforms[index], {2, 25});

			index = state->environmentObjects.Push(push_renderer(pillarMeshHandle, state->materials.environment));
			state->environmentTransforms.Push(create_handle<Transform3D>({width / 4, 0, 0}));
			state->collisionManager.PushCollider(state->environmentTransforms[index], {2, 25});
		}

		if (addLadders)
		{
			int firstLadderIndex = state->environmentObjects.count;
	
			auto ladderMesh 		= load_model(&state->transientMemoryArena, "models/ladder.obj");
			auto ladderMeshHandle 	= push_mesh(&ladderMesh);

			Handle<Transform3D> parent = create_handle<Transform3D>({0, 0.5f, -ladderHeight});

			auto animations = push_array<Animation>(&state->persistentMemoryArena, ladderCount, false);

			for (int ladderIndex = 0; ladderIndex < ladderCount; ++ladderIndex)
			{
				Vector3 position = Vector3{0,0,0};

				auto renderer 	= push_renderer(ladderMeshHandle, state->materials.environment);
				auto transform 	= create_handle<Transform3D>({position});

				state->environmentObjects.Push(renderer);
				state->environmentTransforms.Push(transform);
				transform->parent = parent;
				parent = transform;

				Handle<Collider> collider = state->collisionManager.PushCollider(	transform,
																					{1.0f, 0.5f},
																					{0, 0.5f},
																					ColliderTag::Ladder);

				auto keyframes = push_array<Keyframe>(&state->persistentMemoryArena, {
					Keyframe{ladderIndex * 0.35f, {0, 0, 0}},
					Keyframe{(ladderIndex + 1) * 0.35f, {0, 0, ladderHeight}}
				});
				animations.Push({transform, keyframes});
			}	

			state->laddersUpAnimation 	= make_animation_clip(animations);
			state->laddersDownAnimation = reverse_animation_clip(&state->persistentMemoryArena, &state->laddersUpAnimation);

			state->laddersAnimator = {};
		}

		if(addButton)
		{
			auto keyholeMeshAsset 	= load_model(&state->transientMemoryArena, "models/keyhole.obj");
			auto keyholeMeshHandle 	= push_mesh (&keyholeMeshAsset);

			auto renderer 	= push_renderer(keyholeMeshHandle, state->materials.environment);
			auto transform 	= create_handle<Transform3D>({Vector3{5, 0, 0}});

			state->environmentObjects.Push(renderer);
			state->environmentTransforms.Push(transform);
			
			std::cout << "[ADD BUTTON TRIGGER]";
			state->collisionManager.PushCollider(	transform, 
													{0.3f, 0.6f},
													{0, 0.3f},
													ColliderTag::Trigger);
		}

		if (addPlatforms)
		{
			Vector3 platformPositions [] =
			{
				{-2, 0, 6},
				{2, 0, 6},
				{4, 0, 6},
				{6, 0, 6},
			};

			auto platformMeshAsset = load_model(&state->transientMemoryArena, "models/platform.obj");
			auto platformMeshHandle = push_mesh(&platformMeshAsset);

			int platformIndex = 0;
			for (int platformIndex = 0; platformIndex < platformCount; ++platformIndex)
			{
				auto renderer 	= push_renderer(platformMeshHandle, state->materials.environment);
				auto transform 	= create_handle<Transform3D>({platformPositions[platformIndex]});

				// push_one(&state->environmentObjects, renderer);
				// push_one(&state->environmentTransforms, transform);
				// push_collider(&state->collisionManager, transformm {1.0f, 0.25f});

				state->environmentObjects.Push(renderer);
				state->environmentTransforms.Push(transform);
				state->collisionManager.PushCollider(transform, {1.0f, 0.25f});
			}

		}
	}

	state->gameGuiButtonCount = 2;
	state->gameGuiButtons = push_array<Rectangle>(&state->persistentMemoryArena, state->gameGuiButtonCount);
	state->gameGuiButtonHandles = push_array<GuiHandle>(&state->persistentMemoryArena, state->gameGuiButtonCount);

	state->gameGuiButtons[0] = {380, 200, 180, 40};
	state->gameGuiButtons[1] = {380, 260, 180, 40};

	MeshAsset quadAsset = mesh_primitives::create_quad(&state->transientMemoryArena);
 	MeshHandle quadHandle = push_mesh(&quadAsset);

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

void
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
	        LoadTextureAsset("textures/lava.jpg", &state->transientMemoryArena),
	        LoadTextureAsset("textures/texture.jpg", &state->transientMemoryArena),
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

			outRenderInfo->guiObjects[state->menuGuiButtonHandles[guiButtonIndex]] = guiTransform;
		}

		// Ccamera
	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
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
	// state->characterController.Update(input, &state->worldCamera, &state->staticColliders);
	state->characterController.Update(input, &state->collisionManager);
	state->laddersAnimator.Update(input);

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
		Matrix44 characterTransformMatrix = state->characterTransform->GetMatrix();
		outRenderInfo->renderedObjects[state->characterObjectHandle] = characterTransformMatrix;

		int environmentCount = state->environmentObjects.count;
		for (int i = 0; i < environmentCount; ++i)
		{
			outRenderInfo->renderedObjects[state->environmentObjects[i]]
				= state->environmentTransforms[i]->GetMatrix();
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

				outRenderInfo->guiObjects[state->gameGuiButtonHandles[guiButtonIndex]] = guiTransform;
			}
		}
		else
		{
			for (int  guiButtonIndex = 0; guiButtonIndex < state->gameGuiButtonCount; ++guiButtonIndex)
			{
				Matrix44 guiTransform = {};
				outRenderInfo->guiObjects[state->gameGuiButtonHandles[guiButtonIndex]] = guiTransform;
			}
		}

		// Ccamera
	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}

	return gameMenuResult;
}