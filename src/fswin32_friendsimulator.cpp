/*
Leo Tamminen

Windows platform implementation layer for Friendsimulator.
This is the first file to compile, and everything is included from here.
*/

#include <WinSock2.h>
#include <Windows.h>
#include <Windowsx.h>
#include <xinput.h>

#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>


#define FRIENDSIMULATOR_PLATFORM
// Todo(Leo): these should be in different order
#include "fs_essentials.hpp"
#include "fs_platform_interface.hpp"

#include "logging.cpp"

#include "fswin32_platform_log.cpp"
#include "fswin32_platform_time.cpp"
#include "fswin32_platform_file.cpp"
#include "fswin32_input.cpp"

#include "fswin32_platform_window.cpp"
#include "fswin32_game_dll.cpp"

// Todo(Leo): Proper logging and severity system. Severe prints always, and mild only on error
#include "winapi_ErrorStrings.hpp"
internal void
WinApiLog(const char * message, HRESULT result)
{
    #if MAZEGAME_DEVELOPMENT
    if (result != S_OK)
    {
    	log_debug(FILE_ADDRESS, message, "(", WinApiErrorString(result), ")");
    }
    #endif
}


#include "winapi_WinSocketDebugStrings.cpp"
#include "winapi_Audio.cpp"
#include "winapi_Network.cpp"

// Todo(Leo): Get rid of this :) It is used in some stupid places anyway
#include <vector>
#include "fsvulkan.cpp"

/* Todo(Leo): We could/should use WinMain, but it allocates no console
by default so currently we use this */
int main()
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
		#if MAZEGAME_DEVELOPMENT
		
		void * baseAddress = (void*)terabytes(2);
		platformMemory.memory = VirtualAlloc(baseAddress, platformMemory.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		
		#else
		
		platformMemory.memory = VirtualAlloc(nullptr, platformMemory.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
		
		#endif
		
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
	WinApiAudio audio = fswin32_create_audio();
	fswin32_start_playing(&audio);                

	/// ---------- LOAD GAME CODE ----------------------
	// Note(Leo): Only load game dynamically in development.
	Win32Game game = {};
	fswin32_game_load_dll(&game);
	log_application(1, "Game dll loaded");



	  ////////////////////////////////////////////////////
	 ///             MAIN LOOP                        ///
	////////////////////////////////////////////////////

	bool gameIsRunning = true;
	PlatformInput platformInput = {};

	/// --------- TIMING ---------------------------
	PlatformTimePoint frameFlipTime;
	f64 lastFrameElapsedSeconds;

	// Note(Leo): Set to true for the first frame
	bool32 gameShouldReInitializeGlobalVariables = true;

	while(gameIsRunning)
	{
		/// ----- RELOAD GAME CODE -----

		FILETIME dllLatestWriteTime = fswin32_file_get_write_time(GAMECODE_DLL_FILE_NAME);
		if (CompareFileTime(&dllLatestWriteTime, &game.dllWriteTime) > 0)
		{
			log_application(0, "Attempting to reload game");

			fswin32_game_unload_dll(&game);
			fswin32_game_load_dll(&game);

			log_application(0, "Reloaded game");

			gameShouldReInitializeGlobalVariables = true;
		}

		/// ----- HANDLE INPUT -----
		{
			// Note(Leo): this is not input only...
			fswin32_process_pending_messages(&state, window.hwnd);

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

			if (windowIsActive == false)
			{
				update_unused_input(&platformInput);
			}
			else if (xinputUsed)
			{
				update_controller_input(&platformInput, &xinputState);
			}
			else if (state.keyboardInputIsUsed)
			{
				update_keyboard_input(&platformInput, &state.keyboardInput);
				state.keyboardInputIsUsed = false;
			}
			else
			{
				update_unused_input(&platformInput);
			}

			platformInput.mousePosition = state.keyboardInput.mousePosition;
    		platformInput.mouse0   		= update_button_state(platformInput.mouse0, state.keyboardInput.leftMouseButtonDown);
    		platformInput.mouseZoom 	= state.keyboardInput.mouseScroll;

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
												gameShouldReInitializeGlobalVariables);

					// Note(Leo): We must set this to false if we actually have passed it to dll function
					gameShouldReInitializeGlobalVariables = false;

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
		// Note(Leo): Windows will mostly clean up after us once process exits :)
		log_application(0, "shut down");
	}

	return EXIT_SUCCESS;

}