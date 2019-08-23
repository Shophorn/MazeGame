/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Camera.cpp"
#include "vertex_data.cpp"
#include "AudioFile.cpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>


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

	MeshHandle levelMeshHandle;
	MeshHandle characterMeshHandle;
	MeshHandle otherCharacterMeshHandle;

	int32 sceneryCount = 5;
	MeshHandle sceneryMeshHandles [5];
	Vector3 sceneryPositions [5];

	MaterialHandle characterMaterial;
	MaterialHandle sceneryMaterial;
};

internal TextureAsset
LoadTextureAsset(const char * assetPath)
{
    TextureAsset resultTexture = {};
    stbi_uc * pixels = stbi_load(assetPath, &resultTexture.width, &resultTexture.height,
                                        &resultTexture.channels, STBI_rgb_alpha);

    if(pixels == nullptr)
    {
        // Todo[Error](Leo): Proper handling and logging
        throw std::runtime_error("Failed to load image");
    }

    // rgba channels
    resultTexture.channels = 4;

    int32 pixelCount = resultTexture.width * resultTexture.height;
    resultTexture.pixels.resize(pixelCount);

    uint64 imageMemorySize = resultTexture.width * resultTexture.height * resultTexture.channels;
    memcpy((uint8*)resultTexture.pixels.data(), pixels, imageMemorySize);

    stbi_image_free(pixels);
    return resultTexture;
}


void
InitializeGameState(GameState * state, GameMemory * memory, GamePlatformInfo * platform)
{
    TextureAsset textureAssets [] = {
        LoadTextureAsset("textures/chalet.jpg"),
        LoadTextureAsset("textures/lava.jpg"),
        LoadTextureAsset("textures/texture.jpg"),
    };

	TextureHandle a = memory->PushTexture(memory->graphicsContext, &textureAssets[0]);
	TextureHandle b = memory->PushTexture(memory->graphicsContext, &textureAssets[1]);
	TextureHandle c = memory->PushTexture(memory->graphicsContext, &textureAssets[2]);

	GameMaterial mA = {0, a, a};
	GameMaterial mB = {0, b, c};

	state->characterMaterial = memory->PushMaterial(memory->graphicsContext, &mA);
	state->sceneryMaterial = memory->PushMaterial(memory->graphicsContext, &mB);

	memory->ApplyGraphicsContext(memory->graphicsContext);

	*state = {};

	state->characterPosition = {0, 0, 0};
	state->characterRotation = Quaternion::Identity();
   	state->otherCharacterPosition = {0, 0, 0};

    state->worldCamera = {};

    // Note(Leo): We probably wont need these as camera is updated in first frame already
    // state->worldCamera.position = {0, 10, -10};
    state->worldCamera.forward = CoordinateSystem::Forward;

    state->worldCamera.fieldOfView = 60;
    state->worldCamera.nearClipPlane = 0.1f;
    state->worldCamera.farClipPlane = 1000.0f;
    state->worldCamera.aspectRatio = (real32)platform->screenWidth / (real32)platform->screenHeight;	

    {
	    Vector3 testNear = {0, 0, state->worldCamera.nearClipPlane};
	    Vector3 testFar = {0, 0, state->worldCamera.farClipPlane};

	    auto cameraProjection = state->worldCamera.PerspectiveProjection();
	    std::cout << "test near: " << cameraProjection * testNear << "\ntest far: " << cameraProjection * testFar << "\n"; 
	    std::cout << "camera projection: " << cameraProjection << "\n\n";
    }

    Mesh meshes [] = 
    {
    	GenerateMap(),
    	LoadModel("models/character.obj"),
	};

	state->levelMeshHandle 			= memory->PushMesh(memory->graphicsContext, &meshes[0]);
	state->characterMeshHandle 		= memory->PushMesh(memory->graphicsContext, &meshes[1]);
	state->otherCharacterMeshHandle = memory->PushMesh(memory->graphicsContext, &meshes[1]);

	Mesh sceneryMeshes [] =
	{
		LoadModel("models/pillar.obj"),			// Center
		LoadModel("models/pillar.obj"),			// left
    	LoadModel("models/tree.obj"),			// front
    	LoadModel("models/character.obj"),		// right
    	MeshPrimitives::cube 					// back
    };

    for (int sceneryIndex = 0; sceneryIndex < state->sceneryCount; ++sceneryIndex)
    {
    	state->sceneryMeshHandles[sceneryIndex] = memory->PushMesh(memory->graphicsContext, &sceneryMeshes[sceneryIndex]);
    }

    memory->ApplyGraphicsContext(memory->graphicsContext);

    real32 radius = 15;

    state->sceneryPositions[0] = {};

    state->sceneryPositions[1] = {};
    state->sceneryPositions[1].right = -radius;
 
    state->sceneryPositions[2] = {};
    state->sceneryPositions[2].forward = radius;
 
    state->sceneryPositions[3] = {};
    state->sceneryPositions[3].right = radius;
 
    state->sceneryPositions[4] = {};
    state->sceneryPositions[4].forward = -radius;
    state->sceneryPositions[4].up = 2;
}

