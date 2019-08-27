/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"

// Note(Leo): Make unity build here
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

	real32 cameraOrbitDegrees;
	real32 cameraTumbleDegrees;
	real32 cameraDistance = 20.0f;

	Camera worldCamera;

	MaterialHandle characterMaterial;
	MaterialHandle sceneryMaterial;

	RenderedObjectHandle levelObjectHandle;
	RenderedObjectHandle characterObjectHandle;
	RenderedObjectHandle otherCharacterObjectHandle;

	int32 sceneryCount = 5;
	ArenaArray<Vector3>					sceneryPositions;
	ArenaArray<RenderedObjectHandle> 	sceneryObjectHandles;

	/// Memory
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

    TextureAsset textureAssets [] = {
        LoadTextureAsset("textures/chalet.jpg", &state->transientMemoryArena),
        LoadTextureAsset("textures/lava.jpg", &state->transientMemoryArena),
        LoadTextureAsset("textures/texture.jpg", &state->transientMemoryArena),
    };

	TextureHandle a = platformInfo->graphicsContext->PushTexture(&textureAssets[0]);
	TextureHandle b = platformInfo->graphicsContext->PushTexture(&textureAssets[1]);
	TextureHandle c = platformInfo->graphicsContext->PushTexture(&textureAssets[2]);

	GameMaterial mA = {0, a, a};
	GameMaterial mB = {0, b, c};

	state->characterMaterial = platformInfo->graphicsContext->PushMaterial(&mA);
	state->sceneryMaterial = platformInfo->graphicsContext->PushMaterial(&mB);

	state->characterPosition = {0, 0, 0};
	state->characterRotation = Quaternion::Identity();
   	state->otherCharacterPosition = {0, 0, 0};

    state->worldCamera = {};
    state->worldCamera.forward = CoordinateSystem::Forward;

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
	{
		auto levelMesh 				= GenerateMap(&state->transientMemoryArena);
		auto levelMeshHandle 		= platformInfo->graphicsContext->PushMesh(&levelMesh);
	    state->levelObjectHandle 	= platformInfo->graphicsContext->PushRenderedObject(levelMeshHandle, state->sceneryMaterial);
	}	

	// Scenery
    {
		state->sceneryPositions = PushArray<Vector3>(&state->persistentMemoryArena, state->sceneryCount);
		state->sceneryObjectHandles = PushArray<RenderedObjectHandle>(&state->persistentMemoryArena, state->sceneryCount);

	    Mesh treeMesh =	LoadModel(&state->transientMemoryArena, "models/tree.obj");
	    MeshHandle treeMeshHandle = platformInfo->graphicsContext->PushMesh(&treeMesh);

	    for (int i = 0; i < state->sceneryCount; ++i)
	    {
	    	state->sceneryObjectHandles[i] = platformInfo->graphicsContext->PushRenderedObject(treeMeshHandle, state->sceneryMaterial);
	    }

	    real32 radius = 15;
	    state->sceneryPositions[0] = {};
	    state->sceneryPositions[1] = {-radius, 0, 0};
	    state->sceneryPositions[2] = {0, radius, 0};
	    state->sceneryPositions[3] = {radius, 0, 0};
	    state->sceneryPositions[4] = {0, -radius, 0};
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

	// Todo(Leo): Input these
	real32 volume = 0.5f;

	for (int sampleIndex = 0; sampleIndex < frameCount; ++sampleIndex)
	{
		samples[sampleIndex].left = file.samples[0][fileSampleIndex] * volume;
		samples[sampleIndex].right = file.samples[1][fileSampleIndex] * volume;

		fileSampleIndex += 1;
		fileSampleIndex %= fileSampleCount;
	}
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
	{
		real32 characterSpeed = 6;

		Vector3 viewForward = state->worldCamera.forward;
		viewForward.z = 0;
		viewForward = Normalize(viewForward);
		Vector3 viewRight = Cross(viewForward, CoordinateSystem::Up);

		Vector3 viewAlignedInputVector = viewRight * input->move.x + viewForward * input->move.y;
		characterMovementVector = viewAlignedInputVector;

		state->characterPosition += viewAlignedInputVector * characterSpeed * input->timeDelta;

		bool32 grounded = state->characterPosition.z < 0.1f;

		if (grounded && input->jump == game::InputButtonState::WentDown)
		{
			state->characterZSpeed = 6;
		}

		state->characterPosition.z += state->characterZSpeed * input->timeDelta;

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
			real32 angleToWorldForward = SignedAngle(CoordinateSystem::Forward, characterForward);
			state->characterZRotationRadians = angleToWorldForward;
		}

		state->characterRotation = Quaternion::AxisAngle(CoordinateSystem::Up, state->characterZRotationRadians);
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

		Vector3 cameraOffsetFromTarget = CoordinateSystem::Up * 2.0f;

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
			= Matrix44::Translate(state->characterPosition) * state->characterRotation.ToRotationMatrix();

		if (network->isConnected)
		{
			outRenderInfo->modelMatrixArray[state->otherCharacterObjectHandle]
				= Matrix44::Translate(state->otherCharacterPosition) * state->otherCharacterRotation.ToRotationMatrix();
		}
		else
		{
			outRenderInfo->modelMatrixArray[state->otherCharacterObjectHandle] = {};
		}

		for (int sceneryIndex = 0; sceneryIndex < state->sceneryCount; ++sceneryIndex)
		{
			outRenderInfo->modelMatrixArray[state->sceneryObjectHandles[sceneryIndex]]
				= Matrix44::Translate(state->sceneryPositions[sceneryIndex]);
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
