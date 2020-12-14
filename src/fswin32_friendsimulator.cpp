/*
Leo Tamminen

Windows platform implementation layer for Friendsimulator.
This is the first file to compile, and everything is included from here.

Todo(Leo):
	- Use win32 AdjustClientRect thing when creating window to make right sized window
*/

#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Windowsx.h>
#include <xinput.h>

#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

/*
Note(Leo): Define configurations here. Currently we split develompnent code to platform layer executable
and game layer hot-reloadable dll. We also define different entrypoint names, because release mode does
not need console to be created.
*/

#if defined FS_RELEASE

	#define FS_DEVELOPMENT_ONLY(expr) (void(0))
	#define FS_RELEASE_ONLY(expr) expr

	#define FS_ENTRY_POINT int WinMain(HINSTANCE, HINSTANCE, LPSTR, int)

#elif defined FS_DEVELOPMENT

	#define FS_DEVELOPMENT_ONLY(expr) expr
	#define FS_RELEASE_ONLY(expr) (void(0))

	#define FS_ENTRY_POINT int main()

#else
	#error "FS_DEVELOPMENT or FS_RELEASE must be defined."
#endif

#include "fs_standard_types.h"
#include "fs_standard_functions.h"

#define FS_PLATFORM
#include "fs_platform_interface.hpp"

#include "fs_logging.cpp"

#include "fswin32_platform_log.cpp"
#include "fswin32_platform_time.cpp"
#include "fswin32_platform_file.cpp"

// Todo(Leo): these can be in same file, and maybe even combine them. Windows seems to do that.
#include "win32_platform_window.cpp"
#include "win32_platform_input.cpp"

#include "win32_audio.cpp"

// Todo(Leo): Get rid of this :) It is used in some stupid places anyway
#include <vector>
#include "fsvulkan.cpp"

#include <ImGui/imgui.cpp>
#include <ImGui/imgui_draw.cpp>
#include <ImGui/imgui_widgets.cpp>
#include <ImGui/imgui_demo.cpp>

#include <ImGui/imgui_impl_win32.cpp>
#include <ImGui/imgui_impl_vulkan.cpp>

#include <ImGui/ImGuizmo.cpp>

#include "win32_game_dll.cpp"
#include "win32_window_callback.cpp"

#if defined FS_RELEASE
	#include "friendsimulator.cpp"
#endif

