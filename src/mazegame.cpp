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


/// Note(Leo): These still use external libraries we may want to get rid of
#include "AudioFile.cpp"
#include "MeshLoader.cpp"
#include "TextureLoader.cpp"

struct GuiButton
{
	Vector2 position;
	Vector2 size;
};

struct GameState
{
	Vector3 characterPosition;
	real32 characterZSpeed;
	Quaternion characterRotation;
	real32 characterZRotationRadians;

	Vector3 otherCharacterPosition;
	Quaternion otherCharacterRotation;

	real32 cameraOrbitDegrees = 180;
	real32 cameraTumbleDegrees;
	real32 cameraDistance = 20.0f;

	Camera worldCamera;

	MaterialHandle characterMaterial;
	MaterialHandle sceneryMaterial;
	MaterialHandle terrainMaterial;

	RenderedObjectHandle levelObjectHandle;
	RenderedObjectHandle characterObjectHandle;
	RenderedObjectHandle otherCharacterObjectHandle;

	int32 sceneryCount;
	ArenaArray<Vector3>					sceneryPositions;
	ArenaArray<RenderedObjectHandle> 	sceneryObjectHandles;

	/* Note(Leo): MEMORY
	'persistentMemoryArena' is used to store things from frame to frame.
	'transientMemoryArena' is intended to be used in asset loading etc.*/
	MemoryArena persistentMemoryArena;
	MemoryArena transientMemoryArena;

	static constexpr int guiButtonCount = 4;
	GuiButton guiButtons [guiButtonCount];
	GuiHandle guiButtonHandles[guiButtonCount];

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

	state->characterPosition = {0, 0, 0};
	state->characterRotation = Quaternion::Identity();
   	state->otherCharacterPosition = {0, 0, 0};

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

		state->characterObjectHandle 		= platformInfo->graphicsContext->PushRenderedObject(characterMeshHandle, state->characterMaterial);
		state->otherCharacterObjectHandle 	= platformInfo->graphicsContext->PushRenderedObject(characterMeshHandle, state->characterMaterial);
	}

	// Scenery
    {
    	int worstCaseSceneryCount = levelMap.cellCountPerDirection * levelMap.cellCountPerDirection;
    	state->sceneryPositions = PushArray<Vector3>(&state->persistentMemoryArena, worstCaseSceneryCount);

    	int actualSceneryCount = 0;
    	for (int y = 0; y < levelMap.cellCountPerDirection; ++y)
    	{
    		for (int x = 0; x < levelMap.cellCountPerDirection; ++x)
    		{
    			if (levelMap.Cell(x, y) == 0)
    			{
    				real32 randomRange = 2.5f;

    				state->sceneryPositions[actualSceneryCount] = levelMap.GetCellPosition(x,y);
    				state->sceneryPositions[actualSceneryCount].x += (RandomValue() * 2 - 1) * randomRange;
    				state->sceneryPositions[actualSceneryCount].y += (RandomValue() * 2 - 1) * randomRange;

    				++actualSceneryCount;
    			}
    		}
    	}
    	ShrinkLastArray(&state->persistentMemoryArena, &state->sceneryPositions, actualSceneryCount);

    	state->sceneryObjectHandles = PushArray<RenderedObjectHandle>(&state->persistentMemoryArena, actualSceneryCount);

		// Note(Miida): Puiden läpi ei voi mennä, muahahahahahaaa
	    MeshAsset treeMesh 			= LoadModel(&state->transientMemoryArena, "models/tree.obj");
	    MeshHandle treeMeshHandle 	= platformInfo->graphicsContext->PushMesh(&treeMesh);

    	state->sceneryCount = actualSceneryCount;
	    for (int i = 0; i < state->sceneryCount; ++i)
	    {
	    	state->sceneryObjectHandles[i] = platformInfo->graphicsContext->PushRenderedObject(treeMeshHandle, state->sceneryMaterial);
	    }
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

	state->guiButtons[0] = {380, 200, 180, 40};
	state->guiButtons[1] = {380, 260, 180, 40};
	state->guiButtons[2] = {380, 320, 180, 40};
	state->guiButtons[3] = {380, 380, 180, 40};

	MeshAsset quadAsset = mesh_primitives::CreateQuad(&state->transientMemoryArena);
 	MeshHandle quadHandle = platformInfo->graphicsContext->PushMesh(&quadAsset);

	for (int guiButtonIndex = 0; guiButtonIndex < 4; ++guiButtonIndex)
	{
		state->guiButtonHandles[guiButtonIndex] = platformInfo->graphicsContext->PushGui(quadHandle, state->characterMaterial);
	}

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

enum MenuResult { MENU_NONE, MENU_EXIT, MENU_LOADLEVEL };

internal void 
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
			LoadMainLevel(state, memory, platform);
			state->levelLoaded = true;
		}
	}
	else
	{
		UpdateMainLevel(state, input, memory, platform, network, soundOutput, outRenderInfo);
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
			state->selectedGuiButtonIndex %= GameState::guiButtonCount;
		}

		if (input->up.IsClicked())
		{
			state->selectedGuiButtonIndex -= 1;
			if (state->selectedGuiButtonIndex < 0)
			{
				state->selectedGuiButtonIndex = GameState::guiButtonCount - 1;
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

		for (int  guiButtonIndex = 0; guiButtonIndex < 4; ++guiButtonIndex)
		{
			Vector2 guiTranslate = state->guiButtons[guiButtonIndex].position / halfWindowSize - 1.0f;
			Vector2 guiScale = state->guiButtons[guiButtonIndex].size / halfWindowSize;

			if (guiButtonIndex == state->selectedGuiButtonIndex)
			{
				guiTranslate = (state->guiButtons[guiButtonIndex].position - Vector2{15, 15}) / halfWindowSize - 1.0f;
				guiScale = (state->guiButtons[guiButtonIndex].size + Vector2{30, 30}) / halfWindowSize;
			}

			Matrix44 guiTransform = Matrix44::Translate({guiTranslate.x, guiTranslate.y, 0}) * Matrix44::Scale({guiScale.x, guiScale.y, 1.0});

			outRenderInfo->guiObjects[state->guiButtonHandles[guiButtonIndex]] = guiTransform;

		}

		// Ccamera
	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}

	return result;
}

