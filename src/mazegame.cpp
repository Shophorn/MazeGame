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
output_sound(int sampleCount, game::StereoSoundSample * samples)
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

	for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex)
	{
		samples[sampleIndex].left = file.samples[0][fileSampleIndex] * volume;
		samples[sampleIndex].right = file.samples[1][fileSampleIndex] * volume;

		fileSampleIndex += 1;
		fileSampleIndex %= fileSampleCount;
	}
}

internal void
load_scene(GameState * state, game::PlatformInfo * platform, platform::GraphicsContext * graphics, SceneInfo scene)
{
	state->loadedSceneInfo = scene;

	state->loadedScene = reserve_from_memory_arena(&state->persistentMemoryArena, scene.get_alloc_size());
	scene.load(state->loadedScene, &state->persistentMemoryArena, &state->transientMemoryArena, platform, graphics);

	state->sceneLoaded = true;	
}

internal void
load_gui(GameState * state, game::PlatformInfo * platform, platform::GraphicsContext * graphics, SceneGuiInfo gui)
{
	state->loadedGuiInfo = gui;

	state->loadedGui = reserve_from_memory_arena(&state->persistentMemoryArena, gui.get_alloc_size());
	gui.load(state->loadedGui, &state->persistentMemoryArena, &state->transientMemoryArena, platform, graphics);	
}

internal void
unload_scene_and_gui(GameState * state, game::PlatformInfo * platform, platform::GraphicsContext * graphics)
{
	state->loadedSceneInfo = {};

	platform->unload_scene(graphics);
	clear_memory_arena(&state->persistentMemoryArena);

	state->loadedScene = nullptr;
	state->loadedGui = nullptr;

	state->sceneLoaded = false;
}

extern "C" bool32
GameUpdate(
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformInfo * 	platform,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,

	// Todo(Leo): extra stupid name, but 'renderer' is also problematic, maybe combine with graphics context
	game::RenderInfo * 		outRenderInfo,
	platform::GraphicsContext * graphics)
{
	/* Note(Leo): This is reinterpreted each frame, we don't know and don't care
	if it has been moved or whatever in platform layer*/
	GameState * state = reinterpret_cast<GameState*>(memory->persistentMemory);

	if (memory->isInitialized == false)
	{

		initialize_game_state (state, memory, platform);
		memory->isInitialized = true;

		load_gui(state, platform, graphics, menuSceneGuiGui);
		std::cout << "Game initialized\n";
	}
	flush_memory_arena(&state->transientMemoryArena);
	
	outRenderInfo->prepare_drawing(graphics);
	if (state->loadedScene != nullptr)
	{
		state->loadedSceneInfo.update(state->loadedScene, input, outRenderInfo, platform, graphics);
	}
	auto guiResult = state->loadedGuiInfo.update(state->loadedGui, input, outRenderInfo, platform, graphics);
	outRenderInfo->finish_drawing(graphics);


	bool32 gameIsAlive = true;
	switch(guiResult)
	{
		case MENU_EXIT:
			gameIsAlive = false;
			break;

		case MENU_LOADLEVEL_2D:
			unload_scene_and_gui(state, platform, graphics);
			load_scene(state, platform, graphics, scene2dInfo);
			load_gui(state, platform, graphics, defaultSceneGui);
			break;

		case MENU_LOADLEVEL_3D:
			unload_scene_and_gui(state, platform, graphics);
			load_scene(state, platform, graphics, scene3dInfo);
			load_gui(state, platform, graphics, defaultSceneGui);
			break;

		case SCENE_EXIT:
			unload_scene_and_gui(state, platform, graphics);
			load_gui(state, platform, graphics, menuSceneGuiGui);
			break;

		default:
			break;
	}
	

	// Todo(Leo): These still do not belong here
	if (is_clicked(input->select))
	{
		if (platform->windowIsFullscreen)
		{
			platform->set_window_fullscreen(false);
		}
		else
		{
			platform->set_window_fullscreen(true);
		}
	}

	/// Output sound
	{
		output_sound(soundOutput->sampleCount, soundOutput->samples);
	}

	// Note(Leo): This is just a reminder
	// Todo(Leo): Remove if unnecessary
	/// Update network
	{
		// network->outPackage = {};
		// network->outPackage.characterPosition = state->character.position;
		// network->outPackage.characterRotation = state->character.rotation;
	}
	return gameIsAlive;
}

