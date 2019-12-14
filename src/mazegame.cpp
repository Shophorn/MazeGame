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

	Matrix44 GetMatrix()
	{
		/* Study(Leo): members of this struct are ordered so that position and scale would
		occupy first half of struct, and rotation the other half by itself. Does this matter,
		and furthermore does that matter in this function call, both in order arguments are put
		there as well as is the order same as this' members order. */
		Matrix44 result = Matrix44::Transform(position, rotation, scale);
		return result;
	}
};


// Note(Leo): Make unity build here
#include "Random.cpp"
#include "MapGenerator.cpp"
#include "Camera.cpp"
#include "Collisions.cpp"
#include "CharacterSystems.cpp"

/// Note(Leo): These still use external libraries we may want to get rid of
#include "AudioFile.cpp"
#include "MeshLoader.cpp"
#include "TextureLoader.cpp"

using float3 = Vector3;

struct GameState
{
	Character character;
	// Character otherCharacter;

	// Vector3 keyLocalPosition;
	// Quaternion keyLocalRotation;

	real32 cameraOrbitDegrees = 180;
	real32 cameraTumbleDegrees;
	real32 cameraDistance = 20.0f;

	Camera worldCamera;

	struct
	{
		MaterialHandle character;
		MaterialHandle environment;
	} materials;

	ArenaArray<RenderedObjectHandle> environmentObjects;
	ArenaArray<Transform3D> environmentTransforms;

	// RenderedObjectHandle levelObjectHandle;
	RenderedObjectHandle characterObjectHandle;
	// RenderedObjectHandle otherCharacterObjectHandle;
	// RenderedObjectHandle keyObjectHandle;

	// int32 								treeCount;
	// ArenaArray<Transform3D>				treeTransforms;
	// ArenaArray<RenderedObjectHandle> 	treeRenderHandles;
	// real32 								treeCollisionRadius;


