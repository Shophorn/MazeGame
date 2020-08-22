/*=============================================================================
Leo Tamminen

:MAZEGAME: game code main file.
=============================================================================*/
#include "MazegamePlatform.hpp"
#include "Mazegame.hpp"

static PlatformApi * 		platformApi;
static PlatformGraphics * 	platformGraphics;
static PlatformWindow * 	platformWindow;

static MemoryArena * global_transientMemory;

constexpr f32 physics_gravity_acceleration = -9.81;

constexpr v4 colour_white 			= {1,1,1,1};

constexpr v4 colour_aqua_blue 		= colour_v4_from_rgb_255(51, 255, 255);
constexpr v4 colour_raw_umber 		= colour_v4_from_rgb_255(130, 102, 68);

constexpr v4 colour_bright_red 		= {1.0, 0.0, 0.0, 1.0};
constexpr v4 colour_bright_blue 	= {0.0, 0.0, 1.0, 1.0};
constexpr v4 colour_bright_green 	= {0.0, 1.0, 0.0, 1.0};
constexpr v4 colour_bright_yellow 	= {1.0, 1.0, 0.0, 1.0};
constexpr v4 colour_bright_purple 	= {1, 0, 1, 1};

constexpr v4 colour_dark_green 		= {0, 0.6, 0, 1};
constexpr v4 colour_dark_red 		= {0.6, 0, 0, 1};

constexpr v4 colour_bump 			= {0.5, 0.5, 1.0, 0.0};

constexpr v4 colour_muted_red 		= {0.8, 0.2, 0.3, 1.0};
constexpr v4 colour_muted_green 	= {0.2, 0.8, 0.3, 1.0};
constexpr v4 colour_muted_blue 		= {0.2, 0.3, 0.8, 1.0};
constexpr v4 colour_muted_yellow 	= {0.8, 0.8, 0.2, 1.0};
constexpr v4 color_muted_purple 	= {0.8, 0.2, 0.8, 1};

#include "Debug.cpp"

// Note(Leo): Make unity build here.
#include "Random.cpp"
#include "MapGenerator.cpp"
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

/* Todo(Leo): Haxor: these should be in different translation units, and only include
headers here. For now, 2d scene does not include its stuff because 3d scene does before it
and both are still missing some because they are listed above.

Also, no 2d scene anymore*/
#include "Scene3D.cpp"

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

internal PlatformStereoSoundSample get_next_sample(AudioClip * clip)
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

internal Gui make_main_menu_gui(MemoryArena & allocator)
{
	Gui gui 				= {};
	gui.textSize 			= 40;
	gui.textColor 			= colour_white;
	gui.selectedTextColor 	= colour_muted_red;
	gui.padding 			= 10;
	gui.font 				= load_font("c:/windows/fonts/arial.ttf");

	u32 pixelColor 					= 0xffffffff;
	TextureAsset guiTextureAsset 	= make_texture_asset(&pixelColor, 1, 1, 4);
	gui.panelTexture				= platformApi->push_gui_texture(platformGraphics, &guiTextureAsset);

	// Todo(Leo): this has nothing to do with gui, but this function is called appropriately :)
	// move away or change function name
	HdrSettings hdr = {1, 0};
	platformApi->update_hdr_settings(platformGraphics, &hdr);

	return gui;
}

internal void initialize_game_state(GameState * state, PlatformMemory * memory)
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

	state->gui 					= make_main_menu_gui(state->persistentMemoryArena);
	auto backGroundImageAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/NighestNouKeyArt.png");
	state->backgroundImage		= platformApi->push_gui_texture(platformGraphics, &backGroundImageAsset);

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
	PlatformApi * 			api,

	std::ofstream * logFile)
{
	// Note(Leo): Set these each frame, they are reset if game is reloaded, and we do not know about that
	platformApi 		= api;
	platformGraphics 	= graphics;
	platformWindow 		= window;

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

		logDebug.output     = logFile;
		logWarning.output 	= logFile;
		logAnim.output      = logFile;
		logVulkan.output    = logFile;
		logWindow.output    = logFile;
		logSystem.output    = logFile;
		logNetwork.output   = logFile;
	}
	
	bool32 gameIsAlive = true;
	bool32 sceneIsAlive = true;

	enum { ACTION_NONE, ACTION_NEW_GAME, ACTION_LOAD_GAME, ACTION_QUIT } action = ACTION_NONE;

	platformApi->prepare_frame(graphics);

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
	platformApi->finish_frame(graphics);

	if(action == ACTION_NEW_GAME)
	{
		state->loadedScene 		= load_scene_3d(state->persistentMemoryArena, nullptr);
		state->loadedSceneType 	= LOADED_SCENE_3D;
	}	
	else if (action == ACTION_LOAD_GAME)
	{
		PlatformFileHandle saveFile = platformApi->open_file("save_game.fssave", FILE_MODE_READ);
		state->loadedScene 			= load_scene_3d(state->persistentMemoryArena, saveFile);
		state->loadedSceneType 		= LOADED_SCENE_3D;
		platformApi->close_file(saveFile);
	}

	if (sceneIsAlive == false)
	{
		platformApi->unload_scene(platformGraphics);
		flush_memory_arena(&state->persistentMemoryArena);

		state->loadedScene 		= nullptr;
		state->loadedSceneType 	= LOADED_SCENE_NONE;

		// Todo(Leo): this is a hack, unload_scene also unloads main gui, which is rather unwanted...
		state->gui 					= make_main_menu_gui(state->persistentMemoryArena);	
		auto backGroundImageAsset 	= load_texture_asset(*global_transientMemory, "assets/textures/NighestNouKeyArt.png");
		state->backgroundImage		= platformApi->push_gui_texture(platformGraphics, &backGroundImageAsset);

	}

	// Todo(Leo): These still MAYBE do not belong here
	if (is_clicked(input->select))
	{
		bool isFullScreen = !platformApi->is_window_fullscreen(platformWindow);
		platformApi->set_window_fullscreen(platformWindow, isFullScreen);
		platformApi->set_cursor_visible(platformWindow, !isFullScreen);
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