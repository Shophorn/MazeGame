/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"


// Todo(Leo): Handle has become stupid as it is now, pls remove
#include "Handle.cpp"


// Note(Leo): Make unity build here.
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

#pragma message("Do this soon pls")
/* Todo(Leo): Haxor: these should be in different translation units, and only include
headers here. For now, 2d scene does not include its stuff because 3d scene does before it
and both are still missing some because they are listed above. */
#include "Scene3D.cpp"
#include "Scene2D.cpp"
#include "MenuScene.cpp"

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

	bool32 sceneLoaded = false;
};

internal void
initialize_game_state(GameState * state, game::Memory * memory, game::PlatformFunctions * PlatformFunctions)
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
load_scene(GameState * state, game::PlatformFunctions * functions, platform::Graphics * graphics, platform::Platform * platform, SceneInfo scene)
{
	state->loadedSceneInfo = scene;

	state->loadedScene = reserve_from_memory_arena(&state->persistentMemoryArena, scene.get_alloc_size());
	scene.load(	state->loadedScene,
				&state->persistentMemoryArena,
				&state->transientMemoryArena,
				functions,
				graphics,
				platform);

	state->sceneLoaded = true;	
}

internal void
unload_scene_and_gui(GameState * state, game::PlatformFunctions * functions, platform::Graphics * graphics)
{
	state->loadedSceneInfo = {};

	functions->unload_scene(graphics);
	clear_memory_arena(&state->persistentMemoryArena);

	state->loadedScene = nullptr;
	state->sceneLoaded = false;
}


/*Note(Leo): return whether or not game still continues

Todo(Leo): indicate meaning of return value betterly somewhere,
I almost forgot it.
*/
extern "C" bool32
GameUpdate(
	game::Input * 			input,
	game::Memory * 			memory,
	game::PlatformFunctions * 	functions,
	game::Network *			network,
	game::SoundOutput * 	soundOutput,

	// Todo(Leo): extra stupid name, but 'renderer' is also problematic, maybe combine with graphics context
	game::RenderInfo * 		outRenderInfo,
	platform::Graphics * graphics,
	platform::Platform * platform)
{
	/* Note(Leo): This is reinterpreted each frame, we don't know and don't care
	if it has been moved or whatever in functions layer*/
	GameState * state = reinterpret_cast<GameState*>(memory->persistentMemory);

	if (memory->isInitialized == false)
	{

		initialize_game_state (state, memory, functions);
		memory->isInitialized = true;

		load_scene(state, functions, graphics, platform, menuScene);
		std::cout << "Game initialized\n";
	}
	flush_memory_arena(&state->transientMemoryArena);
	
	outRenderInfo->prepare_drawing(graphics);
	auto guiResult = state->loadedSceneInfo.update(	state->loadedScene,
													input,
													outRenderInfo,
													functions,
													graphics,
													platform);
	outRenderInfo->finish_drawing(graphics);


	bool32 gameIsAlive = true;
	switch(guiResult)
	{
		case MENU_EXIT:
			gameIsAlive = false;
			break;

		case MENU_LOADLEVEL_2D:
			unload_scene_and_gui(state, functions, graphics);
			load_scene(state, functions, graphics, platform, scene2dInfo);
			break;

		case MENU_LOADLEVEL_3D:
			unload_scene_and_gui(state, functions, graphics);
			load_scene(state, functions, graphics, platform, scene3dInfo);
			break;

		case SCENE_EXIT:
			unload_scene_and_gui(state, functions, graphics);
			load_scene(state, functions, graphics, platform, menuScene);
			break;

		default:
			break;
	}
	

	// Todo(Leo): These still do not belong here
	if (is_clicked(input->select))
	{
		if (functions->is_window_fullscreen(platform))
		{
			functions->set_window_fullscreen(false);
		}
		else
		{
			functions->set_window_fullscreen(true);
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

