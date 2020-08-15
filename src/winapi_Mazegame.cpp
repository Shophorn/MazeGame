/*=============================================================================
Leo Tamminen

Windows platform layer for mazegame
=============================================================================*/

#include <WinSock2.h>
#include <Windows.h>
#include <Windowsx.h>
#include <xinput.h>

#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

// Todo(Leo): Get rid of these :)
#include <vector>
#include <fstream>

// TODO(Leo): Make sure that arrays for getting extensions ana layers are large enough
// TOdo(Leo): Combine to fewer functions and remove throws, but return specific enum value instead

/* TODO(Leo) extra: Use separate queuefamily thing for transfering between vertex
staging buffer and actual vertex buffer. https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer */

/* STUDY(Leo):
	http://asawicki.info/news_1698_vulkan_sparse_binding_-_a_quick_overview.html
*/

#include "MazegamePlatform.hpp"


/// PLATFORM TIME IMPLEMENTATION
PlatformTimePoint fswin32_current_time () 
{
	LARGE_INTEGER t;
	QueryPerformanceCounter(&t);
	return {t.QuadPart};
}

f64 fswin32_elapsed_seconds(PlatformTimePoint start, PlatformTimePoint end)
{
	local_persist s64 frequency = []()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		
		logConsole(0) << "performance frequency = " << frequency.QuadPart;

		return frequency.QuadPart;
	}();

	f64 seconds = (f64)(end.value - start.value) / frequency;
	return seconds;
}

/// PLATFORM FILE IMPLEMENTATION
PlatformFileHandle fswin32_open_file(char const * filename, FileMode fileMode)
{
	DWORD access;
	if(fileMode == FILE_MODE_READ)
	{
		access = GENERIC_READ;
	}
	else if (fileMode == FILE_MODE_WRITE)
	{
		access = GENERIC_WRITE;
	}

	// Todo(Leo): check lpSecurityAttributes
	HANDLE file = CreateFileA(	filename,
								access,
								0,
								nullptr,
								OPEN_ALWAYS,
								FILE_ATTRIBUTE_NORMAL,
								nullptr);

	SetFilePointer((HANDLE)file, 0, nullptr, FILE_BEGIN);

	if (fileMode == FILE_MODE_WRITE)
	{
		SetEndOfFile((HANDLE)file);
	}

	PlatformFileHandle result 	= (PlatformFileHandle)file;
	return result;

}

void fswin32_close_file(PlatformFileHandle file)
{
	CloseHandle((HANDLE)file);
}

void fswin32_set_file_position(PlatformFileHandle file, s32 location)
{
	SetFilePointer((HANDLE)file, location, nullptr, FILE_BEGIN);
}

s32 fswin32_get_file_position(PlatformFileHandle file)
{
	DWORD position = SetFilePointer((HANDLE)file, 0, nullptr, FILE_CURRENT);
	return (s32)position;
}

void fswin32_write_file (PlatformFileHandle file, s32 count, void * memory)
{
	DWORD bytesWritten;
	WriteFile((HANDLE)file, memory, count, &bytesWritten, nullptr);

	SetFilePointer((HANDLE)file, count, nullptr, FILE_CURRENT);
}

void fswin32_read_file (PlatformFileHandle file, s32 count, void * memory)
{
	DWORD bytesRead;
	ReadFile((HANDLE)file, memory, count, &bytesRead, nullptr);

	SetFilePointer((HANDLE)file, count, nullptr, FILE_CURRENT);
}

s32 fswin32_get_file_length(PlatformFileHandle file)
{
	LARGE_INTEGER fileSize;
	GetFileSizeEx((HANDLE)file, &fileSize);

	Assert(fileSize.QuadPart < max_value_s32);

	return (s32)fileSize.QuadPart;
}

/* Note(Leo): read file until delimiter is found, or memory size is reached */
s32 fswin32_read_file_until(PlatformFileHandle file, char delimiter, s32 memorySize, void * memory)
{
	char * buffer = (char*)memory;
	s32 count = 0;


	while(count < memorySize)
	{
		DWORD bytesRead;
		ReadFile((HANDLE)file, (void*)buffer, 1, &bytesRead, nullptr);
		
		if (bytesRead == 0)
		{
			break;
		}

		SetFilePointer((HANDLE)file, 1, nullptr, FILE_CURRENT);

		if (*buffer == delimiter)
		{
			break;
		}

		++buffer;
		++count;
	}

	return count;
}


// Todo(Leo): hack to get two controller on single pc and use only 1st controller when online
global_variable int globalXinputControllerIndex;

#include "winapi_VulkanDebugStrings.cpp"
#include "winapi_WinSocketDebugStrings.cpp"
#include "winapi_ErrorStrings.hpp"
#include "winapi_Mazegame.hpp"

// Todo(Leo): Vulkan implementation depends on this, not cool
using BinaryAsset = std::vector<u8>;
BinaryAsset
BAD_read_binary_file (const char * filename)
{
	std::ifstream file (filename, std::ios::ate | std::ios::binary);

	AssertMsg(file.is_open(), filename);

	size_t fileSize = file.tellg();
	BinaryAsset result (fileSize);

	file.seekg(0);
	file.read(reinterpret_cast<char*>(result.data()), fileSize);

	file.close();
	return result;    
}

// Note(Leo): make unity build
#include "fswin32_input.cpp"
#include "fswin32_window.cpp"
#include "winapi_Audio.cpp"
#include "winapi_Network.cpp"

#include "fsvulkan.cpp"