internal void 
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
	real32 characterScale = 1;
	{
		real32 characterSpeed = 5;

		Vector3 viewForward = state->worldCamera.forward;
		viewForward.z = 0;
		viewForward = Normalize(viewForward);
		Vector3 viewRight = Cross(viewForward, World::Up);

		Vector3 viewAlignedInputVector = viewRight * input->move.x + viewForward * input->move.y;
		characterMovementVector = viewAlignedInputVector;

		Vector3 characterNewPosition = state->characterPosition + viewAlignedInputVector * characterSpeed * input->elapsedTime;

		// Collisions
		real32 treeCollisionRadius = 0.5f;
		real32 characterCollisionRadius = 0.5f;
		int treeCount = state->sceneryCount;

		bool32 collide = false;
		for (int treeIndex = 0; treeIndex < treeCount; ++treeIndex)
		{
			Vector2 characterPosition2 = size_cast<Vector2>(characterNewPosition);
			Vector2 treePosition2 = size_cast<Vector2>(state->sceneryPositions[treeIndex]);

			bool32 treeCollide = CircleCircleCollision(	characterPosition2, characterCollisionRadius,
														treePosition2, treeCollisionRadius);

			// Note(Leo): Do not reset on subsequent trees
			if (treeCollide)
			{
				collide = true;
			}
		}

		if (collide == false)
		{
			state->characterPosition = characterNewPosition;
		}

		bool32 grounded = state->characterPosition.z < 0.1f;

		if (grounded && input->jump.IsClicked())// == game::InputButtonState::WentDown)//IsPressed(input->jump))
		{
			state->characterZSpeed = 5;
		}

		state->characterPosition.z += state->characterZSpeed * input->elapsedTime;

		if (state->characterPosition.z > 0)
		{	
			state->characterZSpeed -= 2 * 9.81 * input->elapsedTime;
		}
		else
		{
			state->characterZSpeed = 0;
            state->characterPosition.z = 0;
		}

		
		real32 epsilon = 0.001f;
		if (Abs(input->move.x) > epsilon || Abs(input->move.y) > epsilon)
		{
			Vector3 characterForward = Normalize(viewAlignedInputVector);
			real32 angleToWorldForward = SignedAngle(World::Forward, characterForward, World::Up);
			state->characterZRotationRadians = angleToWorldForward;
		}

		state->characterRotation = Quaternion::AxisAngle(World::Up, state->characterZRotationRadians);

		// Test(Leo): start and select buttons
		if (input->start.IsPressed())
		{
			characterScale = 2.0f;
		}
		else if(input->select.IsPressed())
		{
			characterScale = 0.5f;
		}
	}

	/// Update network
	{
		network->outPackage = {};
		network->outPackage.characterPosition = state->characterPosition;
		network->outPackage.characterRotation = state->characterRotation;

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
	    Vector3 cameraParentPosition = state->characterPosition + cameraAdvanceVector + cameraOffsetFromTarget;
	    */

	    Vector3 characterGroundedPosition = state->characterPosition;
	    characterGroundedPosition.z = 0;
	    Vector3 cameraParentPosition = characterGroundedPosition + cameraOffsetFromTarget;
	    
	    state->worldCamera.position = cameraParentPosition + localPosition;
		state->worldCamera.LookAt(cameraParentPosition);
	}

	/// Output Render info
	// Todo(Leo): Get info about limits of render output array sizes and constraint to those
	{
		outRenderInfo->renderedObjects[state->levelObjectHandle] = Matrix44::Identity();

		outRenderInfo->renderedObjects[state->characterObjectHandle]
			= Matrix44::Translate(state->characterPosition) 
				* state->characterRotation.ToRotationMatrix()
				* Matrix44::Scale(characterScale);

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
		for (int sceneryIndex = 0; sceneryIndex < state->sceneryCount; ++sceneryIndex)
		{
			real32 treeScale = 4.0f;
			outRenderInfo->renderedObjects[state->sceneryObjectHandles[sceneryIndex]]
				= Matrix44::Translate(state->sceneryPositions[sceneryIndex])
				* Matrix44::Scale(treeScale);
		}

		// Ccamera
	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}
}