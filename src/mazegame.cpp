/*=============================================================================
Leo Tamminen

:MAZEGAME: project's main file.
=============================================================================*/
#include <iostream>

#include "MazegamePlatform.hpp"
#include "Camera.cpp"
#include "vertex_data.cpp"

struct GameState
{
	Vector3 characterPosition;
	Vector3 otherCharacterPosition;

	real32 cameraOrbitDegrees;
	real32 cameraTumbleDegrees;
	real32 cameraDistance = 20.0f;

	Camera worldCamera;

	MeshHandle levelMeshHandle;
	MeshHandle characterMeshHandle;
	MeshHandle otherCharacterMeshHandle;
	MeshHandle sceneryMeshHandles [16];

	Vector3 sceneryPositions [16];

	MeshHandle cubeHandle;
	Vector3 cubePosition;
	Vector3 cubeScale;
};

void
InitializeGameState(GameState * state, GameMemory * memory, GamePlatformInfo * platform)
{
	*state = {};

	state->characterPosition = {0, 0, 0};

    state->worldCamera = {};

    state->worldCamera.position = {0, 10, -10};
    state->worldCamera.LookAt(vector3_zero);

    state->worldCamera.fieldOfView = 60;
    state->worldCamera.nearClipPlane = 0.1f;
    state->worldCamera.farClipPlane = 1000.0f;
    state->worldCamera.aspectRatio = (real32)platform->screenWidth / (real32)platform->screenHeight;	

    Mesh meshes [] = 
    {
    	GenerateMap(),
    	LoadModel("models/character.obj"),
    	LoadModel("models/tree.obj"),

    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj"),
    	LoadModel("models/pillar.obj")
    };
    MeshHandle handles[18] = {};
    memory->PushMeshes(18, meshes, handles);

	state->levelMeshHandle = handles[0];
    state->characterMeshHandle = handles[1];

    int32 sceneryCount = 16;
    int32 sceneryCountOffset = 2;
    for (int i = 0; i < sceneryCount; ++i)
    {
    	state->sceneryMeshHandles[i] = handles[i + sceneryCountOffset];
    }

    state->sceneryPositions[0] = {0, 0, 0};

    int32 pillarCount = 15;
    real32 pillarRadius = 15;
    real32 pillarAngleStep = 360.0f / pillarCount;
    int32 pillarCounOffset = 1;

    for (int i = 0; i < pillarCount; ++i)
    {
    	real32 angle = DegToRad * pillarAngleStep * i;
    	real32 x = Sine(angle) * pillarRadius;
    	real32 z = Cosine(angle) * pillarRadius;
    	state->sceneryPositions[i + pillarCounOffset]= {x, 0, z};
    }

   	memory->PushMeshes(1, &MeshPrimitives::cube, &state->cubeHandle);
   	state->cubePosition = {0, 5, 0};
   	state->cubeScale = {1, 1, 1};

   	memory->PushMeshes(1, &meshes[1], &state->otherCharacterMeshHandle);
   	state->otherCharacterPosition = {0, 0, 0};
}

void
GameUpdateAndRender(
	GameInput * 		input,
	GameMemory * 		memory,
	GamePlatformInfo * 	platform,
	GameNetwork	*		network,

	GameRenderInfo * 	outRenderInfo
){
	GameState * state = reinterpret_cast<GameState*>(memory->persistentMemory);

	if (memory->isInitialized == false)
	{
		InitializeGameState (state, memory, platform);
		memory->isInitialized = true;
	}

	// Update Character
	{
		real32 characterSpeed = 8;

		Vector3 viewForward = state->worldCamera.forward;
		viewForward.y = 0;
		viewForward = Normalize(viewForward);
		Vector3 viewRight = Cross(viewForward, vector3_world_up);

		state->characterPosition += viewRight * input->move.x * characterSpeed * input->timeDelta;
		state->characterPosition += viewForward * input->move.y * characterSpeed * input->timeDelta;

		if (state->characterPosition.x > 0)
		{
			state->cubeScale = {1, 1, 20};
		}
		else
		{
			state->cubeScale = {1, 1, 1};
		}

	}

	// Update network
	{
		network->characterPosition = state->characterPosition;
		state->otherCharacterPosition = network->otherCharacterPosition;
	}

	// Update Camera
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

		Vector3 cameraBasePosition = {0, 2.0f, 0};

		if (input->zoomIn)
		{
			state->cameraDistance -= zoomSpeed * input->timeDelta;
			state->cameraDistance = Max(state->cameraDistance, minDistance);
		}
		else if(input->zoomOut)
		{
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
			Cosine(DegToRad * state->cameraOrbitDegrees) * cameraHorizontalDistance,
			Sine(DegToRad * state->cameraTumbleDegrees) * cameraDistance,
			Sine(DegToRad * state->cameraOrbitDegrees) * cameraHorizontalDistance
	    };

	    state->worldCamera.position = state->characterPosition + cameraBasePosition + localPosition;

		state->worldCamera.LookAt(state->characterPosition);
	}

	// Output Render info
	{
		outRenderInfo->modelMatrixArray[state->levelMeshHandle] = Matrix44::Identity();
		outRenderInfo->modelMatrixArray[state->characterMeshHandle] = Matrix44::Translate(state->characterPosition);
		outRenderInfo->modelMatrixArray[state->otherCharacterMeshHandle] = Matrix44::Translate(state->otherCharacterPosition);

		int32 sceneryCount = 16;
		for (int i = 0; i < sceneryCount; ++i)
		{
			outRenderInfo->modelMatrixArray[state->sceneryMeshHandles[i]] = Matrix44::Translate(state->sceneryPositions[i]);
		}

		auto cubeTransform = Matrix44::Transform(state->cubePosition, state->cubeScale);
		outRenderInfo->modelMatrixArray[state->cubeHandle] = cubeTransform;

	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}
}
