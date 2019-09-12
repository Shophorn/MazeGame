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

#include "AudioFile.cpp"
#include "MeshLoader.cpp"
#include "TextureLoader.cpp"


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
};

void
InitializeGameState(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	*state = {};

	state->persistentMemoryArena = CreateMemoryArena(
										reinterpret_cast<byte*>(memory->persistentMemory) + sizeof(GameState),
										memory->persistentMemorySize - sizeof(GameState));

	state->transientMemoryArena = CreateMemoryArena(
										reinterpret_cast<byte*>(memory->transientMemory),
										memory->transientMemorySize);

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

	// Characters
	{
		auto characterMesh 					= LoadModel(&state->transientMemoryArena, "models/character.obj");
		auto characterMeshHandle 			= platformInfo->graphicsContext->PushMesh(&characterMesh);

		state->characterObjectHandle 		= platformInfo->graphicsContext->PushRenderedObject(characterMeshHandle, state->characterMaterial);
		state->otherCharacterObjectHandle 	= platformInfo->graphicsContext->PushRenderedObject(characterMeshHandle, state->characterMaterial);
	}

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

    FlushArena(&state->transientMemoryArena);
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

extern "C" void
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
	}

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

		Vector3 characterNewPosition = state->characterPosition + viewAlignedInputVector * characterSpeed * input->timeDelta;

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

		bool32 jumped = false;
		if (grounded && input->jump == game::InputButtonState::WentDown)//IsPressed(input->jump))
		{
            // std::cout << "JUMPPPP\n";
            state->characterPosition.z = 0;
			state->characterZSpeed = 6;
			jumped = true;
		}

		state->characterPosition.z += state->characterZSpeed * input->timeDelta;

		if (jumped)
		{
			std::cout << state->characterPosition.z << ", dt: " << input->timeDelta << "\n";
		}

		if (state->characterPosition.z > 0)
		{	
			state->characterZSpeed -= 2 * 9.81 * input->timeDelta;
		}
		else
		{
			state->characterZSpeed = 0;
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
		if (IsPressed(input->start))
		{
			characterScale = 2.0f;
		}
		else if(IsPressed(input->select))
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

		if (input->zoomIn)
		{
			// state->worldCamera.fieldOfView -= input->timeDelta * 15;
			// state->worldCamera.fieldOfView = Max(state->worldCamera.fieldOfView, 10.0f);
			state->cameraDistance -= zoomSpeed * input->timeDelta;
			state->cameraDistance = Max(state->cameraDistance, minDistance);
		}
		else if(input->zoomOut)
		{
			// state->worldCamera.fieldOfView += input->timeDelta * 15;
			// state->worldCamera.fieldOfView = Min(state->worldCamera.fieldOfView, 100.0f);
			state->cameraDistance += zoomSpeed * input->timeDelta;
			state->cameraDistance = Min(state->cameraDistance, maxDistance);
		}

	    state->cameraOrbitDegrees += input->look.x * cameraRotateSpeed * input->timeDelta;
	    
	    state->cameraTumbleDegrees += input->look.y * cameraRotateSpeed * input->timeDelta;
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


	    Vector3 cameraParentPosition = state->characterPosition + cameraOffsetFromTarget;

	    
	    state->worldCamera.position = cameraParentPosition + localPosition;
		state->worldCamera.LookAt(cameraParentPosition);
	}

	/// Output Render info
	// Todo(Leo): Get info about limits of render output and constraint to those
	{
		outRenderInfo->modelMatrixArray[state->levelObjectHandle] = Matrix44::Identity();

		outRenderInfo->modelMatrixArray[state->characterObjectHandle]
			= Matrix44::Translate(state->characterPosition) 
				* state->characterRotation.ToRotationMatrix()
				* Matrix44::Scale(characterScale);

		if (network->isConnected)
		{
			outRenderInfo->modelMatrixArray[state->otherCharacterObjectHandle]
				= Matrix44::Translate(state->otherCharacterPosition) * state->otherCharacterRotation.ToRotationMatrix();
		}
		else
		{
			outRenderInfo->modelMatrixArray[state->otherCharacterObjectHandle] = {};
		}

		// Trees
		for (int sceneryIndex = 0; sceneryIndex < state->sceneryCount; ++sceneryIndex)
		{
			real32 treeScale = 4.0f;
			outRenderInfo->modelMatrixArray[state->sceneryObjectHandles[sceneryIndex]]
				= Matrix44::Translate(state->sceneryPositions[sceneryIndex])
				* Matrix44::Scale(treeScale);
		}

		// Ccamera
	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}

	/// Output sound
	{
		OutputSound(soundOutput->sampleCount, soundOutput->samples);
	}
}
