#include <iostream>

#include "mazegame_platform.hpp"
#include "Camera.cpp"

struct GameState
{
	Vector3 characterPosition;

	real32 cameraOrbitDegrees;
	real32 cameraTumbleDegrees;
	real32 cameraDistance = 10.0f;

	Camera worldCamera;
};

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
		*state = {};

		memory->isInitialized = true;
		state->characterPosition = {0, 0, 0};

        state->worldCamera = {};

        state->worldCamera.position = {0, 10, -10};
        state->worldCamera.LookAt(vector3_zero);

        state->worldCamera.fieldOfView = 45;
        state->worldCamera.nearClipPlane = 0.1f;
        state->worldCamera.farClipPlane = 1000.0f;
        state->worldCamera.aspectRatio = (real32)platform->screenWidth / (real32)platform->screenHeight;
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
		real32 cameraTumbleMin = 5;
		real32 cameraTumbleMax = 80;

		// real32 zoomSpeed = 5;
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
	    outRenderInfo->characterMatrix = Matrix44::Translate(state->characterPosition);
	    
	    outRenderInfo->cameraView = state->worldCamera.ViewProjection();
	    outRenderInfo->cameraPerspective = state->worldCamera.PerspectiveProjection();
	}
}
