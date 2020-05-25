/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

static platform::Functions * 	platformApi;
static platform::Graphics * 	platformGraphics;
static platform::Window * 		platformWindow;

static MemoryArena * global_transientMemory;

namespace physics
{
	constexpr f32 gravityAcceleration = -9.81;
}

namespace colors
{
	constexpr v4 brightRed 		= {1.0, 0.0, 0.0, 1.0};
	constexpr v4 brightGreen 	= {0.0, 1.0, 0.0, 1.0};
	constexpr v4 brightBlue 	= {0.0, 0.0, 1.0, 1.0};
	constexpr v4 brightYellow 	= {1.0, 1.0, 0.0, 1.0};

	constexpr v4 mutedRed 		= {0.8, 0.2, 0.3, 1.0};
	constexpr v4 mutedGreen 	= {0.2, 0.8, 0.3, 1.0};
	constexpr v4 mutedBlue 		= {0.2, 0.3, 0.8, 1.0};
	constexpr v4 mutedYellow 	= {0.8, 0.8, 0.2, 1.0};

	constexpr v4 white 			= {1,1,1,1};
	constexpr v4 lightGray 		= {0.8, 0.8, 0.8, 1};
	constexpr v4 darkGray 		= {0.3, 0.3, 0.3, 1};
	constexpr v4 black 			= {0,0,0,1};
}

constexpr v4 colorDarkGreen = {0, 0.6, 0, 1};

#include "Debug.cpp"

// Note(Leo): Make unity build here.
#include "Random.cpp"
#include "MapGenerator.cpp"
#include "Transform3D.cpp"
#include "Animator.cpp"
#include "Collisions2D.cpp"
#include "CollisionManager2D.cpp"
#include "Skybox.cpp"
#include "TerrainGenerator.cpp"
#include "Collisions3D.cpp"


#include "CharacterSystems.cpp"
#include "CameraController.cpp"
#include "RenderingSystem.cpp"

struct Font
{
	static constexpr u8 firstCharacter = 32; // space
	static constexpr u8 lastCharacter = 127; 
	static constexpr s32 count = lastCharacter - firstCharacter;

	TextureHandle atlasTexture;

	f32 spaceWidth;

	f32 leftSideBearings [count];
	f32 advanceWidths [count];
	f32 characterWidths [count];
	v4 uvPositionsAndSizes [count];

	// Todo(Leo): Kerning
};

/// Note(Leo): These still use external libraries we may want to get rid of
#include "Files.cpp"
#include "AudioFile.cpp"
#include "MeshLoader.cpp"

// Note(Leo): stb libraries here seems cool
#include "TextureLoader.cpp"

#include "Gui.cpp"

/* Todo(Leo): Haxor: these should be in different translation units, and only include
headers here. For now, 2d scene does not include its stuff because 3d scene does before it
and both are still missing some because they are listed above. */
#include "Scene3D.cpp"
#include "Scene2D.cpp"

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

	void * loadedScene;

	// Note(Leo): This MUST return false if it intends to close the scene, and true otherwise
	using FN_updateScene = bool32(void * scene, game::Input * input);
	FN_updateScene * updateScene;

	Gui 	gui;
	bool32 	guiVisible;

	// Todo(Leo): move to scene
	AudioClip backgroundAudio;
};

internal Gui make_main_menu_gui(MemoryArena & allocator)
{
	Gui gui 		= {};
	gui.textSize 	= 40;
	gui.textColor 	= colors::white;
	gui.selectedTextColor = colors::mutedBlue;
	gui.padding 	= 10;
	gui.font 		= load_font("c:/windows/fonts/arial.ttf");
	gui_generate_font_material(gui);

	return gui;
}

internal void initialize_game_state(GameState * state, platform::Memory * memory)
{
	*state = {};

	// // Note(Leo): Create persistent arena in the same memoryblock as game state, right after it.
	u64 gameStateSize 				= align_up(sizeof(GameState), MemoryArena::defaultAlignment);
	byte * persistentMemory 		= reinterpret_cast<byte *>(memory->memory) + gameStateSize;
	u64 persistentMemorySize 		= (memory->size / 2) - gameStateSize;
	state->persistentMemoryArena 	= make_memory_arena(persistentMemory, persistentMemorySize); 

	byte * transientMemory 			= reinterpret_cast<byte*>(memory->memory) + gameStateSize + persistentMemorySize;
	u64 transientMemorySize 		= memory->size / 2;
	state->transientMemoryArena 	= make_memory_arena(transientMemory, transientMemorySize);

	state->backgroundAudio = load_audio_clip("assets/sounds/Wind-Mark_DiAngelo-1940285615.wav");

	state->gui = make_main_menu_gui(state->persistentMemoryArena);

	state->isInitialized = true;
}



/* Note(Leo): return whether or not game still continues
Todo(Leo): indicate meaning of return value betterly somewhere, I almost forgot it.
*/
extern "C" __declspec(dllexport)  
bool32 update_game(
	game::Input * 			input,
	platform::Memory * 		memory,
	game::Network *			network,
	platform::SoundOutput *	soundOutput,

	platform::Graphics * 	graphics,
	platform::Window * 		window,
	platform::Functions * 	functions,

	std::ofstream * logFile)
{
	// Note(Leo): Set these each frame, they are reset if game is reloaded, and we do not know about that
	platformApi 		= functions;
	platformGraphics 	= graphics;
	platformWindow 		= window;

	/* Note(Leo): This is reinterpreted each frame, we don't know and don't care
	if it has been moved or whatever in platform layer*/
	GameState * state = reinterpret_cast<GameState*>(memory->memory);

	// Note(Leo): Testing out this idea
	global_transientMemory = &state->transientMemoryArena;

	// Note(Leo): Free space for current frame.
	flush_memory_arena(&state->transientMemoryArena);


	if (state->isInitialized == false)
	{
		initialize_game_state (state, memory);

		logDebug.output     = logFile;
		logAnim.output      = logFile;
		logVulkan.output    = logFile;
		logWindow.output    = logFile;
		logSystem.output    = logFile;
		logNetwork.output   = logFile;

	}
	
	bool32 gameIsAlive = true;
	bool32 sceneIsAlive = true;

	functions->prepare_frame(graphics);

	if (state->loadedScene != nullptr)
	{
		sceneIsAlive = state->updateScene(state->loadedScene, input);
	}
	else
	{
		gui_start(state->gui, input);

		if(gui_button("3D Scene"))
		{
			state->loadedScene = load_scene_3d(state->persistentMemoryArena);
			state->updateScene = update_scene_3d;
		}

		if (gui_button("2D Scene"))
		{
			state->loadedScene = load_scene_2d(state->persistentMemoryArena);
			state->updateScene = update_scene_2d;
		}

		if (gui_button("Quit"))
		{
			gameIsAlive = false;
		}

		gui_end();
	}

	/* Note(Leo): We MUST currently finish the frame before unloading scene, 
	because we have at this point issued commands to vulkan command buffers,
	and we currently have no mechanism to abort those. */
	functions->finish_frame(graphics);

	if (sceneIsAlive == false)
	{
		platformApi->unload_scene(platformGraphics);
		flush_memory_arena(&state->persistentMemoryArena);

		state->loadedScene = nullptr;

		// Todo(Leo): this is a hack, unload_scene also unloads main gui, which is rather unwanted...
		state->gui = make_main_menu_gui(state->persistentMemoryArena);	
	}

	// Todo(Leo): These still MAYBE do not belong here
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


// Note(Leo): This makes sure we have actually defined this correctly.
static_assert(std::is_same_v<GameUpdateFunc, decltype(update_game)>);