/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

// Note(Leo): Make unity build here.
#include "Handle.cpp"
#include "Random.cpp"
#include "MapGenerator.cpp"
#include "Transform3D.cpp"
#include "Animator.cpp"
#include "Camera.cpp"
#include "Collisions2D.cpp"
#include "CollisionManager2D.cpp"
#include "Collisions3D.cpp"
#include "Skybox.cpp"

#include "CharacterSystems.cpp"
#include "CameraController.cpp"
#include "RenderingSystem.cpp"

/// Note(Leo): These still use external libraries we may want to get rid of
#include "Files.cpp"
#include "AudioFile.cpp"
#include "MeshLoader.cpp"
#include "TextureLoader.cpp"

#include "Scene.cpp"
#include "Scene2D.cpp"
#include "Scene3D.cpp"

#include "MenuSystem.cpp"
#include "DefaultSceneGui.cpp"


// Note(Leo): This makes less sense as a 'state' now that we have 'Scene' struct
struct GameState
{
	/* Note(Leo): MEMORY
	'persistentMemoryArena' is used to store things from frame to frame.
	'transientMemoryArena' is intended to be used in asset loading etc. and is flushed each frame*/
	MemoryArena persistentMemoryArena;
	MemoryArena transientMemoryArena;

	// Todo(Leo): Maybe store game pointer to info struct??
	SceneInfo loadedSceneInfo;
	void * loadedScene;

	SceneGuiInfo loadedGuiInfo;
	void * loadedGui;

	bool32 sceneLoaded = false;
};

internal void
initialize_game_state(GameState * state, game::Memory * memory, game::PlatformInfo * platformInfo)
{
	*state = {};

	// // Note(Leo): Create persistent arena in the same memoryblock as game state, right after it.
	uint64 gameStateSize 			= align_up_to(sizeof(GameState), MemoryArena::defaultAlignment);
	byte * persistentMemory 		= reinterpret_cast<byte *> (memory->persistentMemory) + gameStateSize;
	uint64 persistentMemorySize 	= memory->persistentMemorySize - gameStateSize;
	state->persistentMemoryArena 	= make_memory_arena(persistentMemory, persistentMemorySize); 

	byte * transientMemory 			= reinterpret_cast<byte*>(memory->transientMemory);
	uint64 transientMemorySize 		= memory->transientMemorySize;
	state->transientMemoryArena 	= make_memory_arena(transientMemory, transientMemorySize);
}

internal void
output_sound(int frameCount, game::StereoSoundSample * samples)
{
	// Note(Leo): Shit these are bad :DD
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

internal void
load_scene(GameState * state, game::PlatformInfo * platform, SceneInfo scene)
{
	state->loadedSceneInfo = scene;

	state->loadedScene = reserve_from_memory_arena(&state->persistentMemoryArena, scene.get_alloc_size());
	scene.load(state->loadedScene, &state->persistentMemoryArena, &state->transientMemoryArena, platform);

	state->sceneLoaded = true;	
	platform->graphicsContext->Apply();
}

internal void
load_gui(GameState * state, game::PlatformInfo * platform, SceneGuiInfo gui)
{
	state->loadedGuiInfo = gui;

	state->loadedGui = reserve_from_memory_arena(&state->persistentMemoryArena, gui.get_alloc_size());
	gui.load(state->loadedGui, &state->persistentMemoryArena, &state->transientMemoryArena, platform);	
	
	platform->graphicsContext->Apply();
}

internal void
unload_scene_and_gui(GameState * state, game::PlatformInfo * platform)
{
	state->loadedSceneInfo = {};

	platform->graphicsContext->UnloadAll();
	clear_memory_arena(&state->persistentMemoryArena);

	state->loadedScene = nullptr;
	state->loadedGui = nullptr;

	state->sceneLoaded = false;
}

extern "C" game::UpdateResult
GameUpdate(
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platform,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,

	// Todo(Leo): extra stupid name, but 'renderer' is also problematic, maybe combine with graphics context
	game::RenderInfo * 		outRenderInfo)
{
	/* Note(Leo): This is reinterpreted each frame, we don't know and don't care
	if it has been moved or whatever in platform layer*/
	GameState * state = reinterpret_cast<GameState*>(memory->persistentMemory);

	if (memory->isInitialized == false)
	{
		initialize_game_state (state, memory, platform);
		memory->isInitialized = true;

		load_gui(state, platform, menuSceneGuiGui);
	}
	flush_memory_arena(&state->transientMemoryArena);

	if (state->loadedScene != nullptr)
	{
		state->loadedSceneInfo.update(state->loadedScene, input, outRenderInfo, platform);
	}

	// Note(Leo): This is just a reminder
	// Todo(Leo): Remove if unnecessary
	/// Update network
	{
		// network->outPackage = {};
		// network->outPackage.characterPosition = state->character.position;
		// network->outPackage.characterRotation = state->character.rotation;
	}

	game::UpdateResult result = {};
	result.exit = false;
	
	switch(state->loadedGuiInfo.update(state->loadedGui, input, outRenderInfo, platform))
	{
		case MENU_EXIT:
			result = { true };
			break;

		case MENU_LOADLEVEL_2D:
			unload_scene_and_gui(state, platform);
			load_scene(state, platform, scene2dInfo);
			load_gui(state, platform, defaultSceneGui);
			break;

		case MENU_LOADLEVEL_3D:
			unload_scene_and_gui(state, platform);
			load_scene(state, platform, scene3dInfo);
			load_gui(state, platform, defaultSceneGui);
			break;

		case SCENE_EXIT:
			unload_scene_and_gui(state, platform);
			load_gui(state, platform, menuSceneGuiGui);
			break;

		default:
			break;
	}
	

	// Todo(Leo): These still do not belong here
	if (is_clicked(input->select))
	{
		if (platform->windowIsFullscreen)
			result.setWindow = game::UpdateResult::SET_WINDOW_WINDOWED;
		else
			result.setWindow = game::UpdateResult::SET_WINDOW_FULLSCREEN;
	}

	/// Output sound
	{
		output_sound(soundOutput->sampleCount, soundOutput->samples);
	}

	return result;
}

