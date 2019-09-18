/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

// Note(Leo): Make unity build here
#include "Random.cpp"
#include "MapGenerator.cpp"
#include "Camera.cpp"
#include "CharacterSystems.cpp"

/// Note(Leo): These still use external libraries we may want to get rid of
#include "AudioFile.cpp"
#include "MeshLoader.cpp"
#include "TextureLoader.cpp"

struct Rectangle
{
	Vector2 position;
	Vector2 size;
};

struct GameState
{
	Character character;

	Vector3 otherCharacterPosition;
	Quaternion otherCharacterRotation;

	real32 cameraOrbitDegrees = 180;
	real32 cameraTumbleDegrees;
	real32 cameraDistance = 20.0f;

	Camera worldCamera;

	MaterialHandle characterMaterial;
	MaterialHandle sceneryMaterial;
	MaterialHandle terrainMaterial;
	MaterialHandle paletteMaterial;

	RenderedObjectHandle levelObjectHandle;
	RenderedObjectHandle characterObjectHandle;
	RenderedObjectHandle otherCharacterObjectHandle;

	int32 								treeCount;
	ArenaArray<Vector3>					treePositions;
	ArenaArray<Quaternion>				treeRotations;
	ArenaArray<real32>					treeScales;
	ArenaArray<RenderedObjectHandle> 	treeRenderHandles;
	real32 								treeCollisionRadius;