	// int32 								keyholeCount;
	// ArenaArray<Transform3D>				keyholeTransforms;
	// ArenaArray<RenderedObjectHandle> 	keyholeRenderHandles;
	// real32								keyholeCollisionRadius;
	// ArenaArray<Circle>					keyholeTriggerColliders;



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
		state->materials.character 		= PushMaterial(MaterialType::Character, lavaTexture, textureTexture, blackTexture);
		state->materials.environment 	= PushMaterial(MaterialType::Character, tilesTexture, blackTexture, blackTexture);
	}

    state->worldCamera = {};
    state->worldCamera.forward = World::Forward;

    state->worldCamera.fieldOfView = 60;
    state->worldCamera.nearClipPlane = 0.1f;
    state->worldCamera.farClipPlane = 1000.0f;
    state->worldCamera.aspectRatio = (real32)platformInfo->windowWidth / (real32)platformInfo->windowHeight;	

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
		auto characterMesh 				= LoadModel(&state->transientMemoryArena, "models/character.obj");
		auto characterMeshHandle 		= PushMesh(&characterMesh);
	
		state->character.position 		= {0, 0, 0};
		state->character.rotation 		= Quaternion::Identity();

		state->characterObjectHandle 	= PushRenderer(characterMeshHandle, state->materials.character);
		state->character.scale 			= 1;
	}


	// Environment
	{
		int environmentObjectCount 		= 5;

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
		state->environmentObjects[3]	= PushRenderer(pillarMeshHandle, state->materials.environment);
		state->environmentObjects[4]	= PushRenderer(pillarMeshHandle, state->materials.environment);

		state->environmentTransforms [1] = {-width / 4, -depth / 4, 0};
		state->environmentTransforms [2] = {width / 4, -depth / 4, 0};
		state->environmentTransforms [3] = {-width / 4, depth / 4, 0};
		state->environmentTransforms [4] = {width / 4, depth / 4, 0};

		Vector2 colliderSize = {4.0f, 4.0f};
		Vector2 halfColliderSize = colliderSize / 2.0f;
		state->staticColliders 			= PushArray<Rectangle>(&state->persistentMemoryArena, 4);
		state->staticColliders[0] 		= { Vector2{-width / 4, -depth / 4} - halfColliderSize, colliderSize};
		state->staticColliders[1] 		= { Vector2{width / 4, -depth / 4} - halfColliderSize, colliderSize};
		state->staticColliders[2] 		= { Vector2{-width / 4, depth / 4} - halfColliderSize, colliderSize};
		state->staticColliders[3] 		= { Vector2{width / 4, depth / 4} - halfColliderSize, colliderSize};
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
			state->persistentMemoryArena.Flush();

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

	/// Update Character
	Vector3 characterMovementVector;
	{
		real32 characterSpeed = 10;
		bool32 grounded = state->character.position.z < 0.1f;

		characterMovementVector = ProcessCharacterInput(input, &state->worldCamera);

		Vector3 characterNewPosition = state->character.position + characterMovementVector * characterSpeed * input->elapsedTime;

		// Collisions
		real32 characterCollisionRadius = 0.5f;
		Circle characterCollisionCircle = {characterNewPosition.x, characterNewPosition.y, characterCollisionRadius};

		auto collisionResult = GetCollisions(characterCollisionCircle, state->staticColliders);

		if (collisionResult.isCollision == false)
		{
			state->character.position = characterNewPosition;
		}


		if (grounded && input->jump.IsClicked())
		{
			state->character.zSpeed = 5;
		}

		state->character.position.z += state->character.zSpeed * input->elapsedTime;

		if (state->character.position.z > 0)
		{	
			state->character.zSpeed -= 2 * 9.81 * input->elapsedTime;
		}
		else
		{
			state->character.zSpeed = 0;
            state->character.position.z = 0;
		}

		
		real32 epsilon = 0.001f;
		if (Abs(input->move.x) > epsilon || Abs(input->move.y) > epsilon)
		{
			Vector3 characterForward = Normalize(characterMovementVector);
			real32 angleToWorldForward = SignedAngle(World::Forward, characterForward, World::Up);
			state->character.zRotationRadians = angleToWorldForward;
		}

		state->character.rotation = Quaternion::AxisAngle(World::Up, state->character.zRotationRadians);
	}

	/// Update network
	{
		// network->outPackage = {};
		// network->outPackage.characterPosition = state->character.position;
		// network->outPackage.characterRotation = state->character.rotation;
	}

	/// Update Camera
	{
		// Note(Leo): Update aspect ratio each frame, in case screen size has changed.
	    state->worldCamera.aspectRatio = (real32)platform->windowWidth / (real32)platform->windowHeight;

		real32 cameraRotateSpeed = 180;
		real32 cameraTumbleMin = -10;
		real32 cameraTumbleMax = 85;

		real32 relativeZoomSpeed = 0.1f;
		real32 zoomSpeed = state->cameraDistance;
		real32 minDistance = 5;
		real32 maxDistance = 100;

		Vector3 cameraOffsetFromTarget = World::Up * 2.0f;

		if (input->zoomIn.IsPressed())
		{
			// state->worldCamera.fieldOfView -= input->elapsedTime * 15;
			// state->worldCamera.fieldOfView = Max(state->worldCamera.fieldOfView, 10.0f);
			state->cameraDistance -= zoomSpeed * input->elapsedTime;
			state->cameraDistance = Max(state->cameraDistance, minDistance);
		}
		else if(input->zoomOut.IsPressed())
		{
			// state->worldCamera.fieldOfView += input->elapsedTime * 15;
			// state->worldCamera.fieldOfView = Min(state->worldCamera.fieldOfView, 100.0f);
			state->cameraDistance += zoomSpeed * input->elapsedTime;
			state->cameraDistance = Min(state->cameraDistance, maxDistance);
		}

	    state->cameraOrbitDegrees += input->look.x * cameraRotateSpeed * input->elapsedTime;
	    
	    state->cameraTumbleDegrees += input->look.y * cameraRotateSpeed * input->elapsedTime;
	    state->cameraTumbleDegrees = Clamp(state->cameraTumbleDegrees, cameraTumbleMin, cameraTumbleMax);

	    real32 cameraDistance = state->cameraDistance;
	    real32 cameraHorizontalDistance = Cosine(DegToRad * state->cameraTumbleDegrees) * cameraDistance;
	    Vector3 localPosition 
	    {
			Sine(DegToRad * state->cameraOrbitDegrees) * cameraHorizontalDistance,
			Cosine(DegToRad * state->cameraOrbitDegrees) * cameraHorizontalDistance,
			Sine(DegToRad * state->cameraTumbleDegrees) * cameraDistance
	    };


	    /*
	    Todo[Camera] (Leo): This is good effect, but its too rough like this,
	    make it good later when projections work

	    real32 cameraAdvanceAmount = 5;
	    Vector3 cameraAdvanceVector = characterMovementVector * cameraAdvanceAmount;
	    Vector3 cameraParentPosition = state->character.position + cameraAdvanceVector + cameraOffsetFromTarget;
	    */

	    Vector3 characterGroundedPosition = state->character.position;
	    characterGroundedPosition.z = 0;
	    Vector3 cameraParentPosition = characterGroundedPosition + cameraOffsetFromTarget;
	    
	    state->worldCamera.position = cameraParentPosition + localPosition;
		state->worldCamera.LookAt(cameraParentPosition);
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