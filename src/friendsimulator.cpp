/*
Leo Tamminen

Friendsimulator game code main file.
*/

#if !defined FRIENDSIMLATOR_PLATFORM
	#define FRIENDSIMULATOR_GAME_DLL
#endif

// Todo(Leo): these should be other way around, platform interface cannot be dependant of c/c++ files
#include "fs_essentials.hpp"

#include "fs_platform_interface.hpp"
#include "logging.cpp"

static PlatformGraphics * 	platformGraphics;
static PlatformWindow * 	platformWindow;

static MemoryArena * 		global_transientMemory;

static String push_temp_string (s32 capacity)
{
	String result = {0, push_memory<char>(*global_transientMemory, capacity, ALLOC_ZERO_MEMORY)};
	return result;
};

template<typename ... TArgs>
static String push_temp_string_format(s32 capacity, TArgs ... args)
{
	String result = push_temp_string(capacity);
	string_append_format(result, capacity, args...);
	return result;
}

constexpr f32 physics_gravity_acceleration = -9.81;

#include "colour.cpp"
#include "Debug.cpp"

// Note(Leo): Make unity build here.
#include "Random.cpp"
#include "Transform3D.cpp"
#include "Animator.cpp"
#include "Skybox.cpp"
#include "TerrainGenerator.cpp"
#include "Collisions3D.cpp"

#include "CameraController.cpp"
#include "RenderingSystem.cpp"

struct Font
{
	static constexpr u8 firstCharacter = 32; // space
	static constexpr u8 lastCharacter = 127; 
	static constexpr s32 count = lastCharacter - firstCharacter;

	// TextureHandle atlasTexture;
	GuiTextureHandle atlasTexture;

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
#include "Game.cpp"

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

static PlatformStereoSoundSample get_next_sample(AudioClip * clip)
{
	PlatformStereoSoundSample sample = {
		.left = clip->file.samples[0][clip->sampleIndex],
		.right = clip->file.samples[1][clip->sampleIndex]
	};

	clip->sampleIndex = (clip->sampleIndex + 1) % clip->sampleCount;

	return sample;
}

// Todo(Leo): remove 2d scene
enum LoadedSceneType { LOADED_SCENE_NONE, LOADED_SCENE_3D };

// Note(Leo): This makes less sense as a 'state' now that we have 'Scene' struct
// Todo(Leo): This makes less sense as a 'state' now that we have 'Scene' struct
struct GameState
{
	/* Note(Leo): MEMORY
	'persistentMemoryArena' is used to store things from frame to frame.
	'transientMemoryArena' is intended to be used in asset loading etc. and is flushed each frame*/
	MemoryArena persistentMemoryArena;
	MemoryArena transientMemoryArena;

	bool isInitialized;

	LoadedSceneType loadedSceneType;
	void * loadedScene;

	Gui 				gui;
	bool32 				guiVisible;
	GuiTextureHandle 	backgroundImage;