	int32 								keyholeCount;
	ArenaArray<Vector3> 				keyholePositions;
	ArenaArray<Quaternion>				keyholeRotations;
	ArenaArray<RenderedObjectHandle> 	keyholeRenderHandles;
	real32								keyholeCollisionRadius;

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
	        LoadTextureAsset("textures/chalet.jpg", &state->transientMemoryArena),
	        LoadTextureAsset("textures/lava.jpg", &state->transientMemoryArena),
	        LoadTextureAsset("textures/texture.jpg", &state->transientMemoryArena),
	        LoadTextureAsset("textures/PaletteColor.jpg", &state->transientMemoryArena),
	        LoadTextureAsset("textures/PaletteMetals.jpg", &state->transientMemoryArena),
	    };

		TextureHandle texA = platformInfo->graphicsContext->PushTexture(&textureAssets[0]);
		TextureHandle texB = platformInfo->graphicsContext->PushTexture(&textureAssets[1]);
		TextureHandle texC = platformInfo->graphicsContext->PushTexture(&textureAssets[2]);

		TextureHandle paletteColors = platformInfo->graphicsContext->PushTexture(&textureAssets[3]);
		TextureHandle paletteMetals = platformInfo->graphicsContext->PushTexture(&textureAssets[4]);

		MaterialAsset characterMaterialAsset = {MaterialType::Character, texB, texC, blackTexture};
		MaterialAsset sceneryMaterialAsset = {MaterialType::Terrain, texA, texA, blackTexture};
		MaterialAsset terrainMaterialAsset = {MaterialType::Terrain, texA, texA, whiteTexture};

		MaterialAsset paletteMaterialAsset = {MaterialType::Character, paletteColors, paletteMetals, blackTexture};

		state->characterMaterial = platformInfo->graphicsContext->PushMaterial(&characterMaterialAsset);
		state->sceneryMaterial = platformInfo->graphicsContext->PushMaterial(&sceneryMaterialAsset);
		state->terrainMaterial = platformInfo->graphicsContext->PushMaterial(&terrainMaterialAsset);
		state->paletteMaterial = platformInfo->graphicsContext->PushMaterial(&paletteMaterialAsset);
	}


    state->worldCamera = {};
    state->worldCamera.forward = World::Forward;

    state->worldCamera.fieldOfView = 60;
    state->worldCamera.nearClipPlane = 0.1f;
    state->worldCamera.farClipPlane = 1000.0f;
    state->worldCamera.aspectRatio = (real32)platformInfo->windowWidth / (real32)platformInfo->windowHeight;	


 	// Level
 	HexMap levelMap;
	{
		map::CreateInfo mapInfo = {};
		mapInfo.cellCountPerDirection = 60;
		mapInfo.cellSize = 5.0f;

		levelMap 					= map::GenerateMap(state->transientMemoryArena, mapInfo);
		auto levelMesh 				= map::GenerateMapMesh(state->transientMemoryArena, levelMap);
		auto levelMeshHandle 		= platformInfo->graphicsContext->PushMesh(&levelMesh);
	    state->levelObjectHandle 	= platformInfo->graphicsContext->PushRenderedObject(levelMeshHandle, state->terrainMaterial);
	}	

	// Characters
	{
		auto characterMesh 					= LoadModel(&state->transientMemoryArena, "models/character.obj");
		auto characterMeshHandle 			= platformInfo->graphicsContext->PushMesh(&characterMesh);
	
		state->character.position = {0, 0, 0};
		state->character.rotation = Quaternion::Identity();
	   	state->otherCharacterPosition = {0, 0, 0};

		state->characterObjectHandle 		= platformInfo->graphicsContext->PushRenderedObject(characterMeshHandle, state->characterMaterial);
		state->otherCharacterObjectHandle 	= platformInfo->graphicsContext->PushRenderedObject(characterMeshHandle, state->characterMaterial);
		state->character.scale = 1;
	}

	// Scenery
    {
		real32 randomRange 			= 2.5f;
    	int worstCaseSceneryCount 	= levelMap.cellCountPerDirection * levelMap.cellCountPerDirection;
    	state->treePositions 		= PushArray<Vector3>(&state->persistentMemoryArena, worstCaseSceneryCount);

    	int actualSceneryCount = 0;
    	for (int y = 0; y < levelMap.cellCountPerDirection; ++y)
    	{
    		for (int x = 0; x < levelMap.cellCountPerDirection; ++x)
    		{
    			if (levelMap.Cell(x, y) == 0)
    			{
    				state->treePositions[actualSceneryCount] = levelMap.GetCellPosition(x,y);
    				state->treePositions[actualSceneryCount].x += (RandomValue() * 2 - 1) * randomRange;
    				state->treePositions[actualSceneryCount].y += (RandomValue() * 2 - 1) * randomRange;

    				++actualSceneryCount;
    			}
    		}
    	}
    	ShrinkLastArray(&state->persistentMemoryArena, &state->treePositions, actualSceneryCount);


		// Note(Miida): Puiden läpi ei voi mennä, muahahahahahaaa
	    MeshAsset treeMesh 			= LoadModel(&state->transientMemoryArena, "models/tree.obj");
	    MeshHandle treeMeshHandle 	= platformInfo->graphicsContext->PushMesh(&treeMesh);

    	state->treeCount = actualSceneryCount;
    	state->treeRenderHandles 	= PushArray<RenderedObjectHandle>(&state->persistentMemoryArena, state->treeCount);
    	state->treeRotations 		= PushArray<Quaternion>(&state->persistentMemoryArena, state->treeCount);
	    state->treeScales 			= PushArray<real32>(&state->persistentMemoryArena, state->treeCount);
	    for (int treeIndex = 0; treeIndex < state->treeCount; ++treeIndex)
	    {
	    	real32 rotation = RandomValue() * 360.0f;
	    	state->treeRotations[treeIndex] = Quaternion::AxisAngle(World::Up, rotation);

	    	state->treeScales[treeIndex] = RandomRange(3.5, 4.5);

	    	state->treeRenderHandles[treeIndex] = platformInfo->graphicsContext->PushRenderedObject(treeMeshHandle, state->sceneryMaterial);
	    }
	    state->treeCollisionRadius = 0.5f;

	    /// ----- Keyholes -----
	    real32 keyholeProbability 	= 0.1f;
	    int32 worstCaseKeyholeCount = worstCaseSceneryCount;
	    state->keyholePositions 	= PushArray<Vector3>(&state->persistentMemoryArena, worstCaseKeyholeCount);

	    int32 actualKeyholeCount = 0;
	    for (int y = 0; y < levelMap.cellCountPerDirection; ++y)
	    {
	  		for (int x = 0; x < levelMap.cellCountPerDirection; ++x)
	  		{
				if (levelMap.Cell(x, y) == 1 && RandomValue() < keyholeProbability)
				{
     				state->keyholePositions[actualKeyholeCount] = levelMap.GetCellPosition(x,y);
    				state->keyholePositions[actualKeyholeCount].x += (RandomValue() * 2 - 1) * randomRange;
    				state->keyholePositions[actualKeyholeCount].y += (RandomValue() * 2 - 1) * randomRange;

    				++actualKeyholeCount;	
				}
	  		}  	
	    }
    	ShrinkLastArray(&state->persistentMemoryArena, &state->keyholePositions, actualKeyholeCount);

	    MeshAsset keyholeMesh 			= LoadModel(&state->transientMemoryArena, "models/keyhole.obj");
	    MeshHandle keyholeMeshHandle 	= platformInfo->graphicsContext->PushMesh(&keyholeMesh);

    	state->keyholeCount 		= actualKeyholeCount;
    	state->keyholeRotations 	= PushArray<Quaternion>(&state->persistentMemoryArena, state->keyholeCount);
    	state->keyholeRenderHandles = PushArray<RenderedObjectHandle>(&state->persistentMemoryArena, state->keyholeCount);
	    for (int keyholeIndex = 0; keyholeIndex < state->keyholeCount; ++keyholeIndex)
	    {
			real32 rotation = RandomValue() * 360.0f;
			state->keyholeRotations[keyholeIndex] = Quaternion::AxisAngle(World::Up, rotation);

	    	state->keyholeRenderHandles[keyholeIndex] = platformInfo->graphicsContext->PushRenderedObject(keyholeMeshHandle, state->paletteMaterial);
	    }

	    state->keyholeCollisionRadius = 0.1f;
 	}

	state->gameGuiButtonCount = 2;
	state->gameGuiButtons = PushArray<Rectangle>(&state->persistentMemoryArena, state->gameGuiButtonCount);
	state->gameGuiButtonHandles = PushArray<GuiHandle>(&state->persistentMemoryArena, state->gameGuiButtonCount);

	state->gameGuiButtons[0] = {380, 200, 180, 40};
	state->gameGuiButtons[1] = {380, 260, 180, 40};

	MeshAsset quadAsset = mesh_primitives::CreateQuad(&state->transientMemoryArena);
 	MeshHandle quadHandle = platformInfo->graphicsContext->PushMesh(&quadAsset);

	for (int guiButtonIndex = 0; guiButtonIndex < state->gameGuiButtonCount; ++guiButtonIndex)
	{
		state->gameGuiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, state->characterMaterial);
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
	        LoadTextureAsset("textures/chalet.jpg", &state->transientMemoryArena),
	        LoadTextureAsset("textures/lava.jpg", &state->transientMemoryArena),
	        LoadTextureAsset("textures/texture.jpg", &state->transientMemoryArena),
	    };

		TextureHandle texA = platformInfo->graphicsContext->PushTexture(&textureAssets[0]);
		TextureHandle texB = platformInfo->graphicsContext->PushTexture(&textureAssets[1]);
		TextureHandle texC = platformInfo->graphicsContext->PushTexture(&textureAssets[2]);

		MaterialAsset characterMaterialAsset = {MaterialType::Character, texB, texC, blackTexture};
		MaterialAsset sceneryMaterialAsset = {MaterialType::Terrain, texA, texA, blackTexture};
		MaterialAsset terrainMaterialAsset = {MaterialType::Terrain, texA, texA, whiteTexture};

		state->characterMaterial = platformInfo->graphicsContext->PushMaterial(&characterMaterialAsset);
		state->sceneryMaterial = platformInfo->graphicsContext->PushMaterial(&sceneryMaterialAsset);
		state->terrainMaterial = platformInfo->graphicsContext->PushMaterial(&terrainMaterialAsset);
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
		state->menuGuiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, state->characterMaterial);
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

