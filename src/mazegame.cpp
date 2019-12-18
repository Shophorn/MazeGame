/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

struct Transform3D
{
	Vector3 position 	= {0, 0, 0};
	real32 scale 		= 1.0f;
	Quaternion rotation = Quaternion::Identity();

	Transform3D * parent;

	Matrix44 GetMatrix() const noexcept
	{
		/* Study(Leo): members of this struct are ordered so that position and scale would
		occupy first half of struct, and rotation the other half by itself. Does this matter,
		and furthermore does that matter in this function call, both in order arguments are put
		there as well as is the order same as this' members order. */
		Matrix44 result = Matrix44::Transform(position, rotation, scale);

		if (parent != nullptr)
		{
			result = result * parent->GetMatrix();
		}

		return result;
	}

	Vector3 GetWorldPosition()
	{
		if (parent == nullptr)
			return position;

		return parent->GetMatrix() * position;
	}
};

// Note(Leo): Make unity build here.
#include "Random.cpp"
#include "MapGenerator.cpp"
#include "Animator.cpp"
#include "Camera.cpp"
#include "Collisions.cpp"
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
	Character character;
	CharacterControllerSideScroller characterController;
	// Character otherCharacter;

	Animator testAnimator;

	Camera worldCamera;
	// CameraController3rdPerson cameraController;
	CameraControllerSideScroller cameraController;

	struct
	{
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	ArenaArray<RenderedObjectHandle> environmentObjects;
	ArenaArray<Transform3D> environmentTransforms;

	CollisionManager collisionManager;
	RenderedObjectHandle characterObjectHandle;

	int32 staticColliderCount;
	ArenaArray<Rectangle> staticColliders;

	/* Note(Leo): MEMORY
	'persistentMemoryArena' is used to store things from frame to frame.
	'transientMemoryArena' is intended to be used in asset loading etc.*/
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
	structures to graphics context. Afterwards, just flush memoryarena.
	*/

	state->collisionManager =
	{
		.colliderCount = 0,
		.colliders = PushArray<Collider>(&state->persistentMemoryArena, 100),
		.collisions = PushArray<Collision>(&state->persistentMemoryArena, 100) // Todo(Leo): Not enough..
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
    	.target 		= &state->character.transform
    };

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
		state->characterController = 
		{
			.character 	= &state->character,
			.collider 	= state->collisionManager.PushCollider(&state->character.transform, {0.5f, 1.0f})
		};

		state->character.transform.position = {0, 0, 1};
	}


	// Environment
	{
		int groundCount = 1;
		int pillarCount = 2;
		int ladderCount = 6;
		int keyholeCount = 1;

		int environmentObjectCount 		= groundCount + pillarCount + ladderCount + keyholeCount;

		state->environmentObjects 		= PushArray<RenderedObjectHandle>(&state->persistentMemoryArena, environmentObjectCount);
		state->environmentTransforms 	= PushArray<Transform3D>(&state->persistentMemoryArena, environmentObjectCount);
	
		auto groundQuad 				= mesh_primitives::CreateQuad(&state->transientMemoryArena);

		float width = 100;
		float depth = 100;
		auto meshTransform				= Matrix44::Translate({-width / 2, -depth /2, 0}) * Matrix44::Scale({width, depth, 0});
		mesh_ops::Transform(&groundQuad, meshTransform);
		mesh_ops::TransformTexCoords(&groundQuad, {0,0}, {width / 2, depth / 2});

		auto groundQuadHandle 			= PushMesh(&groundQuad);
		state->environmentObjects[0] 	= PushRenderer(groundQuadHandle, state->materials.environment);
		state->environmentTransforms[0] = {};

		auto pillarMesh 				= LoadModel(&state->transientMemoryArena, "models/big_pillar.obj");
		auto pillarMeshHandle 			= PushMesh(&pillarMesh);
		state->environmentObjects[1]	= PushRenderer(pillarMeshHandle, state->materials.environment);
		state->environmentObjects[2]	= PushRenderer(pillarMeshHandle, state->materials.environment);

		state->environmentTransforms [1] = {-width / 4, 0, 0};
		state->environmentTransforms [2] = {width / 4, 0, 0};

		state->collisionManager.PushCollider(&state->environmentTransforms[0], {100, 1}, {0, -1.0f});
		// state->collisionManager.PushCollider(&state->environmentTransforms[1], {2, 25});
		// state->collisionManager.PushCollider(&state->environmentTransforms[2], {2, 25});

		auto ladderMesh 				= LoadModel(&state->transientMemoryArena, "models/ladder.obj");
		auto ladderMeshHandle 			= PushMesh(&ladderMesh);

		float ladderHeight = 1.0f;
		state->testAnimator = 
		{
		 	// .localStartPosition = {-10, 0.5, -1}, 
		 	// .localEndPosition 	= {-10, 0.5, 0},
		 	.localStartPosition = {0, 0, 0}, 
		 	.localEndPosition 	= {0, 0, ladderHeight},
		 	.speed 				= 2.5f
		};

		int environmentIndex = 3;
		for (int ladderIndex = 0; ladderIndex < ladderCount; ++ladderIndex)
		{
			state->environmentObjects[environmentIndex] = PushRenderer(ladderMeshHandle, state->materials.environment);
		
			if (ladderIndex == 0)
			{
				state->environmentTransforms[environmentIndex] = {-10, 0.5, ladderHeight};
			}
			else
			{
				state->environmentTransforms[environmentIndex] = {0, 0, 0};
				state->environmentTransforms[environmentIndex].parent = &state->environmentTransforms[environmentIndex - 1];
			}

			state->testAnimator.targets[ladderIndex] = &state->environmentTransforms[environmentIndex];

			auto * collider = state->collisionManager.PushCollider(&state->environmentTransforms[environmentIndex], {1.0f, 0.5f}, {0, 0.5f});
			collider->isLadder = true;
			environmentIndex++;
		}


		{
			auto keyholeMeshAsset = LoadModel(&state->transientMemoryArena, "models/keyhole.obj");
			auto keyholeMeshHandle = PushMesh (&keyholeMeshAsset);
			state->environmentObjects[environmentIndex] = PushRenderer(keyholeMeshHandle, state->materials.environment);
			state->environmentTransforms[environmentIndex] = {Vector3{5, 0, 0}};
			auto * collider = state->collisionManager.PushCollider(&state->environmentTransforms[environmentIndex], {0.3f, 0.6f}, {0, 0.3f}, ColliderTag::Trigger);


			environmentIndex++;
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

	// Generate static, unchanging colliders
	{
		// Nothing here now
	}

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
	state->testAnimator.Update(input);

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
		Matrix44 characterTransform = state->character.transform.GetMatrix();
		outRenderInfo->renderedObjects[state->characterObjectHandle] = characterTransform;

		int environmentCount = state->environmentObjects.count;
		for (int i = 0; i < environmentCount; ++i)
		{
			outRenderInfo->renderedObjects[state->environmentObjects[i]]
				= state->environmentTransforms[i].GetMatrix();
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