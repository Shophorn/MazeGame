/*
Leo Tamminen

Windows platform implementation layer for Friendsimulator.
This is the first file to compile, and everything is included from here.
*/

/*
Note(Leo): There is fuckery going inside win32 things
WinSock2 must be included before lean and mean and windows.h
*/
#define NOMINMAX
#include <WinSock2.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>
#include <xinput.h>

#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define FS_PLATFORM

#if defined FS_FULL_GAME

	#define FS_DEVELOPMENT_ONLY(expr) (void(0))
	#define FS_FULL_GAME_ONLY(expr) expr

	#define FS_ENTRY_POINT int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

#elif defined FS_DEVELOPMENT

	#define FS_DEVELOPMENT_ONLY(expr) expr
	#define FS_FULL_GAME_ONLY(expr) (void(0))

	#define FS_ENTRY_POINT int main()

#else
	#error "FS_DEVELOPMENT or FS_FULL_GAME must be defined."
#endif

// Todo(Leo): these should be in different order, maybe. Essentials describe some c++ stuff that shouldn't concern platform api
#include "fs_standard_types.h"
#include "fs_standard_functions.h"
#include "fs_platform_interface.hpp"

#include "logging.cpp"

#include "fswin32_platform_log.cpp"
#include "fswin32_platform_time.cpp"
#include "fswin32_platform_file.cpp"
#include "fswin32_platform_input.cpp"

#include "fswin32_platform_window.cpp"
#include "fswin32_game_dll.cpp"

#include "winapi_ErrorStrings.hpp"

#define WIN32_CHECK(result) {if (result != S_OK) { log_debug(FILE_ADDRESS, fswin32_error_string(result), " (", (s32)result, ")"); abort(); }}

#include "winapi_WinSocketDebugStrings.cpp"
#include "winapi_Audio.cpp"
#include "winapi_Network.cpp"

// Todo(Leo): Get rid of this :) It is used in some stupid places anyway
#include <vector>
#include "fsvulkan.cpp"

#if defined FS_FULL_GAME
	#include "friendsimulator.cpp"
#endif