internal bool32
CircleCircleCollision(Vector2 pointA, real32 radiusA, Vector2 pointB, real32 radiusB)
{
	real32 distanceThreshold = radiusA + radiusB;
	real32 sqrDistanceThreshold = distanceThreshold * distanceThreshold;

	real32 sqrDistance = SqrLength (pointA - pointB);

	bool32 result = sqrDistance < sqrDistanceThreshold;
	return result;
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

	game::UpdateResult result = { false };
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
	/// Update Character
	Vector3 characterMovementVector;
	{
		real32 characterSpeed = 10;
		bool32 grounded = state->character.position.z < 0.1f;

		Vector3 viewForward = state->worldCamera.forward;
		viewForward.z = 0;
		viewForward = Normalize(viewForward);
		Vector3 viewRight = Cross(viewForward, World::Up);

		Vector3 viewAlignedInputVector = viewRight * input->move.x + viewForward * input->move.y;
		characterMovementVector = viewAlignedInputVector;

		real32 speed = grounded ? characterSpeed : characterSpeed / 2.0f;
		Vector3 characterNewPosition = state->character.position + viewAlignedInputVector * speed * input->elapsedTime;

		// Collisions
		real32 characterCollisionRadius = 0.5f;
		Vector2 characterPosition2 = size_cast<Vector2>(characterNewPosition);

		bool32 collide = false;
		for (int treeIndex = 0; treeIndex < state->treeCount; ++treeIndex)
		{
			Vector2 treePosition2 = size_cast<Vector2>(state->treePositions[treeIndex]);
			bool32 treeCollide = CircleCircleCollision(	characterPosition2, characterCollisionRadius,
														treePosition2, state->treeCollisionRadius);

			// Note(Leo): Do not reset on subsequent trees
			if (treeCollide)
			{
				collide = true;
			}
		}

		if (collide == false)
		{
			for (int keyholeIndex = 0; keyholeIndex < state->keyholeCount; ++keyholeIndex)
			{
				Vector2 keyholePosition2 = size_cast<Vector2>(state->keyholePositions[keyholeIndex]);
				bool32 keyholeCollide = CircleCircleCollision(	characterPosition2, characterCollisionRadius,
															keyholePosition2, state->keyholeCollisionRadius);

				// Note(Leo): Do not reset on subsequent keyholes
				if (keyholeCollide)
				{
					collide = true;
				}
			}
		}

		if (collide == false)
		{
			state->character.position = characterNewPosition;
		}


		if (grounded && input->jump.IsClicked())// == game::InputButtonState::WentDown)//IsPressed(input->jump))
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
			Vector3 characterForward = Normalize(viewAlignedInputVector);
			real32 angleToWorldForward = SignedAngle(World::Forward, characterForward, World::Up);
			state->character.zRotationRadians = angleToWorldForward;
		}

		state->character.rotation = Quaternion::AxisAngle(World::Up, state->character.zRotationRadians);
	}

	/// Update network
	{
		network->outPackage = {};
		network->outPackage.characterPosition = state->character.position;
		network->outPackage.characterRotation = state->character.rotation;

		state->otherCharacterPosition = network->inPackage.characterPosition;
		state->otherCharacterRotation = network->inPackage.characterRotation;
	}

	/// Update Camera
	{
		// Note(Leo): Update aspect ratio each frame, in case screen size has changed.
	    state->worldCamera.aspectRatio = (real32)platform->windowWidth / (real32)platform->windowHeight;

		real32 cameraRotateSpeed = 90;
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
		outRenderInfo->renderedObjects[state->levelObjectHandle] = Matrix44::Identity();

		outRenderInfo->renderedObjects[state->characterObjectHandle]
			= Matrix44::Translate(state->character.position) 
			* state->character.rotation.ToRotationMatrix()
			* Matrix44::Scale(state->character.scale);

		if (network->isConnected)
		{
			outRenderInfo->renderedObjects[state->otherCharacterObjectHandle]
				= Matrix44::Translate(state->otherCharacterPosition) * state->otherCharacterRotation.ToRotationMatrix();
		}
		else
		{
			outRenderInfo->renderedObjects[state->otherCharacterObjectHandle] = {};
		}

		// Trees
		for (int treeIndex = 0; treeIndex < state->treeCount; ++treeIndex)
		{
			outRenderInfo->renderedObjects[state->treeRenderHandles[treeIndex]]
				= Matrix44::Translate(state->treePositions[treeIndex])
				* state->treeRotations[treeIndex].ToRotationMatrix()
				* Matrix44::Scale(state->treeScales[treeIndex]);
		}


		for (int keyholeIndex = 0; keyholeIndex < state->keyholeCount; ++keyholeIndex)
		{
			outRenderInfo->renderedObjects[state->keyholeRenderHandles[keyholeIndex]]
				= Matrix44::Translate(state->keyholePositions[keyholeIndex])
				* state->keyholeRotations[keyholeIndex].ToRotationMatrix();
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