void
OutputSound(int frameCount, GameStereoSoundSample * samples)
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
	GameInput * 		input,
	GameMemory * 		memory,
	GamePlatformInfo * 	platform,
	GameNetwork	*		network,
	GameSoundOutput * 	soundOutput,

	GameRenderInfo * 	outRenderInfo
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

		if (grounded && input->jump == GAME_BUTTON_WENT_DOWN)
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
			// std::cout 
			// 	<< characterForward 
			// 	<< " || " << state->characterPosition 
			// 	<< " || " << viewForward
			// 	<< " || " << viewRight 
			// 	<< " || " << state->worldCamera.position << "\n";

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
	// Todo[camera] (Leo): view projection seems wrong!!!!!
	{
		// Note(Leo): Update aspect ratio each frame, in case screen size has changed.
	    state->worldCamera.aspectRatio = (real32)platform->screenWidth / (real32)platform->screenHeight;

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
			state->worldCamera.fieldOfView -= input->timeDelta * 15;
			state->worldCamera.fieldOfView = Max(state->worldCamera.fieldOfView, 10.0f);
			// state->cameraDistance -= zoomSpeed * input->timeDelta;
			// state->cameraDistance = Max(state->cameraDistance, minDistance);
		}
		else if(input->zoomOut)
		{
			state->worldCamera.fieldOfView += input->timeDelta * 15;
			state->worldCamera.fieldOfView = Min(state->worldCamera.fieldOfView, 100.0f);
			// state->cameraDistance += zoomSpeed * input->timeDelta;
			// state->cameraDistance = Min(state->cameraDistance, maxDistance);
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
	    make it good whren projections work

	    real32 cameraAdvanceAmount = 5;
	    Vector3 cameraAdvanceVector = characterMovementVector * cameraAdvanceAmount;
	    Vector3 cameraParentPosition = state->characterPosition + cameraAdvanceVector + cameraOffsetFromTarget;
	    */


	    Vector3 cameraParentPosition = state->characterPosition + cameraOffsetFromTarget;

	    
	    state->worldCamera.position = cameraParentPosition + localPosition;

		state->worldCamera.LookAt(cameraParentPosition);
	}

	/// Output Render info
	{
		outRenderInfo->modelMatrixArray[state->levelMeshHandle] = Matrix44::Identity();

		outRenderInfo->modelMatrixArray[state->characterMeshHandle]
			= Matrix44::Translate(state->characterPosition) * state->characterRotation.ToRotationMatrix();

		if (network->isConnected)
		{
			outRenderInfo->modelMatrixArray[state->otherCharacterMeshHandle]
				= Matrix44::Translate(state->otherCharacterPosition) * state->otherCharacterRotation.ToRotationMatrix();
		}
		else
		{
			outRenderInfo->modelMatrixArray[state->otherCharacterMeshHandle] = {};
		}

		for (int sceneryIndex = 0; sceneryIndex < state->sceneryCount; ++sceneryIndex)
		{
			outRenderInfo->modelMatrixArray[state->sceneryMeshHandles[sceneryIndex]]
				= Matrix44::Translate(state->sceneryPositions[sceneryIndex]);
		}

	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}

	/// Output sound
	{
		OutputSound(soundOutput->sampleCount, soundOutput->samples);
	}
}
