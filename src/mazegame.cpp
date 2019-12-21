/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

template<typename T>
struct Handle
{
	bool32 IsValid() const
	{ 
		bool32 result = (index > -1) && (index < storage.count);
		return result;
	}

	T * operator->()
	{ 
		MAZEGAME_ASSERT(IsValid(), "Cannot reference uninitialized Handle.");
		return &storage[index];
	}

	const T * operator -> () const
	{
		MAZEGAME_ASSERT(IsValid(), "Cannot reference uninitialized Handle.");
		return &storage[index];	
	}


	// Note(Leo): We may not need these, as I am  not quite sure if they are good idea or not
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

	static Handle
	Create (T item)
	{
		Handle handle = {};
		handle.index = storage.Push(item);
		return handle;
	}

	static void
	Allocate(MemoryArena * memoryArena, int32 count)
	{
		storage = PushArray<T>(memoryArena, count, false);
	}

	Handle () = default;
	Handle (const Handle & other) = default;

	inline static Handle Null = {};

private:
	int64 index = -1;

	// This is not necessarily the best idea
	inline global_variable ArenaArray<T> storage;
};

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

		if (parent.IsValid())
		{
			result = result * parent->GetMatrix();
		}

		return result;
	}

	Vector3 GetWorldPosition() const noexcept
	{
		if (parent.IsValid())
		{
			return parent->GetMatrix() * position;
		}

		return position;
	}
};


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
	Animation laddersUpAnimation;
	Animation laddersDownAnimation;

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
InitializeGameState(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	*state = {};

	// Note(Leo): Create persistent arena in the same memoryblock as game state, right after it.
	state->persistentMemoryArena = MemoryArena::Create(
										reinterpret_cast<byte*>(memory->persistentMemory) + sizeof(GameState),
										memory->persistentMemorySize - sizeof(GameState));

	state->transientMemoryArena = MemoryArena::Create(
										reinterpret_cast<byte*>(memory->transientMemory),
										memory->transientMemorySize);


	std::cout << "Screen size = (" << platformInfo->windowWidth << "," << platformInfo->windowHeight << ")\n";
}