FS_ENTRY_POINT
{
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	
	SYSTEMTIME time_;
	GetLocalTime(&time_);

	log_application(0,"\n",
				"\t----- FriendSimulator -----\n",
				"\tBuild time: ", BUILD_DATE_TIME, "\n");

	// ----------------------------------------------------------------------------------

	Win32ApplicationState state = {};
	load_xinput();

	// ---------- INITIALIZE PLATFORM ------------
	PlatformWindow window 	= fswin32_make_window(hInstance, 960, 540);
	state.window 			= &window;
	SetWindowLongPtrW(window.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&state));

	VulkanContext vulkanContext = winapi::create_vulkan_context(&window);
   
	// ------- MEMORY ---------------------------
	PlatformMemory platformMemory = {};
	{
		// TODO [MEMORY] (Leo): Properly measure required amount
		// TODO [MEMORY] (Leo): Think of alignment
		platformMemory.size = gigabytes(2);

		// TODO [MEMORY] (Leo): Check support for large pages
		FS_DEVELOPMENT_ONLY(void * baseAddress = (void*)terabytes(2));
		FS_FULL_GAME_ONLY(void * baseAddress = nullptr);
		platformMemory.memory = VirtualAlloc(baseAddress, platformMemory.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		Assert(platformMemory.memory != nullptr);
	}

	// -------- INITIALIZE NETWORK ---------
	bool32 networkIsRuined = false;
	// WinApiNetwork network = winapi::CreateNetwork();
	WinApiNetwork network       = {};
	PlatformNetwork platformNetwork   = {};
	f64 networkSendDelay        = 1.0 / 20;
	f64 networkNextSendTime     = 0;
	
	/// --------- INITIALIZE AUDIO ----------------
	f32 audioBufferLengthSeconds = 0.1;
	WinApiAudio audio = fswin32_create_audio(audioBufferLengthSeconds);
	fswin32_start_playing(&audio);                

	/// ---------- LOAD GAME CODE ----------------------
	// Note(Leo): Only load game dynamically in development.
	Win32Game game = {};

	FS_DEVELOPMENT_ONLY(fswin32_game_load_dll(&game));
	FS_FULL_GAME_ONLY(game.update = update_game);

	game.shouldReInitializeGlobalVariables = true;

	  ////////////////////////////////////////////////////
	 ///             MAIN LOOP                        ///
	////////////////////////////////////////////////////

	bool gameIsRunning = true;
	PlatformInput platformInput = {};

	/// --------- TIMING ---------------------------
	PlatformTimePoint frameFlipTime;
	f64 lastFrameElapsedSeconds;

	while(gameIsRunning)
	{
		/// ----- RELOAD GAME CODE -----

		FS_DEVELOPMENT_ONLY(fswin32_game_reload(game));

		/// ----- HANDLE INPUT -----
		{
			// Note(Leo): this is not input only...
		
			fswin32_process_pending_messages(&state, window.hwnd);

			if (window.isCursorVisible == false)
			{
				f32 cursorX = window.width / 2;
				f32 cursorY = window.height / 2;

				POINT currentCursorPosition;
				GetCursorPos(&currentCursorPosition);

				v2 mouseMovement = {currentCursorPosition.x - cursorX, currentCursorPosition.y - cursorY};

				platformInput.axes[InputAxis_mouse_move_x] = mouseMovement.x;
				platformInput.axes[InputAxis_mouse_move_y] = mouseMovement.y;

				SetCursorPos((s32)cursorX, (s32)cursorY);
			}




			HWND foregroundWindow = GetForegroundWindow();
			bool32 windowIsActive = window.hwnd == foregroundWindow;

			/* Note(Leo): Only get input from a single controller, locally this is a single
			player game. Use global controller index depending on network status to test
			multiplayering */
		   /* TODO [Input](Leo): Test controller and index behaviour when being connected and
			disconnected */

			XINPUT_STATE xinputState;
			bool32 xinputReceived = xinput_get_state(globalXinputControllerIndex, &xinputState) == ERROR_SUCCESS;
			bool32 xinputUsed = xinputReceived && xinput_is_used(state.gamepadInput, xinputState);

			for(auto & button : platformInput.buttons)
			{
				if (button == InputButtonState_went_down)
				{
					button = InputButtonState_is_down;
				}

				else if (button == InputButtonState_went_up)
				{
					button = InputButtonState_is_up;
				}
			}

			if (xinputUsed)
			{
				update_controller_input(&platformInput, &xinputState);
				platformInput.gamepadInputUsed = true;
			}
			else
			{
				platformInput.gamepadInputUsed = false;
			}

			if (state.keyboardInputIsUsed)
			{
				update_keyboard_input(&platformInput, &state.keyboardInput);
				platformInput.mouseAndKeyboardInputUsed = true;
			}
			else
			{
				platformInput.mouseAndKeyboardInputUsed = false;
			}

			// Todo(Leo): Totally in wrong place :))
			platformInput.mousePosition = state.keyboardInput.mousePosition;
    		platformInput.axes[InputAxis_mouse_scroll]	= state.keyboardInput.mouseScroll;

    		state.keyboardInput.mouseScroll = 0;

		}


		/// ----- PRE-UPDATE NETWORK PASS -----
		{
			// Todo(Leo): just quit now if some unhandled network error occured
			if (networkIsRuined)
			{
				auto error = WSAGetLastError();
				log_network(1, "failed: ", WinSocketErrorString(error), " (", error, ")"); 
				break;
			}

			if (network.isListening)
			{
				winapi::NetworkListen(&network);
			}
			else if (network.isConnected)
			{
				winapi::NetworkReceive(&network, &platformNetwork.inPackage);
			}
			platformNetwork.isConnected = network.isConnected;
		}

		PlatformTime platformTime = {};
		platformTime.elapsedTime = (f32)lastFrameElapsedSeconds;

		/// --------- UPDATE GAME -------------
		// Note(Leo): Game is not updated when window is not drawable.
		if (fswin32_is_window_drawable(&window))
		{
			PlatformSoundOutput gameSoundOutput = {};
			fswin32_get_audio_buffer(&audio, &gameSoundOutput.sampleCount, &gameSoundOutput.samples);
	
			switch(fsvulkan_prepare_frame(&vulkanContext))
			{
				case PGFR_FRAME_OK:
					PlatformApiDescription apiDescription;
					platform_set_api(&apiDescription);
					gameIsRunning = game.update(&platformInput, 
												&platformTime,
												&platformMemory,
												&platformNetwork,
												&gameSoundOutput,
												
												&vulkanContext,
												&window,
												&apiDescription,
												game.shouldReInitializeGlobalVariables);

					// Note(Leo): We must set this to false if we actually have passed it to dll function
					game.shouldReInitializeGlobalVariables = false;

					break;


				case PGFR_FRAME_RECREATE:
					fsvulkan_recreate_drawing_resources(&vulkanContext, window.width, window.height);
					break;

				case PGFR_FRAME_BAD_PROBLEM:
					AssertMsg(false, "We should not be here, please investigate");


			}

			fswin32_release_audio_buffer(&audio, gameSoundOutput.sampleCount);

			// Note(Leo): It doesn't so much matter where this is checked.
			if (state.shouldClose)
			{
				gameIsRunning = false;
			}

		}


		// ----- POST-UPDATE NETWORK PASS ------
		// TODO[network](Leo): Now that we have fixed frame rate, we should count frames passed instead of time
		// if (network.isConnected && frameStartTime > networkNextSendTime)
		// {
		// 	networkNextSendTime = frameStartTime + networkSendDelay;
		// 	winapi::NetworkSend(&network, &platformNetwork.outPackage);
		// }


		// ----- MEASURE ELAPSED TIME ----- 
		{
			PlatformTimePoint now   = platform_time_now();
			lastFrameElapsedSeconds = platform_time_elapsed_seconds(frameFlipTime, now);


		// Todo(Leo): FIX THISSSSSS
		// Todo(Leo): FIX THISSSSSS
		// Todo(Leo): FIX THISSSSSS
		// Todo(Leo): FIX THISSSSSS
		// Todo(Leo): FIX THISSSSSS
		// #if defined FS_DEVELOPMENT
		#if 1
			// Restrict framerate so we do not burn our computer
			f32 targetFrameTime2 = 1.0f / 30;

			s32 targetMilliseconds = (s32)(targetFrameTime2 * 1000);  
			s32 elapsedMilliseconds = (s32)(lastFrameElapsedSeconds * 1000);

			s32 millisecondsToSleep = targetMilliseconds - elapsedMilliseconds;

		#if 1
			// Todo(leo): This for debugger also, it seems to get stuck on sleep on first round
			// Also application itself...
			millisecondsToSleep = min_s32(millisecondsToSleep, 100);
		#endif

			if (millisecondsToSleep > 2)
			{
				timeBeginPeriod(1);
				Sleep(millisecondsToSleep);
				timeEndPeriod(1);
			}

			now   					= platform_time_now();
			lastFrameElapsedSeconds = platform_time_elapsed_seconds(frameFlipTime, now);
		#endif
			frameFlipTime           = now;

			// approxAvgFrameTime = interpolate(approxAvgFrameTime, lastFrameElapsedSeconds, approxAvgFrameTimeAlpha);
			// log_console(0) << approxAvgFrameTime;

		// #endif
		}
	}
	///////////////////////////////////////
	///         END OF MAIN LOOP        ///
	///////////////////////////////////////


	/// -------- CLEANUP ---------
	fswin32_stop_playing(&audio);
	fswin32_release_audio(&audio);
	winapi::CloseNetwork(&network);

	winapi::destroy_vulkan_context(&vulkanContext);

	/// ----- Cleanup Windows
	{
		FS_DEVELOPMENT_ONLY(fswin32_game_unload_dll(&game));
		log_application(0, "shut down");
	}

	return EXIT_SUCCESS;

}