	// Todo(Leo): move to scene
	AudioClip backgroundAudio;
};

static Gui make_main_menu_gui(MemoryArena & allocator)
{
	Gui gui 				= {};
	gui.textSize 			= 40;
	gui.textColor 			= colour_white;
	gui.selectedTextColor 	= colour_muted_red;
	gui.padding 			= 10;
	gui.font 				= load_font("c:/windows/fonts/arial.ttf");

	u32 pixelColor 						= 0xffffffff;
	TextureAssetData guiTextureAsset 	= make_texture_asset(&pixelColor, 1, 1, 4);
	gui.panelTexture					= graphics_memory_push_gui_texture(platformGraphics, &guiTextureAsset);

	// Todo(Leo): this has nothing to do with gui, but this function is called appropriately :)
	// move away or change function name
	HdrSettings hdr = {1, 0};
	graphics_drawing_update_hdr_settings(platformGraphics, &hdr);

	return gui;
}

static void initialize_game_state(GameState * state, PlatformMemory * memory)
{
	*state = {};

	// // Note(Leo): Create persistent arena in the same memoryblock as game state, right after it.
	u64 gameStateSize 				= memory_align_up(sizeof(GameState), MemoryArena::defaultAlignment);
	byte * persistentMemory 		= reinterpret_cast<byte *>(memory->memory) + gameStateSize;
	u64 persistentMemorySize 		= (memory->size / 2) - gameStateSize;
	state->persistentMemoryArena 	= memory_arena(persistentMemory, persistentMemorySize); 

	byte * transientMemory 			= reinterpret_cast<byte*>(memory->memory) + gameStateSize + persistentMemorySize;
	u64 transientMemorySize 		= memory->size / 2;
	state->transientMemoryArena 	= memory_arena(transientMemory, transientMemorySize);

	state->backgroundAudio = load_audio_clip("assets/sounds/Wind-Mark_DiAngelo-1940285615.wav");

	state->gui 					= make_main_menu_gui(state->persistentMemoryArena);
	auto backGroundImageAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/NighestNouKeyArt.png");
	state->backgroundImage		= graphics_memory_push_gui_texture(platformGraphics, &backGroundImageAsset);

	state->isInitialized = true;
}



/* Note(Leo): return whether or not game still continues
Todo(Leo): indicate meaning of return value betterly somewhere, I almost forgot it.
*/
extern "C" __declspec(dllexport)  
bool32 update_game(
	PlatformInput const *	input,
	PlatformTime const *	time,

	PlatformMemory * 		memory,
	PlatformNetwork *		network,
	PlatformSoundOutput *	soundOutput,

	PlatformGraphics * 		graphics,
	PlatformWindow * 		window,
	PlatformApiDescription * apiDescripition,

	bool32 gameShouldReInitializeGlobalVariables)
{
	if (gameShouldReInitializeGlobalVariables)
	{
		platformGraphics 	= graphics;
		platformWindow 		= window;

		platform_set_api(apiDescripition);

		log_application(1, "Reinitialized global variables");
	}	

	/* Note(Leo): This is reinterpreted each frame, we don't know and don't care
	if it has been moved or whatever in platform layer*/
	GameState * state = reinterpret_cast<GameState*>(memory->memory);

	// Note(Leo): Testing out this idea
	// Note(Leo): So far, so good 12.06.2020
	global_transientMemory = &state->transientMemoryArena;

	// Note(Leo): Free space for current frame.
	flush_memory_arena(&state->transientMemoryArena);

	if (state->isInitialized == false)
	{
		initialize_game_state (state, memory);
	}
	
	bool32 gameIsAlive = true;
	bool32 sceneIsAlive = true;

	enum { ACTION_NONE, ACTION_NEW_GAME, ACTION_LOAD_GAME, ACTION_QUIT } action = ACTION_NONE;

	graphics_drawing_prepare_frame(graphics);

	if (state->loadedScene != nullptr)
	{
		if (state->loadedSceneType == LOADED_SCENE_3D)
		{
			sceneIsAlive = update_scene_3d(state->loadedScene, *input, *time);
		}
	}
	else
	{
		gui_start_frame(state->gui, *input, time->elapsedTime);

		gui_position({0,0});
		gui_image(state->backgroundImage, {1920, 1080}, {1,1,1,1});


		gui_position({870, 500});

		v4 guiPanelColor = colour_rgb_alpha(colour_bright_blue.rgb, 0.5);
		gui_start_panel(GUI_NO_TITLE, guiPanelColor);

		if(gui_button("New Game"))
		{
			action = ACTION_NEW_GAME;
		}

		if (gui_button("Load Game"))
		{
			action = ACTION_LOAD_GAME;
		}

		if (gui_button("Quit"))
		{
			gameIsAlive = false;
		}	

		
		gui_end_panel();

		// gui_position({100, 100});

		gui_end_frame();
	}

	/* Note(Leo): We MUST currently finish the frame before unloading scene, 
	because we have at this point issued commands to vulkan command buffers,
	and we currently have no mechanism to abort those. */
	// Todo(Leo): We maybe could use onpostrender on this
	graphics_drawing_finish_frame(graphics);

	if(action == ACTION_NEW_GAME)
	{
		state->loadedScene 		= load_scene_3d(state->persistentMemoryArena, nullptr);
		state->loadedSceneType 	= LOADED_SCENE_3D;
	}	
	else if (action == ACTION_LOAD_GAME)
	{
		PlatformFileHandle saveFile = platform_file_open("save_game.fssave", FILE_MODE_READ);
		state->loadedScene 			= load_scene_3d(state->persistentMemoryArena, saveFile);
		state->loadedSceneType 		= LOADED_SCENE_3D;
		platform_file_close(saveFile);
	}

	if (sceneIsAlive == false)
	{
		graphics_memory_unload(platformGraphics);
		flush_memory_arena(&state->persistentMemoryArena);

		state->loadedScene 		= nullptr;
		state->loadedSceneType 	= LOADED_SCENE_NONE;

		// Todo(Leo): this is a hack, unload_scene also unloads main gui, which is rather unwanted...
		state->gui 					= make_main_menu_gui(state->persistentMemoryArena);	
		auto backGroundImageAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/NighestNouKeyArt.png");
		state->backgroundImage		= graphics_memory_push_gui_texture(platformGraphics, &backGroundImageAsset);

	}

	// Todo(Leo): These still MAYBE do not belong here
	if (is_clicked(input->select))
	{
		bool isFullScreen = platform_window_is_fullscreen(platformWindow);
		platform_window_set_fullscreen(platformWindow, !isFullScreen);
		platform_window_set_cursor_visible(platformWindow, isFullScreen);
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
static_assert(is_same_type<UpdateGameFunc, decltype(update_game)>);