void 
LoadMainLevel(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	/*
	Note(Leo): Load all assets to state->transientMemoryArena, process them and load proper
	structures to graphics context. Afterwards, just flush memory arena.
	*/

	Handle<Transform3D>::Allocate(&state->persistentMemoryArena, 20);
	Handle<Collider>::Allocate(&state->persistentMemoryArena, 20);

	state->collisionManager =
	{
		.colliders 		= PushArray<Handle<Collider>>(&state->persistentMemoryArena, 100, false),
		.collisions 	= PushArray<Collision>(&state->persistentMemoryArena, 100) // Todo(Leo): Not enough..
	};

	// Create MateriaLs
	{
		TextureAsset whiteTextureAsset = {};
		whiteTextureAsset.pixels = PushArray<uint32>(&state->transientMemoryArena, 1);
		whiteTextureAsset.pixels[0] = 0xffffffff;
		whiteTextureAsset.width = 1;
		whiteTextureAsset.height = 1;
		whiteTextureAsset.channels = 4;

		TextureAsset blackTextureAsset = {};
		blackTextureAsset.pixels = PushArray<uint32>(&state->transientMemoryArena, 1);
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

		auto PushMaterial = [platformInfo](MaterialType type, TextureHandle a, TextureHandle b, TextureHandle c) -> MaterialHandle
		{
			MaterialAsset asset = { type, a, b, c };
			MaterialHandle handle = platformInfo->graphicsContext->PushMaterial(&asset);
			return handle;
		};

		state->materials =
		{
			.character  	= PushMaterial(	MaterialType::Character,
											lavaTexture,
											textureTexture,
											blackTexture),

			.environment 	= PushMaterial(	MaterialType::Character,
											tilesTexture,
											blackTexture,
											blackTexture)
		};
	}

    auto PushMesh = [platformInfo] (MeshAsset * asset) -> MeshHandle
    {
    	auto handle = platformInfo->graphicsContext->PushMesh(asset);
    	return handle;
    };

    auto PushRenderer = [platformInfo] (MeshHandle mesh, MaterialHandle material) -> RenderedObjectHandle
    {
    	auto handle = platformInfo->graphicsContext->PushRenderedObject(mesh, material);
    	return handle;
    };

	// Characters
	{
		auto characterMesh 					= LoadModel(&state->transientMemoryArena, "models/character.obj");
		auto characterMeshHandle 			= PushMesh(&characterMesh);

		state->characterObjectHandle 		= PushRenderer(	characterMeshHandle,
															state->materials.character);

		state->characterTransform = Handle<Transform3D>::Create({});
		state->characterController = 
		{
			.transform 	= state->characterTransform,
			.collider 	= state->collisionManager.PushCollider(state->characterTransform, {0.5f, 1.0f})
		};

		state->characterTransform->position = {0, 0, 1};

		state->characterController.OnTriggerLadder = [state]() -> void
		{
			static bool32 ladderOn = false;

			ladderOn = !ladderOn;
			if (ladderOn)
				state->laddersAnimator.Play(&state->laddersUpAnimation);
			else
				state->laddersAnimator.Play(&state->laddersDownAnimation);

			std::cout << "Ladder button triggered\n";

		};
		// state->characterController.OnTriggerLadder = [state]()
		// {
		// 	state->laddersAnimator.Play(state->laddersUpAnimation);
		// };//TriggerLadder;
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
		constexpr int platformCount 	= 2;
		constexpr int pillarCount 		= 2;
		constexpr int ladderCount 		= 6;
		constexpr int keyholeCount 		= 1;

		int32 environmentObjectCount 		= groundCount + platformCount + pillarCount + ladderCount + keyholeCount;

		constexpr float depth = 100;
		constexpr float width = 100;
		constexpr float ladderHeight = 1.0f;

		constexpr bool32 addPillars 	= true;
		constexpr bool32 addLadders 	= true;
		constexpr bool32 addButton 		= true;
		constexpr bool32 addPlatforms 	= true;

		state->environmentObjects 		= PushArray<RenderedObjectHandle>(&state->persistentMemoryArena, environmentObjectCount, false);
		state->environmentTransforms 	= PushArray<Handle<Transform3D>>(&state->persistentMemoryArena, environmentObjectCount, false);

		int environmentIndex = 0;
		{
			auto groundQuad 				= mesh_primitives::CreateQuad(&state->transientMemoryArena);

			auto meshTransform				= Matrix44::Translate({-width / 2, -depth /2, 0}) * Matrix44::Scale({width, depth, 0});
			mesh_ops::Transform(&groundQuad, meshTransform);
			mesh_ops::TransformTexCoords(&groundQuad, {0,0}, {width / 2, depth / 2});

			auto groundQuadHandle 							= PushMesh(&groundQuad);
			auto index = state->environmentObjects.Push(PushRenderer(groundQuadHandle, state->materials.environment));
			state->environmentTransforms.Push(Handle<Transform3D>::Create({}));
			state->collisionManager.PushCollider(state->environmentTransforms[index], {100, 1}, {0, -1.0f});

			// environmentIndex++;
		}

		if (addPillars)
		{
			auto pillarMesh 				= LoadModel(&state->transientMemoryArena, "models/big_pillar.obj");
			auto pillarMeshHandle 			= PushMesh(&pillarMesh);

			auto index = state->environmentObjects.Push(PushRenderer(pillarMeshHandle, state->materials.environment));
			state->environmentTransforms.Push(Handle<Transform3D>::Create({-width / 4, 0, 0}));
			state->collisionManager.PushCollider(state->environmentTransforms[index], {2, 25});

			index = state->environmentObjects.Push(PushRenderer(pillarMeshHandle, state->materials.environment));
			state->environmentTransforms.Push(Handle<Transform3D>::Create({width / 4, 0, 0}));
			state->collisionManager.PushCollider(state->environmentTransforms[index], {2, 25});
		}

		if (addLadders)
		{
			int firstLadderIndex = state->environmentObjects.count;
	
			auto ladderMesh 		= LoadModel(&state->transientMemoryArena, "models/ladder.obj");
			auto ladderMeshHandle 	= PushMesh(&ladderMesh);

			Handle<Transform3D> parent = Handle<Transform3D>::Null;

			for (int ladderIndex = 0; ladderIndex < ladderCount; ++ladderIndex)
			{
				Vector3 position = (ladderIndex == 0) ? 
									Vector3{-10, 0.5, ladderHeight} : 
									Vector3{0,0,0};

				auto index = state->environmentObjects.Push(PushRenderer(ladderMeshHandle, state->materials.environment));
				state->environmentTransforms.Push(Handle<Transform3D>::Create({position}));
				state->environmentTransforms[index]->parent = parent;
				parent = state->environmentTransforms[index];


				Handle<Collider> collider = state->collisionManager.PushCollider(state->environmentTransforms[index], {1.0f, 0.5f}, {0, 0.5f});
				collider->isLadder = true;
				environmentIndex++;
			}

			// TEST ANIMATIONS
			state->laddersUpAnimation =
			{
				.children = PushArray<ChildAnimation>(&state->persistentMemoryArena, 6, false),
				.duration = 2.4f
			};
			uint64 childIndex;
			auto * childAnimations = &state->laddersUpAnimation.children;

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 0], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({0.0f, {0, 0, -ladderHeight}});
			(*childAnimations)[childIndex].keyframes.Push({0.4f, {0, 0, 0}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 1], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({0.4f, {0, 0, 0}});
			(*childAnimations)[childIndex].keyframes.Push({0.8f, {0, 0, ladderHeight}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 2], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({0.8f, {0, 0, 0}});
			(*childAnimations)[childIndex].keyframes.Push({1.2f, {0, 0, ladderHeight}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 3], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({1.2f, {0, 0, 0}});
			(*childAnimations)[childIndex].keyframes.Push({1.6f, {0, 0, ladderHeight}});
			
			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 4], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({1.6f, {0, 0, 0}});
			(*childAnimations)[childIndex].keyframes.Push({2.0f, {0, 0, ladderHeight}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 5], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({2.0f, {0, 0, 0}});
			(*childAnimations)[childIndex].keyframes.Push({2.4f, {0, 0, ladderHeight}});


			state->laddersDownAnimation =
			{
				.children = PushArray<ChildAnimation>(&state->persistentMemoryArena, 6, false),
				.duration = 2.4f
			};
			childAnimations = &state->laddersDownAnimation.children;

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 5], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({0.0f, {0, 0, ladderHeight}});
			(*childAnimations)[childIndex].keyframes.Push({0.4f, {0, 0, 0}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 4], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({0.4f, {0, 0, ladderHeight}});
			(*childAnimations)[childIndex].keyframes.Push({0.8f, {0, 0, 0}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 3], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({0.8f, {0, 0, ladderHeight}});
			(*childAnimations)[childIndex].keyframes.Push({1.2f, {0, 0, 0}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 2], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({1.2f, {0, 0, ladderHeight}});
			(*childAnimations)[childIndex].keyframes.Push({1.6f, {0, 0, 0}});
			
			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 1], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({1.6f, {0, 0, ladderHeight}});
			(*childAnimations)[childIndex].keyframes.Push({2.0f, {0, 0, 0}});

			childIndex = childAnimations->Push({state->environmentTransforms[firstLadderIndex + 0], PushArray<Keyframe>(&state->persistentMemoryArena, 2, false)});
			(*childAnimations)[childIndex].keyframes.Push({2.0f, {0, 0, 0}});
			(*childAnimations)[childIndex].keyframes.Push({2.4f, {0, 0, -ladderHeight}});

			state->laddersAnimator = {};
			// state->laddersAnimator.Play(&state->laddersUpAnimation);
		}

		if(addButton)
		{
			auto keyholeMeshAsset = LoadModel(&state->transientMemoryArena, "models/keyhole.obj");
			auto keyholeMeshHandle = PushMesh (&keyholeMeshAsset);

			auto renderer = PushRenderer(keyholeMeshHandle, state->materials.environment);
			auto transform = Handle<Transform3D>::Create({Vector3{5, 0, 0}});

			state->environmentObjects.Push(renderer);
			state->environmentTransforms.Push(transform);
			
			state->collisionManager.PushCollider(transform, {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger);
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

			auto platformMeshAsset = LoadModel(&state->transientMemoryArena, "models/platform.obj");
			auto platformMeshHandle = PushMesh(&platformMeshAsset);

			int platformIndex = 0;
			for (int platformIndex = 0; platformIndex < platformCount; ++platformIndex)
			{
				auto index = state->environmentObjects.Push(PushRenderer(platformMeshHandle, state->materials.environment));
				state->environmentTransforms.Push(Handle<Transform3D>::Create({platformPositions[platformIndex]}));
				state->collisionManager.PushCollider(state->environmentTransforms[index], {1.0f, 0.25f});
			}

		}
	}

	state->gameGuiButtonCount = 2;
	state->gameGuiButtons = PushArray<Rectangle>(&state->persistentMemoryArena, state->gameGuiButtonCount);
	state->gameGuiButtonHandles = PushArray<GuiHandle>(&state->persistentMemoryArena, state->gameGuiButtonCount);

	state->gameGuiButtons[0] = {380, 200, 180, 40};
	state->gameGuiButtons[1] = {380, 260, 180, 40};

	MeshAsset quadAsset = mesh_primitives::CreateQuad(&state->transientMemoryArena);
 	MeshHandle quadHandle = PushMesh(&quadAsset);

	for (int guiButtonIndex = 0; guiButtonIndex < state->gameGuiButtonCount; ++guiButtonIndex)
	{
		state->gameGuiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, state->materials.character);
	}

	state->selectedGuiButtonIndex = 0;
	state->showGameMenu = false;

 	// Note(Leo): Apply all pushed changes and flush transient memory
    platformInfo->graphicsContext->Apply();
    state->transientMemoryArena.Flush();
}

void
LoadMenu(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	// Create MateriaLs
	{
		TextureAsset whiteTextureAsset = {};
		whiteTextureAsset.pixels = PushArray<uint32>(&state->transientMemoryArena, 1);
		whiteTextureAsset.pixels[0] = 0xffffffff;
		whiteTextureAsset.width = 1;
		whiteTextureAsset.height = 1;
		whiteTextureAsset.channels = 4;

		TextureAsset blackTextureAsset = {};
		blackTextureAsset.pixels = PushArray<uint32>(&state->transientMemoryArena, 1);
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
	state->menuGuiButtons = PushArray<Rectangle>(&state->persistentMemoryArena, state->menuGuiButtonCount);
	state->menuGuiButtonHandles = PushArray<GuiHandle>(&state->persistentMemoryArena, state->menuGuiButtonCount);

	state->menuGuiButtons[0] = {380, 200, 180, 40};
	state->menuGuiButtons[1] = {380, 260, 180, 40};
	state->menuGuiButtons[2] = {380, 320, 180, 40};
	state->menuGuiButtons[3] = {380, 380, 180, 40};

	MeshAsset quadAsset = mesh_primitives::CreateQuad(&state->transientMemoryArena);
 	MeshHandle quadHandle = platformInfo->graphicsContext->PushMesh(&quadAsset);

	for (int guiButtonIndex = 0; guiButtonIndex < state->menuGuiButtonCount; ++guiButtonIndex)
	{
		state->menuGuiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, state->materials.character);
	}

	state->selectedGuiButtonIndex = 0;

 	// Note(Leo): Apply all pushed changes and flush transient memory
    platformInfo->graphicsContext->Apply();
    state->transientMemoryArena.Flush();
}

void
OutputSound(int frameCount, game::StereoSoundSample * samples)
{
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
		InitializeGameState (state, memory, platform);
		memory->isInitialized = true;

		LoadMenu(state, memory, platform);
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
			state->persistentMemoryArena.Flush();

			LoadMainLevel(state, memory, platform);
			state->levelLoaded = true;
		}
	}
	else
	{
		MenuResult result = UpdateMainLevel(state, input, memory, platform, network, soundOutput, outRenderInfo);
		if (result == MENU_EXIT)
		{
			platform->graphicsContext->UnloadAll();
			state->persistentMemoryArena.Clear();

			LoadMenu(state, memory, platform);
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
	state->transientMemoryArena.Flush();
	state->collisionManager.DoCollisions();


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