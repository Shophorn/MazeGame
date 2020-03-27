/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

#include "Debug.cpp"

#include "CStringUtility.cpp"
#include "SmallString.cpp"

// Note(Leo): Make unity build here.
#include "Random.cpp"
#include "MapGenerator.cpp"
#include "Transform3D.cpp"
#include "Animator.cpp"
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

/* Todo(Leo): Haxor: these should be in different translation units, and only include
headers here. For now, 2d scene does not include its stuff because 3d scene does before it
and both are still missing some because they are listed above. */
#include "Scene3D.cpp"
#include "Scene2D.cpp"
#include "MenuScene.cpp"

struct AudioClip
{
	AudioFile<float> file;
	s32 sampleCount;
	s32 sampleIndex;
};

AudioClip load_audio_clip(char const * filepath)
{
	AudioClip clip = {};
	clip.file.load(filepath);
	clip.sampleCount = clip.file.getNumSamplesPerChannel();

	return clip;
}

platform::StereoSoundSample
get_next_sample(AudioClip * clip)
{
	platform::StereoSoundSample sample = {
		.left = clip->file.samples[0][clip->sampleIndex],
		.right = clip->file.samples[1][clip->sampleIndex]
	};

	clip->sampleIndex = (clip->sampleIndex + 1) % clip->sampleCount;

	return sample;
}

// Note(Leo): This makes less sense as a 'state' now that we have 'Scene' struct
struct GameState
{
	/* Note(Leo): MEMORY
	'persistentMemoryArena' is used to store things from frame to frame.
	'transientMemoryArena' is intended to be used in asset loading etc. and is flushed each frame*/
	MemoryArena persistentMemoryArena;
	MemoryArena transientMemoryArena;

	bool isInitialized;

	SceneInfo loadedSceneInfo;
	void * loadedScene;

	// Todo(Leo): move to scene
	AudioClip backgroundAudio;
};

internal void
initialize_game_state(GameState * state, platform::Memory * memory)
{
	*state = {};

	// // Note(Leo): Create persistent arena in the same memoryblock as game state, right after it.
	u64 gameStateSize 				= align_up_to(sizeof(GameState), MemoryArena::defaultAlignment);
	byte * persistentMemory 		= reinterpret_cast<byte *>(memory->memory) + gameStateSize;
	u64 persistentMemorySize 		= (memory->size / 2) - gameStateSize;
	state->persistentMemoryArena 	= make_memory_arena(persistentMemory, persistentMemorySize); 

	byte * transientMemory 			= reinterpret_cast<byte*>(memory->memory) + gameStateSize + persistentMemorySize;
	u64 transientMemorySize 		= memory->size / 2;
	state->transientMemoryArena 	= make_memory_arena(transientMemory, transientMemorySize);

	state->backgroundAudio = load_audio_clip("assets/sounds/Wind-Mark_DiAngelo-1940285615.wav");


	state->isInitialized = true;
}

internal void
load_scene(	GameState * state,
			SceneInfo scene,
			platform::Graphics * graphics,
			platform::Window * system,
			platform::Functions * functions)
{
	state->loadedSceneInfo = scene;

	state->loadedScene = allocate(state->persistentMemoryArena, scene.get_alloc_size(), true);
	scene.load(	state->loadedScene,
				&state->persistentMemoryArena,
				&state->transientMemoryArena,
				graphics,
				system,
				functions);
}

internal void
unload_scene(	GameState * state,
				platform::Graphics * graphics,
				platform::Functions * functions)
{
	state->loadedSceneInfo = {};

	functions->unload_scene(graphics);
	flush_memory_arena(&state->persistentMemoryArena);

	state->loadedScene = nullptr;
}

/* Note(Leo): return whether or not game still continues
Todo(Leo): indicate meaning of return value betterly somewhere, I almost forgot it.
*/
extern "C" bool32
update_game(
	game::Input * 			input,
	platform::Memory * 		memory,
	game::Network *			network,
	platform::SoundOutput *	soundOutput,

	platform::Graphics * 	graphics,
	platform::Window * 		window,
	platform::Functions * 	functions)
{
	/* Note(Leo): This is reinterpreted each frame, we don't know and don't care
	if it has been moved or whatever in platform layer*/
	GameState * state = reinterpret_cast<GameState*>(memory->memory);

	if (state->isInitialized == false)
	{
		initialize_game_state (state, memory);
		load_scene(state, get_menu_scene_info(), graphics, window, functions);
	}

	// Note(Leo): Free space for current frame.
	flush_memory_arena(&state->transientMemoryArena);
	
	functions->prepare_frame(graphics);
	auto guiResult = state->loadedSceneInfo.update(	state->loadedScene,
													input,
													graphics,
													window,
													functions);
	functions->finish_frame(graphics);

	bool32 gameIsAlive = true;
	switch(guiResult)
	{
		case MENU_EXIT:
			gameIsAlive = false;
			break;

		case MENU_LOADLEVEL_2D:
			unload_scene(state, graphics, functions);
			load_scene(state, get_2d_scene_info(), graphics, window, functions);
			break;

		case MENU_LOADLEVEL_3D:
			unload_scene(state, graphics, functions);
			load_scene(state, get_3d_scene_info(), graphics, window, functions);
			break;

		case SCENE_EXIT:
			unload_scene(state, graphics, functions);
			load_scene(state, get_menu_scene_info(), graphics, window, functions);
			break;

		default:
			break;
	}
	

	// Todo(Leo): These still do not belong here
	if (is_clicked(input->select))
	{
		if (functions->is_window_fullscreen(window))
		{
			functions->set_window_fullscreen(window, false);
		}
		else
		{
			functions->set_window_fullscreen(window, true);
		}
	}

	/* Todo(Leo): this should probably be the master mixer, and scenes
	just put their audio in it.	*/
	{
		// Todo(Leo): get volume from some input structure
		float volume = 0.5f;

		for (auto & sample : *soundOutput)
		{
			sample = get_next_sample(&state->backgroundAudio);
		}
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

