#include <iostream>

#include "mazegame_platform.hpp"
#include "Camera.cpp"
#include "vertex_data.cpp"

struct GameState
{
	Vector3 characterPosition;

	real32 cameraOrbitDegrees;
	real32 cameraTumbleDegrees;
	real32 cameraDistance = 20.0f;

	Camera worldCamera;

	MeshHandle levelMeshHandle;
	MeshHandle characterMeshHandle;
	MeshHandle sceneryMeshHandles [2];

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


    Mesh levelMesh = GenerateMap();
    Mesh characterMesh = LoadModel("models/character.obj");
    
    state->levelMeshHandle = memory->PushMesh(&levelMesh);
    state->characterMeshHandle = memory->PushMesh(&characterMesh);
    
    Mesh sceneryMeshes [] = 
    {
    	LoadModel("models/tree.obj"),
    	LoadModel("models/pillar.obj")
    };

    state->sceneryMeshHandles[0] = memory->PushMesh(&sceneryMeshes[0]);
    state->sceneryMeshHandles[1] = memory->PushMesh(&sceneryMeshes[1]);
}

void
GameUpdateAndRender(
	GameInput * 		input,
	GameMemory * 		memory,
	GamePlatformInfo * 	platform,

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
		real32 characterSpeed = 5;

		Vector3 viewForward = state->worldCamera.forward;
		viewForward.y = 0;
		viewForward = Normalize(viewForward);
		Vector3 viewRight = Cross(viewForward, vector3_world_up);

		state->characterPosition += viewRight * input->move.x * characterSpeed * input->timeDelta;
		state->characterPosition += viewForward * input->move.y * characterSpeed * input->timeDelta;
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

		outRenderInfo->modelMatrixArray[state->sceneryMeshHandles[0]] = Matrix44::Identity();
		outRenderInfo->modelMatrixArray[state->sceneryMeshHandles[1]] = Matrix44::Translate({5, 0, 0});

	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}
}
