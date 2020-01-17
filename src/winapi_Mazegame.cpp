/*=============================================================================
Leo Tamminen

Windows platform layer for mazegame
=============================================================================*/

#include <WinSock2.h>
#include <Windows.h>
#include <xinput.h>

#include <objbase.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <stdexcept>
#include <vector>
#include <fstream>
#include <chrono>

// TODO(Leo): Make sure that arrays for getting extensions ana layers are large enough
// TOdo(Leo): Combine to fewer functions and remove throws, instead returning specific enum value

/* TODO(Leo) extra: Use separate queuefamily thing for transfering between vertex
staging buffer and actual vertex buffer. https://vulkan-tutorial.com/en/Vertex_buffers/Staging_buffer */

/* STUDY(Leo):
    http://asawicki.info/news_1698_vulkan_sparse_binding_-_a_quick_overview.html
*/

#include "MazegamePlatform.hpp"

// Todo(Leo): hack to get two controller on single pc and use only 1st controller when online
global_variable int globalXinputControllerIndex;

#include "winapi_VulkanDebugStrings.cpp"
#include "winapi_WinSocketDebugStrings.cpp"
#include "winapi_ErrorStrings.hpp"
#include "winapi_Mazegame.hpp"

// Todo(Leo): Vulkan implementation depends on this, not cool
using BinaryAsset = std::vector<u8>;
BinaryAsset
ReadBinaryFile (const char * fileName)
{
    std::ifstream file (fileName, std::ios::ate | std::ios::binary);

    if (file.is_open() == false)
    {
        throw std::runtime_error("failed to open file");
    }

    size_t fileSize = file.tellg();
    BinaryAsset result (fileSize);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(result.data()), fileSize);

    file.close();
    return result;    
}

// Note(Leo): make unity build
#include "winapi_VulkanCommandBuffers.cpp"
#include "winapi_Vulkan.cpp"
#include "winapi_Audio.cpp"
#include "winapi_Network.cpp"
#include "winapi_Input.cpp"
#include "winapi_Window.cpp"

