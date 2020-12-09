/*
Leo Tamminen

Friendsimulator game code main file.
*/
#if defined FS_DEVELOPMENT
	#define _CRT_SECURE_NO_WARNINGS

	#include <imgui/imgui.h>
	#include <imgui/imgui.cpp>
	#include <imgui/imgui_widgets.cpp>
	#include <imgui/imgui_draw.cpp>
	#include <imgui/imgui_demo.cpp>

	#define FS_GAME_DLL

	#include "fs_standard_types.h"

	// Todo(Leo): make this .cpp file
	#include "fs_standard_functions.h"
	#include "fs_platform_interface.hpp"
	#include "logging.cpp"

	FS_GAME_API void game_set_platform_functions(PlatformApiDescription * apiDescripition, ImGuiContext * imguiContext)
	{
		platform_set_api(apiDescripition);
		ImGui::SetCurrentContext(imguiContext);
	}


#endif



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

#include "experimental.cpp"

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
#include "Gui.cpp"

#include "audio_mixer.cpp"
#include "Game.cpp"


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

	Game * loadedGame;

	Gui gui;
	GameAssets assets;
};

static Gui make_main_menu_gui(MemoryArena & allocator, GameAssets & assets)
{
	Gui gui 				= {};
	gui.textSize 			= 40;
	gui.textColor 			= colour_white;
	gui.selectedTextColor 	= colour_muted_red;
	gui.padding 			= 10;
	gui.font 				= assets_get_font(assets, FontAssetId_game);

	u32 pixelColor 						= 0xffffffff;
	TextureAssetData guiTextureAsset 	= make_texture_asset(&pixelColor, 1, 1, 4);
	TextureHandle guiTextureHandle 		= graphics_memory_push_texture(platformGraphics, &guiTextureAsset);
	gui.panelTexture 					= graphics_memory_push_material(platformGraphics, GraphicsPipeline_screen_gui, 1, &guiTextureHandle);

	// Todo(Leo): this has nothing to do with gui, but this function is called conveniently :)
	// move away or change function name
	HdrSettings hdr = {1, 0};
	graphics_drawing_update_hdr_settings(platformGraphics, &hdr);

	return gui;
}

static void game_init_state(GameState * state, MemoryBlock memory)
{
	*state = {};

	// // Note(Leo): Create persistent arena in the same memoryblock as game state, right after it.
	u64 gameStateSize 				= memory_align_up(sizeof(GameState), MemoryArena::defaultAlignment);
	byte * persistentMemory 		= reinterpret_cast<byte *>(memory.memory) + gameStateSize;
	u64 persistentMemorySize 		= (memory.size / 2) - gameStateSize;
	state->persistentMemoryArena 	= memory_arena(persistentMemory, persistentMemorySize); 

	byte * transientMemory 			= reinterpret_cast<byte*>(memory.memory) + gameStateSize + persistentMemorySize;
	u64 transientMemorySize 		= memory.size / 2;
	state->transientMemoryArena 	= memory_arena(transientMemory, transientMemorySize);

	state->assets 	= init_game_assets(&state->persistentMemoryArena);
	state->gui 		= make_main_menu_gui(state->persistentMemoryArena, state->assets);

	state->isInitialized = true; 
}

// Note(Leo): return whether or not game still continues
// Todo(Leo): indicate meaning of return value betterly somewhere, I almost forgot it.
FS_GAME_API bool32 game_update(	MemoryBlock 				gameMemory,
								PlatformInput  *			input,
								PlatformGraphics * 			graphics,
								PlatformWindow * 			window,
								PlatformAudio *				audio,
								f32 						elapsedTimeSeconds)
{
	platformGraphics 	= graphics;
	platformWindow 		= window;


	/* Note(Leo): This is reinterpreted each frame, we don't know and don't care
	if it has been moved or whatever in platform layer*/
	GameState * state = reinterpret_cast<GameState*>(gameMemory.memory);

	// Note(Leo): Testing out this idea
	// Note(Leo): So far, so good 12.06.2020
	global_transientMemory = &state->transientMemoryArena;

	// Note(Leo): Free space for current frame.
	flush_memory_arena(&state->transientMemoryArena);

	if (state->isInitialized == false)
	{
		game_init_state (state, gameMemory);
	}
	
	bool32 gameIsAlive = true;
	bool32 sceneIsAlive = true;

	enum { ACTION_NONE, ACTION_NEW_GAME, ACTION_LOAD_GAME, ACTION_QUIT } action = ACTION_NONE;

	// graphics_drawing_prepare_frame(graphics);

	if (state->loadedGame != nullptr)
	{
		StereoSoundOutput soundOutput = audio_get_output_buffer(audio);
		sceneIsAlive = game_game_update(state->loadedGame, input, &soundOutput, elapsedTimeSeconds);
		audio_release_output_buffer(audio, soundOutput);
	}
	else
	{
		gui_start_frame(state->gui, input, elapsedTimeSeconds);

		// ImGui::Begin("Game Panel");
		// ImGui::End();


		gui_position({0,0});
		gui_image(assets_get_material(state->assets, MaterialAssetId_menu_background), {1920, 1080}, {1,1,1,1});


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
		gui_end_frame();
	}

	/* Note(Leo): We MUST currently finish the frame before unloading scene, 
	because we have at this point issued commands to vulkan command buffers,
	and we currently have no mechanism to abort those. */
	// Todo(Leo): We maybe could use onpostrender on this
	// graphics_drawing_finish_frame(graphics);

	if(action == ACTION_NEW_GAME)
	{
		state->loadedGame = game_load_game(state->persistentMemoryArena, nullptr);
	}	
	else if (action == ACTION_LOAD_GAME)
	{
		PlatformFileHandle saveFile = platform_file_open("save_game.fssave", FILE_MODE_READ);
		state->loadedGame 			= game_load_game(state->persistentMemoryArena, saveFile);
		platform_file_close(saveFile);
	}

	if (sceneIsAlive == false)
	{
		graphics_memory_unload(platformGraphics);
		flush_memory_arena(&state->persistentMemoryArena);

		// // Note(Leo): we flushed graphics and cpu memory, our assets are gone, we will reinit them next frame
		state->isInitialized 	= false;
		state->loadedGame 		= nullptr;
	}

	// Todo(Leo): These still MAYBE do not belong here
	if (input_button_went_down(input, InputButton_select) || input_button_went_down(input, InputButton_keyboard_f2))
	{
		bool isFullScreen = platform_window_get_fullscreen(platformWindow);
		platform_window_set_fullscreen(platformWindow, !isFullScreen);
		platform_window_set_cursor_visible(platformWindow, isFullScreen);
	}

	return gameIsAlive;
}