internal void
Run(HINSTANCE hInstance)
{
	SYSTEMTIME time_;
	GetLocalTime(&time_);

	SmallString timestamp   = make_timestamp(time_.wHour, time_.wMinute, time_.wSecond);
	auto logFileName        = SmallString("logs/log_").append(timestamp).append(".log");

	auto logFile = std::ofstream(logFileName.data());

	logDebug.output     = &logFile;
	logAnim.output      = &logFile;
	logVulkan.output    = &logFile;
	logWindow.output    = &logFile;
	logSystem.output    = &logFile;
	logNetwork.output   = &logFile;
	logAudio.output     = &logFile;


	logSystem(0) << "\n"
				<< "\t----- FriendSimulator -----\n"
				<< "\tBuild time: " << BUILD_DATE_TIME << "\n";

	// ----------------------------------------------------------------------------------

	winapi::State state = {};
	load_xinput();

	// ---------- INITIALIZE PLATFORM ------------
	PlatformWindow window = fswin32_make_window(hInstance, 960, 540);
	
	VulkanContext vulkanContext = winapi::create_vulkan_context(&window);
	HWNDUserPointer userPointer = 
	{
		.window = &window,
		.state = &state,
	};
	SetWindowLongPtrW(window.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&userPointer));
   
	PlatformApi platformApi = {};

	// SETTING FUNCTION POINTERS
	{
		// WINDOW FUNCTIONS
		platformApi.get_window_width        = [](PlatformWindow const * window) { return window->width; };
		platformApi.get_window_height       = [](PlatformWindow const * window) { return window->height; };
		platformApi.is_window_fullscreen    = [](PlatformWindow const * window) { return window->isFullscreen; };
		platformApi.set_window_fullscreen   = fswin32_set_window_fullscreen;
		platformApi.set_cursor_visible 		= fswin32_set_cursor_visible;

		fsvulkan_set_platform_graphics_api(&vulkanContext, &platformApi);

		platformApi.current_time 	= fswin32_current_time;
		platformApi.elapsed_seconds = fswin32_elapsed_seconds;

		platformApi.open_file 			= fswin32_open_file;
		platformApi.close_file 			= fswin32_close_file;
		platformApi.set_file_position 	= fswin32_set_file_position;
		platformApi.get_file_position 	= fswin32_get_file_position;
		platformApi.write_file 			= fswin32_write_file;
		platformApi.read_file 			= fswin32_read_file;
		platformApi.get_file_length 	= fswin32_get_file_length;
		platformApi.read_file_until 	= fswin32_read_file_until;

		Assert(platform_all_functions_set(&platformApi));
	}

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
	winapi::Game game = {};
	winapi::load_game(&game);
	logSystem() << "Game dll loaded";

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
		FILETIME dllLatestWriteTime = get_file_write_time(GAMECODE_DLL_FILE_NAME);
		if (CompareFileTime(&dllLatestWriteTime, &game.dllWriteTime) > 0)
		{
			logSystem(0) << "Attempting to reload game";

			winapi::unload_game(&game);
			winapi::load_game(&game);

			logSystem(0) << "Reloaded game";
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
			bool32 xinputReceived = XInputGetState(globalXinputControllerIndex, &xinputState) == ERROR_SUCCESS;
			bool32 xinputUsed = xinputReceived && xinput_is_used(&state, &xinputState);

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

			platformInput.mousePosition = state.mousePosition;
    		platformInput.mouse0   		= update_button_state(platformInput.mouse0, state.leftMouseButtonDown);

		}


		/// ----- PRE-UPDATE NETWORK PASS -----
		{
			// Todo(Leo): just quit now if some unhandled network error occured
			if (networkIsRuined)
			{
				auto error = WSAGetLastError();
				logNetwork() << "failed: " << WinSocketErrorString(error) << " (" << error << ")"; 
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
					gameIsRunning = game.Update(&platformInput, 
												&platformTime,
												&platformMemory,
												&platformNetwork,
												&gameSoundOutput,
												
												&vulkanContext,
												&window,
												&platformApi,
												&logFile);
					break;

				case PGFR_FRAME_RECREATE:
					fsvulkan_recreate_drawing_resources(&vulkanContext, window.width, window.height);
					break;

				case PGFR_FRAME_BAD_PROBLEM:
					AssertMsg(false, "We should not be here, please investigate");


			}


			fswin32_release_audio_buffer(&audio, gameSoundOutput.sampleCount);
			// Note(Leo): It doesn't so much matter where this is checked.
			if (window.shouldClose)
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
			PlatformTimePoint now   = fswin32_current_time();
			lastFrameElapsedSeconds = fswin32_elapsed_seconds(frameFlipTime, now);

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

			now   					= fswin32_current_time();
			lastFrameElapsedSeconds = fswin32_elapsed_seconds(frameFlipTime, now);
		#endif
			frameFlipTime           = now;

			// approxAvgFrameTime = interpolate(approxAvgFrameTime, lastFrameElapsedSeconds, approxAvgFrameTimeAlpha);
			// logConsole(0) << approxAvgFrameTime;
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
		logWindow(0) << "shut down\n";
	}

}


// int CALLBACK
// WinMain(
//     HINSTANCE   hInstance,
//     HINSTANCE   previousWinInstance,
//     LPSTR       cmdLine,
//     int         showCommand)
// {



int main()
{
	/* Todo(Leo): we should make a decision about how we handle errors etc.
	Currently there are exceptions (which lead here) and asserts mixed ;)

	Maybe, put a goto on asserts, so they would come here too? */
	try {

		HMODULE module = GetModuleHandle(nullptr);
		Run(module);
	} catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}