internal void
Run(HINSTANCE winInstance)
{
    winapi::State state = {};
    load_xinput();

    // ---------- INITIALIZE PLATFORM ------------
    WinAPIWindow window         = winapi::make_window(winInstance, 960, 540);
    VulkanContext vulkanContext = winapi::make_vulkan_context(winInstance, window.hwnd);

    HWNDUserPointer userPointer = 
    {
        .window = &window,
        .state = &state,
    };
    SetWindowLongPtrW(window.hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&userPointer));
   
    // SETTING FUNCTION POINTERS
    {
        std::cout << "setting functions\n";

        // GRAPHICS/VULKAN FUNCTIONS
        state.platformFunctions.push_mesh           = vulkan::push_mesh;
        state.platformFunctions.push_model          = vulkan::push_model;
        state.platformFunctions.push_material       = vulkan::push_material;
        state.platformFunctions.push_gui_material   = vulkan::push_gui_material;
        state.platformFunctions.push_texture        = vulkan::push_texture;
        state.platformFunctions.push_cubemap        = vulkan::push_cubemap;
        state.platformFunctions.push_pipeline       = vulkan::push_pipeline;
        state.platformFunctions.unload_scene        = vulkan::unload_scene;

        state.platformFunctions.prepare_frame       = vulkan::prepare_drawing;
        state.platformFunctions.finish_frame        = vulkan::finish_drawing;
        state.platformFunctions.update_camera       = vulkan::update_camera;
        state.platformFunctions.draw_model          = vulkan::record_draw_command;
        state.platformFunctions.draw_line           = vulkan::record_line_draw_command;
        state.platformFunctions.draw_gui            = vulkan::record_gui_draw_command;

        // WINDOW FUNCTIONS
        state.platformFunctions.get_window_width        = [](platform::Window const * window) { return window->width; };
        state.platformFunctions.get_window_height       = [](platform::Window const * window) { return window->height; };
        state.platformFunctions.is_window_fullscreen    = [](platform::Window const * window) { return window->isFullscreen; };
        state.platformFunctions.set_window_fullscreen   = winapi::set_window_fullscreen;

        std::cout << "functions set!\n";   
    }

    // ------- MEMORY ---------------------------
    game::Memory gameMemory = {};
    MemoryArena platformTransientMemoryArena;
    {
        // TODO [MEMORY] (Leo): Properly measure required amount
        // TODO [MEMORY] (Leo): Think of alignment
        gameMemory.persistentMemorySize = gigabytes(1);
        gameMemory.transientMemorySize  = gigabytes(1);

        u64 totalGameMemorySize = gameMemory.persistentMemorySize + gameMemory.transientMemorySize;

        u64 platformTransientMemorySize = megabytes(1);
        u64 totalMemorySize = totalGameMemorySize + platformTransientMemorySize;
   
        // TODO [MEMORY] (Leo): Check support for large pages
    #if MAZEGAME_DEVELOPMENT
        void * baseAddress = (void*)terabytes(2);
        void * memoryBlock = VirtualAlloc(baseAddress, totalMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    #else
        void * memoryBlock = VirtualAlloc(nullptr, totalMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    #endif

        gameMemory.persistentMemory     = memoryBlock;
        gameMemory.transientMemory      = (byte *)memoryBlock + gameMemory.persistentMemorySize;

        platformTransientMemoryArena = make_memory_arena((byte *)memoryBlock + totalGameMemorySize, platformTransientMemorySize);

    }

   // -------- GPU MEMORY ---------------------- 
   {
        // TODO [MEMORY] (Leo): Properly measure required amount
        // TODO[memory] (Leo): Log usage
        u64 staticMeshPoolSize       = megabytes(500);
        u64 stagingBufferPoolSize    = megabytes(100);
        u64 modelUniformBufferSize   = megabytes(100);
        u64 sceneUniformBufferSize   = megabytes(100);
        u64 guiUniformBufferSize     = megabytes(100);

        // TODO[MEMORY] (Leo): This will need guarding against multithreads once we get there
        vulkanContext.staticMeshPool = vulkan::make_buffer_resource(  
                                        &vulkanContext, staticMeshPoolSize,
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        vulkanContext.stagingBufferPool = vulkan::make_buffer_resource(  
                                        &vulkanContext, stagingBufferPoolSize,
                                        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vulkanContext.modelUniformBuffer = vulkan::make_buffer_resource(  
                                        &vulkanContext, modelUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vulkanContext.sceneUniformBuffer = vulkan::make_buffer_resource(
                                        &vulkanContext, sceneUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        vulkanContext.guiUniformBuffer = vulkan::make_buffer_resource( 
                                        &vulkanContext, guiUniformBufferSize,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    }

    // -------- DRAWING ---------
    {
        // Note(Leo): After buffer creations!!
        vulkan::create_uniform_descriptor_pool(&vulkanContext);
        vulkan::create_material_descriptor_pool(&vulkanContext);
        vulkan::create_model_descriptor_sets(&vulkanContext);
        vulkan::create_scene_descriptor_sets(&vulkanContext);
        vulkan::create_texture_sampler(&vulkanContext);
    }

    // -------- INITIALIZE NETWORK ---------
    bool32 networkIsRuined = false;
    WinApiNetwork network = winapi::CreateNetwork();
    game::Network gameNetwork = {};
    real64 networkSendDelay     = 1.0 / 20;
    real64 networkNextSendTime  = 0;
    
    /// --------- INITIALIZE AUDIO ----------------
    WinApiAudio audio = winapi::CreateAudio();
    winapi::StartPlaying(&audio);                

    /// --------- TIMING ---------------------------
    auto startTimeMark          = std::chrono::high_resolution_clock::now();
    real64 lastFrameStartTime   = 0;

    real64 targetFrameTime;
    u32 deviceMinSchedulerGranularity;
    {
        TIMECAPS timeCaps;
        timeGetDevCaps(&timeCaps, sizeof(timeCaps));
        deviceMinSchedulerGranularity = timeCaps.wPeriodMin;

        // TODO(LEO): Make proper settings :)
        /* NOTE(Leo): Change to lower framerate if on battery. My development
        laptop at least changes to slower processing speed when not on power,
        though this probably depends on power settings. These are just some
        arbitrary values that happen to work on development laptop. */
        SYSTEM_POWER_STATUS powerStatus;
        GetSystemPowerStatus (&powerStatus);

        real64 targetFrameTimeYesPower = 60;
        real64 targetFrameTimeNoPower = 30;

        constexpr BYTE AC_ONLINE = 1;
        real64 targetFrameTimeThreshold = (powerStatus.ACLineStatus == AC_ONLINE) ? targetFrameTimeYesPower : targetFrameTimeNoPower;

        if (powerStatus.ACLineStatus == AC_ONLINE)
        {
            std::cout << "Power detected\n";
        }
        else
        {
            std::cout << "No power detected\n";
        }

        s32 gameUpdateRate = targetFrameTimeThreshold;
        {
            HDC deviceContext = GetDC(window.hwnd);
            int monitorRefreshRate = GetDeviceCaps(deviceContext, VREFRESH);
            ReleaseDC(window.hwnd, deviceContext);

            if (monitorRefreshRate > 1)
            {  
                gameUpdateRate = monitorRefreshRate;

                while((gameUpdateRate / 2.0f) > targetFrameTimeThreshold)
                {
                    gameUpdateRate /= 2.0f;
                }
            }
        }
        targetFrameTime = 1.0f / gameUpdateRate;
        
        std::cout << "Target frame time = " << targetFrameTime << ", (fps = " << 1.0 / targetFrameTime << ")\n";
    }

    /// ---------- LOAD GAME CODE ----------------------
    // Note(Leo): Only load game dynamically in development.
    winapi::Game game = {};
    winapi::load_game(&game);

    ////////////////////////////////////////////////////
    ///             MAIN LOOP                        ///
    ////////////////////////////////////////////////////

    bool gameIsRunning = true;
    game::Input gameInput = {};
    while(gameIsRunning)
    {

        /// ----- START TIME -----
        auto currentTimeMark = std::chrono::high_resolution_clock::now();
        real64 frameStartTime = std::chrono::duration<real64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();

        /// ----- RELOAD GAME CODE -----
        FILETIME dllLatestWriteTime = get_file_write_time(GAMECODE_DLL_FILE_NAME);
        if (CompareFileTime(&dllLatestWriteTime, &game.dllWriteTime) > 0)
        {
            winapi::unload_game(&game);
            winapi::load_game(&game);
        }

        /// ----- HANDLE INPUT -----
        {
            // Note(Leo): this is not input only...
            winapi::ProcessPendingMessages(&state, window.hwnd);

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
                update_unused_input(&gameInput);
            }
            else if (xinputUsed)
            {
                update_controller_input(&gameInput, &xinputState);
            }
            else if (state.keyboardInputIsUsed)
            {
                update_keyboard_input(&gameInput, &state.keyboardInput);
                state.keyboardInputIsUsed = false;
            }
            else
            {
                update_unused_input(&gameInput);
            }

            gameInput.elapsedTime = targetFrameTime;
        }

        /// ----- PRE-UPDATE NETWORK PASS -----
        {
            // Todo(Leo): just quit now if some unhandled network error occured
            if (networkIsRuined)
            {
                auto error = WSAGetLastError();
                std::cout << "NETWORK failed: " << WinSocketErrorString(error) << " (" << error << ")\n"; 
                break;
            }

            if (network.isListening)
            {
                winapi::NetworkListen(&network);
            }
            else if (network.isConnected)
            {
                winapi::NetworkReceive(&network, &gameNetwork.inPackage);
            }
            gameNetwork.isConnected = network.isConnected;
        }

        /// --------- UPDATE GAME -------------
        // Note(Leo): Game is not updated when window is not drawable.
        if (winapi::is_window_drawable(&window))
        {
            // Todo(Leo): Study fences
            // Todo(Leo): Vulkan related things should be moved to vulkan section            
            vkWaitForFences(vulkanContext.device, 1, &get_current_virtual_frame(&vulkanContext)->inFlightFence,
                            VK_TRUE, VULKAN_NO_TIME_OUT);

            u32 imageIndex;
            VkResult getNextImageResult = vkAcquireNextImageKHR(vulkanContext.device,
                                                                vulkanContext.swapchainItems.swapchain,
                                                                VULKAN_NO_TIME_OUT,//MaxValue<u64>,
                                                                get_current_virtual_frame(&vulkanContext)->imageAvailableSemaphore,
                                                                VK_NULL_HANDLE,
                                                                &imageIndex);
            
            if(winapi::is_loaded(&game))
            {
                vulkanContext.currentDrawFrameIndex = imageIndex;
    
                game::SoundOutput gameSoundOutput = {};
                winapi::GetAudioBuffer(&audio, &gameSoundOutput.sampleCount, &gameSoundOutput.samples);

                gameIsRunning = game.Update(&gameInput, &gameMemory,
                                            &gameNetwork, &gameSoundOutput,
                                            
                                            &vulkanContext,
                                            &window,
                                            &state.platformFunctions);

                winapi::ReleaseAudioBuffer(&audio, gameSoundOutput.sampleCount);
            }

            // Note(Leo): It doesn't so much matter where this is checked.
            if (window.shouldClose)
            {
                gameIsRunning = false;
            }

            /// ---- DRAW -----    
            /*
            Note(Leo): Only draw image if we have window that is not minimized. Vulkan on windows MUST not
            have framebuffer size 0, which is what minimized window is.
            */
        
            switch (getNextImageResult)
            {
                case VK_SUCCESS:
                    vulkan::draw_frame(&vulkanContext, imageIndex);
                    break;

                case VK_SUBOPTIMAL_KHR:
                case VK_ERROR_OUT_OF_DATE_KHR:
                    vulkan::recreate_swapchain(&vulkanContext, window.width, window.height);
                    break;

                default:
                    throw std::runtime_error("Failed to acquire swap chain image");
            }
        }



        // ----- POST-UPDATE NETWORK PASS ------
        // TODO[network](Leo): Now that we have fixed frame rate, we should count frames passed instead of time
        if (network.isConnected && frameStartTime > networkNextSendTime)
        {
            networkNextSendTime = frameStartTime + networkSendDelay;
            winapi::NetworkSend(&network, &gameNetwork.outPackage);
        }

        /// ----- CLEAR MEMORY ------
        flush_memory_arena(&platformTransientMemoryArena);

        /// ----- WAIT FOR TARGET FRAMETIME -----
        {
            auto currentTimeMark = std::chrono::high_resolution_clock::now();
            real64 currentTimeSeconds = std::chrono::duration<real64, std::chrono::seconds::period>(currentTimeMark - startTimeMark).count();

            real64 elapsedSeconds = currentTimeSeconds - frameStartTime;
            real64 timeToSleepSeconds = targetFrameTime - elapsedSeconds;

            /* TODO[time](Leo): It seems okay to sleep 0 milliseconds in case the time to sleep ends up being
            less than 1 millisecond on floating point representation. Also we may want to do a busy loop over
            remainder time after sleep. This is due to windows scheduler granularity, that is at best on 
            one millisecond scale. */
            if (timeToSleepSeconds > 0)
            {
                DWORD timeToSleepMilliSeconds = static_cast<DWORD>(1000 * timeToSleepSeconds);
                // std::cout << "Sleep for " << timeToSleepMilliSeconds << " ms\n";
                Sleep(timeToSleepMilliSeconds);

            }
        }
    }
    ///////////////////////////////////////
    ///         END OF MAIN LOOP        ///
    ///////////////////////////////////////


    /// -------- CLEANUP ---------
    winapi::StopPlaying(&audio);
    winapi::ReleaseAudio(&audio);
    winapi::CloseNetwork(&network);

    /// ------- CLEANUP VULKAN -----
    {
        // Note(Leo): All draw frame operations are asynchronous, must wait for them to finish
        vkDeviceWaitIdle(vulkanContext.device);
        Cleanup(&vulkanContext);

        std::cout << "[VULKAN]: shut down\n";
    }
    /// ----- Cleanup Windows
    {
        // Note(Leo): Windows will mostly clean up after us once process exits :)


        /* TODO(Leo): If there are multiple of these it means we had crash previously
        and want to reset this. This should be handled betterly though.... :( */
        timeEndPeriod(deviceMinSchedulerGranularity);
        std::cout << "[WINDOWS]: shut down\n";
    }
}

int CALLBACK
WinMain(
    HINSTANCE   winInstance,
    HINSTANCE   previousWinInstance,
    LPSTR       cmdLine,
    int         showCommand)
{
    /* Todo(Leo): we should make a decision about how we handle errors etc.
    Currently there are exceptions (which lead here) and asserts mixed ;) */
    try {
        Run(winInstance);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}