FS_ENTRY_POINT
{
	log_application(0,"\n",
				"\t----- FriendSimulator -----\n",
				"\tBuild time: ", BUILD_DATE_TIME, "\n");

	// ----------------------------------------------------------------------------------

	Win32PlatformWindow window;
	Win32PlatformInput input;

	Win32WindowUserData userData = {&window, &input};

	window 	= win32_platform_window_create(&userData, win32_window_callback, 960, 540);
	input 	= win32_platform_input_create();
	

	/// --------- INITIALIZE AUDIO ----------------
	f32 audioBufferLengthSeconds = 0.1;
	Win32Audio audio = fswin32_create_audio(audioBufferLengthSeconds);
	fswin32_start_playing(&audio);                

	MemoryBlock gameMemory = {};
	{
		// TODO [MEMORY] (Leo): Properly measure required amount
		// TODO [MEMORY] (Leo): Think of alignment
		gameMemory.size = gigabytes(2);

		// TODO [MEMORY] (Leo): Check support for large pages
		FS_DEVELOPMENT_ONLY(void * baseAddress = (void*)terabytes(2));
		FS_RELEASE_ONLY(void * baseAddress = nullptr);
		gameMemory.memory = reinterpret_cast<u8*>(VirtualAlloc(baseAddress, gameMemory.size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
		Assert(gameMemory.memory != nullptr);
	}

	VulkanContext graphics = winapi::create_vulkan_context(&window);
   


	// Todo(Leo): Move imgui stuff away, maybe after we have imguizmo
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO & io = ImGui::GetIO();
		(void)io;
		// io.ConfigFlags |= 

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(window.hwnd);

		ImGui_ImplVulkan_InitInfo initInfo 	= {};
		initInfo.Instance 					= graphics.instance;
		initInfo.PhysicalDevice 			= graphics.physicalDevice;
		initInfo.Device 					= graphics.device;
		initInfo.QueueFamily 				= graphics.graphicsQueueFamily;
		initInfo.Queue 						= graphics.graphicsQueue;
		initInfo.PipelineCache 				= VK_NULL_HANDLE;
		initInfo.DescriptorPool 			= graphics.persistentDescriptorPool;
		initInfo.MinImageCount 				= 2;
		initInfo.ImageCount 				= graphics.swapchainImages.size();
		initInfo.MSAASamples 				= VK_SAMPLE_COUNT_1_BIT;
		initInfo.Allocator 					= nullptr;
		initInfo.CheckVkResultFn 			= nullptr;

		ImGui_ImplVulkan_Init(&initInfo, graphics.screenSpaceRenderPass);

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VULKAN_CHECK(vkBeginCommandBuffer(graphics.virtualFrames[0].mainCommandBuffer, &beginInfo));
		
		ImGui_ImplVulkan_CreateFontsTexture(graphics.virtualFrames[0].mainCommandBuffer);

		VULKAN_CHECK(vkEndCommandBuffer(graphics.virtualFrames[0].mainCommandBuffer));

		VkSubmitInfo submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &graphics.virtualFrames[0].mainCommandBuffer;

		VULKAN_CHECK(vkQueueSubmit(graphics.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE));

		VULKAN_CHECK(vkDeviceWaitIdle(graphics.device));

		ImGui_ImplVulkan_DestroyFontUploadObjects();

	}













	/// ---------- LOAD GAME CODE ----------------------
	// Note(Leo): Only load game dynamically in development.
	Win32Game game = {};

	FS_DEVELOPMENT_ONLY(fswin32_game_load_dll(&game));
	FS_RELEASE_ONLY(game.update = game_update );	

	s64 frameFlipTime 			= platform_time_now();
	f64 lastFrameElapsedSeconds = 0;

	bool gameIsRunning 			= true;

	while(gameIsRunning)
	{
		/// ----- RELOAD GAME CODE -----

		FS_DEVELOPMENT_ONLY(fswin32_game_reload(game));

		/// ----- HANDLE INPUT -----

		// Todo(Leo): merge to single function call, these are not used anywhere else (or at least they should not)
		win32_input_reset(input);
		win32_window_process_messages(window);
		win32_input_update(input, window);

		if (window.events.resized && window.isMinimized == false)
		{
			vulkan_recreate_drawing_resources(&graphics, window.width, window.height);
		}

		// Todo(Leo): Also ask vulkan
		if (win32_window_is_drawable(&window))
		{
			vulkan_prepare_frame(&graphics);

			// Todo(Leo): merge
			{
				ImGui_ImplVulkan_NewFrame();			
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
			}

			// Todo(Leo): use function directly in release build
			gameIsRunning = game.update(gameMemory,
										&input, &graphics, &window, &audio,
										lastFrameElapsedSeconds);

			// Todo(Leo): merge
			{
				ImGui::EndFrame();
				ImGui::Render();
				ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
												graphics.virtualFrames[graphics.virtualFrameIndex].guiCommandBuffer);
			}


			vulkan_render_frame(&graphics);
		}

		if (window.shouldClose)
		{
			gameIsRunning = false;
		}

		// ----- MEASURE ELAPSED TIME ----- 
		{
			#if defined FS_DEVELOPMENT && 0
			{
				s64 currentTime 				= platform_time_now();
				f64 elapsedTimeBeforeSleep 		= platform_time_elapsed_seconds(frameFlipTime, currentTime);
				f32 minFrameTime 				= 1.0f / 30;
				s32 millisecondsToSleep 		= static_cast<s32>((minFrameTime - elapsedTimeBeforeSleep) * 1000);

				if (millisecondsToSleep > 0)
				{
					timeBeginPeriod(1);
					Sleep(millisecondsToSleep);
					timeEndPeriod(1);
				}
			}
			#endif

			s64 currentTime   		= platform_time_now();
			lastFrameElapsedSeconds = platform_time_elapsed_seconds(frameFlipTime, currentTime);
			frameFlipTime           = currentTime;
		}
	}
	///////////////////////////////////////
	///         END OF MAIN LOOP        ///
	///////////////////////////////////////


	/// -------- CLEANUP ---------

	vkDeviceWaitIdle(graphics.device);

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	winapi::destroy_vulkan_context(&graphics);
	
	fswin32_stop_playing(&audio);
	fswin32_release_audio(&audio);
	
	/// ----- Cleanup Windows
	{
		FS_DEVELOPMENT_ONLY(fswin32_game_unload_dll(&game));
		log_application(0, "shut down");
	}

	return EXIT_SUCCESS;

}